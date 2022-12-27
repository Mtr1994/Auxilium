#include "linuxpacker.h"
#include "Public/appsignal.h"
#include "Public/appconfig.h"

#include <QFile>
#include <QFileInfo>
#include <thread>
#include <QTextCodec>
#include <QDir>

// test
#include <QDebug>

// 获取 linuxdeployqt 程序： https://github.com/probonopd/linuxdeployqt/releases

LinuxPacker::LinuxPacker(QObject *parent)
    : QObject{parent}
{

}

void LinuxPacker::pack(const QString &path, bool isWidget, bool isSimpleMode, const QString &sourceroot)
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
    mSourceRoot = sourceroot;

    auto func = std::bind(&LinuxPacker::threadPack, this, path);
    std::thread th(func);
    th.detach();
}

void LinuxPacker::threadPack(const QString &path)
{
#ifdef Q_OS_LINUX
    // 文件详细信息
    QFileInfo fileInfo(path);

    emit AppSignal::getInstance()->sgl_system_logger_message("开始运行 Qt 打包流程 【linuxdeployqt】", "#fc9153");

    bool collectQtDependsFlag = false;

    // 先运行系统的 linuxdeployqt 程序，没有就提示找不到程序
    FILE *fp = nullptr;
    char buf[1024] = { 0 };
    QString qmlDir;

    if (!mIsQtWidgetType)
    {
        // 先查找 Qt 的 qml 模块位置，没有就提示找不到程序
        fp = popen(QString("whereis uic").toStdString().data(), "r");

        if (fp)
        {
            size_t ret = fread(buf, 1, sizeof(buf) - 1, fp);
            if(ret > 0)
            {
                QTextCodec *codec = QTextCodec::codecForName("GBK");
                QString message = QString::fromUtf8(codec->toUnicode(QByteArray(buf)).toUtf8());
                qmlDir = QFileInfo(message.remove("\n")).absolutePath() + "/../qml";
            }
            else
            {
                emit AppSignal::getInstance()->sgl_system_logger_message("找不到 QML 模块位置，请先配置 Qt 环境变量", "#dd3737");
            }
            pclose(fp);
        }

        if (qmlDir.isEmpty()) return;

        // qml 程序需要解析源码，获取模块信息，耗时更久
        emit AppSignal::getInstance()->sgl_system_logger_message("正在查找 QML 模块依赖，可能会持续一段时间，请等待", "#fc9153");
    }

    // qmldir 是程序源码的根目录，根据源码里面引入的内容，确定要打包哪些模块
    // qmlimport 是 qml 系统模块的主目录
    fp = popen(QString("linuxdeployqt %1 %2 -appimage").arg(
                    fileInfo.absoluteFilePath(),
                    QString(mIsQtWidgetType ? " " : ("--qmldir " + mSourceRoot + " --qmlimport " + qmlDir))).toStdString().data(), "r");
    if(fp)
    {
        size_t ret = fread(buf, 1, sizeof(buf) - 1, fp);
        if(ret > 0)
        {
            //QTextCodec *codec = QTextCodec::codecForName("GBK");
            //QString message = QString::fromUtf8(codec->toUnicode(QByteArray(buf)).toUtf8());
            emit AppSignal::getInstance()->sgl_system_logger_message("基础 Qt 依赖库打包完成", "#fc9153");
            collectQtDependsFlag = true;
        }
        else
        {
            emit AppSignal::getInstance()->sgl_system_logger_message("打包程序 【linuxdeployqt】 执行失败，请检查 Qt 环境变量", "#dd3737");
        }
        pclose(fp);
    }

    if (!collectQtDependsFlag)
    {
        emit AppSignal::getInstance()->sgl_system_logger_message("基础打包未完成，继续查找其他依赖", "#fc9153");

        std::lock_guard<std::mutex> lock(mMutex);
        mThreadPacking = false;

        // 此处应该报错返回
        // return;
    }

    // 运行 Visual 2019 (或其他版本) dumpbin 程序，没有就提示找不到
    emit AppSignal::getInstance()->sgl_system_logger_message("开始运行 Linux 打包流程 【ldd】", "#fc9153");

    uint32_t length = 1024 * 10;
    char *buffer = new char[length];
    QStringList listDepends;
    QStringList listDependsLoss;

    auto findDepends = [&listDepends, &buffer, &length] (const QString &item)
    {
        FILE *fp = nullptr;
        memset(buffer, 0, length);

        fp = popen(QString("ldd %1").arg(item).toStdString().data(), "r");
        if(fp)
        {
            size_t ret = fread(buffer, 1, length - 1, fp);
            if(ret > 0)
            {
                QTextCodec *codec = QTextCodec::codecForName("GBK");
                QString message = QString::fromUtf8(codec->toUnicode(QByteArray(buffer)).toUtf8());
                QStringList tempList = message.remove("\n").split("\t", Qt::SkipEmptyParts);

                QStringList listSo;
                for (auto &tempSo : tempList)
                {
                    if (!tempSo.contains(".so", Qt::CaseSensitive)) continue;
                    if (!tempSo.contains("=>", Qt::CaseSensitive)) continue;
                    auto tempList = tempSo.split(" ", Qt::SkipEmptyParts);
                    if (tempList.size() == 4) listSo.append(tempList.at(2));
                }

                if (listSo.isEmpty())
                {
                    pclose(fp);
                    emit AppSignal::getInstance()->sgl_system_logger_message(QString("动态库 %1 没有后续依赖").arg(item), "#3f8f54");
                    return;
                }

                for (auto &depend : listSo)
                {
                    if (listDepends.contains(depend)) continue;
                    emit AppSignal::getInstance()->sgl_system_logger_message(QString("找到一个系统依赖库 %1 文件  【序号： %2】").arg(depend.split("/").last(), QString::number(listDepends.size())), "#3f8f54");
                    listDepends.append(depend);
                }

                //qDebug() << "listDepends " << listDepends;
            }
            else
            {
                emit AppSignal::getInstance()->sgl_system_logger_message("打包程序 【ldd】 不存在，请先配置其环境变量", "#dd3737");
            }
        }
        pclose(fp);
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
#else
    Q_UNUSED(path);
#endif
}
