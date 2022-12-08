﻿#include "windowspacker.h"
#include "Public/appsignal.h"
#include "Public/appconfig.h"

#include <thread>
#include <QFile>
#include <QTextCodec>
#include <QFileInfo>
#include <regex>
#include <string>
#include <QDir>

#ifdef Q_OS_WINDOWS
#include <stdio.h>
#include <Windows.h>
#endif

using namespace std;

// test
#include <QDebug>

WindowsPacker::WindowsPacker(QObject *parent)
    : QObject{parent}
{

}

void WindowsPacker::pack(const QString &path, bool isWidget, bool isSimpleMode)
{
    if (mThreadPacking)
    {
        emit AppSignal::getInstance()->sgl_system_logger_message("系统正在进行程序打包，请等待本次打包工作结束", "#fc9153");
        return;
    }

    if (!QFile::exists(path))
    {
        emit AppSignal::getInstance()->sgl_system_logger_message("可执行程序不存在，请检查程序路径", "#dd3737");
        return;
    }

    std::lock_guard<std::mutex> lock(mMutex);
    mThreadPacking= true;

    mIsSimpleMode = isSimpleMode;
    mIsQtWidgetType = isWidget;

    auto func = std::bind(&WindowsPacker::threadPack, this, path);
    std::thread th(func);
    th.detach();
}

void WindowsPacker::threadPack(const QString &path)
{
    // 文件详细信息
    QFileInfo fileInfo(path);

    emit AppSignal::getInstance()->sgl_system_logger_message("开始运行 Qt 打包流程 【windeployqt】", "#fc9153");

    bool collectQtDependsFlag = false;

    // 先运行系统的 windeployqt 程序，没有就提示找不到程序
    FILE *fp = nullptr;
    char buf[1024] = { 0 };

    AllocConsole();
    ShowWindow(GetConsoleWindow(), SW_HIDE);
    // 此处读取配置文件中 qml 文件夹的位置
    fp = _popen(QString("windeployqt %1 %2 %3").arg(
                    fileInfo.absoluteFilePath(),
                    QString(mIsQtWidgetType ? " " : "–qmldir").toStdString().data(),
                    QString(mIsQtWidgetType ? " " : " ").toStdString().data()).toStdString().data(), "r");
    if(fp)
    {
        size_t ret = fread(buf, 1, sizeof(buf) - 1, fp);
        if(ret > 0)
        {
            //QTextCodec *codec = QTextCodec::codecForName("GBK");
            //QString message = QString::fromUtf8(codec->toUnicode(QByteArray(buf)).toUtf8());
            emit AppSignal::getInstance()->sgl_system_logger_message("基础依赖库打包完成", "#fc9153");
            collectQtDependsFlag = true;
        }
        else
        {
            emit AppSignal::getInstance()->sgl_system_logger_message("打包程序 【windeployqt】 不存在，请先配置其环境变量", "#dd3737");
        }
        _pclose(fp);
    }
    FreeConsole();

    if (!collectQtDependsFlag)
    {
        emit AppSignal::getInstance()->sgl_system_logger_message("打包未完成，已中止", "#fc9153");

        std::lock_guard<std::mutex> lock(mMutex);
        mThreadPacking = false;
        return;
    }

    // 运行 Visual 2019 (或其他版本) dumpbin 程序，没有就提示找不到
    emit AppSignal::getInstance()->sgl_system_logger_message("开始运行 Windows 打包流程 【dumpbin】", "#fc9153");

    // 检索路径需要通过配置文件记录，使用者自己添加，所有环境变量检索的结果很离谱
    QStringList listSystemPath;
    // 默认先加一个当前文件夹
    listSystemPath.append(fileInfo.absolutePath());
    QStringList list = AppConfig::getInstance()->getValue("SearchPath", "value").split(";");
    for (auto &item : list)
    {
        if (!QDir(item.trimmed()).exists()) continue;
        listSystemPath.append(item.trimmed());
    }

    uint32_t length = 1024 * 10;
    char *buffer = new char[length];
    QStringList listDepends;
    QStringList listDependsLoss;
    AllocConsole();
    ShowWindow(GetConsoleWindow(), SW_HIDE);

    auto findDepends = [&listDepends, &listSystemPath, &listDependsLoss, &buffer, &length] (const QString &item)
    {
        FILE *fp = nullptr;
        memset(buffer, 0, length);

        fp = _popen(QString("dumpbin /DEPENDENTS %1").arg(item).toStdString().data(), "r");
        if(fp)
        {
            size_t ret = fread(buffer, 1, length - 1, fp);
            if(ret > 0)
            {
                QTextCodec *codec = QTextCodec::codecForName("GBK");
                QString message = QString::fromUtf8(codec->toUnicode(QByteArray(buffer)).toUtf8());

                QStringList tempList = message.replace("\n", " ").replace("\r", " ").split(" ", Qt::SkipEmptyParts);

                QStringList listDll;

                for (auto &tempDll : tempList)
                {
                    if (!tempDll.endsWith(".dll", Qt::CaseSensitive)) continue;
                    if (tempDll.contains("\\")) continue;
                    listDll.append(tempDll.trimmed());
                }

                if (listDll.isEmpty())
                {
                     _pclose(fp);
                     emit AppSignal::getInstance()->sgl_system_logger_message(QString("动态库 %1 没有后续依赖").arg(item), "#3f8f54");
                    return;
                }

                for (auto &dll : listDll)
                {
                    if (listDepends.contains(dll)) continue;
                    if (listDependsLoss.contains(dll)) continue;

                    // 在系统中查询该文件的绝对路径
                    bool flag = false;
                    for (auto &path : listSystemPath)
                    {
                        QDir dir(path);
                        QStringList fileList = dir.entryList({"*.dll"}, QDir::Files);

                        if (fileList.contains(dll, Qt::CaseInsensitive))
                        {
                            flag = true;
                            QString absoluteFilePath = QString("%1/%2").arg(path, dll);
                            if (listDepends.contains(absoluteFilePath)) break;

                            emit AppSignal::getInstance()->sgl_system_logger_message(QString("找到一个系统依赖库 %1 文件  【序号： %2】").arg(dll, QString::number(listDepends.size())), "#3f8f54");
                            listDepends.append(absoluteFilePath); break;
                        }
                    }

                    if (!flag)
                    {
                        listDependsLoss.append(dll);
                        emit AppSignal::getInstance()->sgl_system_logger_message(QString("系统没有找到 %1 的依赖 %2 文件").arg(item, dll), "#dd3737");
                    }
                }

                //qDebug() << "listDepends " << listDepends;
            }
            else
            {
                emit AppSignal::getInstance()->sgl_system_logger_message("打包程序 【windeployqt】 不存在，请先配置其环境变量", "#dd3737");
            }
        }
        _pclose(fp);
    };

    // 查询顶层
    findDepends(fileInfo.absoluteFilePath());

    // 非简洁模式，进行循环查找
    if (!mIsSimpleMode)
    {
        // 后续循环查询
        int32_t index = 0;
        while (index < listDepends.size())
        {
            QString item = listDepends.at(index);
            emit AppSignal::getInstance()->sgl_system_logger_message(QString("正在查询 %1 的所有依赖").arg(item), "#fc9153");
            findDepends(item);
            index++;
        }
    }

    FreeConsole();

    delete [] buffer;

    emit AppSignal::getInstance()->sgl_system_logger_message(QString("当前已经尽可能的确定所有依赖库位置，准备开始拷贝共 %1 个动态库文件").arg(QString::number(listDepends.size())), "#3f8f54");

    // 此处开始文件拷贝
    int64_t size = listDepends.size();
    int64_t ignoreNumber = 0;
    for (int64_t i = 0; i < size; i++)
    {
        QFileInfo info(listDepends.at(i));

        // 就算有，也要覆盖，感觉更合理
        if (QFile::exists(QString("%1/%2").arg(fileInfo.absolutePath(), info.fileName())))
        {
            ignoreNumber++;
            emit AppSignal::getInstance()->sgl_system_logger_message(QString("文件 %1 已经存在, 跳过复制").arg(info.fileName()), "#fc9153");
            continue;
        }

        bool status = QFile::copy(listDepends.at(i), QString("%1/%2").arg(fileInfo.absolutePath(), info.fileName()));
        emit AppSignal::getInstance()->sgl_system_logger_message(QString("文件 %1 拷贝 %2").arg(info.fileName(), status ? "成功" : "失败"), status ? "#3f8f54" : "dd3737");
    }
    // 此处文件拷贝结束

    emit AppSignal::getInstance()->sgl_system_logger_message(QString("系统依赖库收集程序完成；成功【%1 个】，跳过【%2 个】，失败【%3 个】；祝你好运 ~").arg(
                                                                 QString::number(listDepends.size()),
                                                                 QString::number(ignoreNumber),
                                                                 QString::number(listDependsLoss.size())), "#3f8f54");

    std::lock_guard<std::mutex> lock(mMutex);
    mThreadPacking = false;
}
