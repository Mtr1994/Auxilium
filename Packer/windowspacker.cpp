#include "windowspacker.h"
#include "Public/appsignal.h"
#include "Public/appconfig.h"

#include <thread>
#include <QFile>

#if QT_VERSION <= 0x050000
#include <QTextCodec>
#endif

#include <QFileInfo>
#include <regex>
#include <string>
#include <QDir>
#include <fstream>

#ifdef Q_OS_WINDOWS
#include <stdio.h>
#include <Windows.h>
#include <tchar.h>
#endif

using namespace std;

// test
#include <QDebug>

WindowsPacker::WindowsPacker(QObject *parent)
    : QObject{parent}
{

}

void WindowsPacker::pack(const QString &path, bool isWidget, int mode, const QString &sourceroot)
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

    mPackMode = mode;
    mIsQtWidgetType = isWidget;
    mSourceRoot = sourceroot;

    auto func = std::bind(&WindowsPacker::threadPack, this, path);
    std::thread th(func);
    th.detach();
}

void WindowsPacker::threadPack(const QString &path)
{
#ifdef Q_OS_WINDOWS
    // 文件详细信息
    QFileInfo fileInfo(path);

    emit AppSignal::getInstance()->sgl_system_logger_message("开始运行 Qt 打包流程 【windeployqt】", "#fc9153");

    bool collectQtDependsFlag = false;

    FILE *fp = nullptr;
    char buf[1024] = { 0 };
    QString qmlDir;

    AllocConsole();
    ShowWindow(GetConsoleWindow(), SW_HIDE);

    if (!mIsQtWidgetType)
    {
        // 先查找 Qt 的 qml 模块位置，没有就提示找不到程序
        fp = _popen(QString("where windeployqt").toStdString().data(), "r");

        if (fp)
        {
            size_t ret = fread(buf, 1, sizeof(buf) - 1, fp);
            if(ret > 0)
            {
                #if QT_VERSION <= 0x050000
                    QTextCodec *codec = QTextCodec::codecForName("GBK");
                    QString message = QString::fromUtf8(codec->toUnicode(QByteArray(buf)).toUtf8());
                    qmlDir = QFileInfo(message.remove("\n")).absolutePath() + "/../qml";
                #else
                    QString message = QByteArray(buf);
                    qmlDir = QFileInfo(message.remove("\n")).absolutePath() + "/../qml";
                #endif

            }
            else
            {
                emit AppSignal::getInstance()->sgl_system_logger_message("找不到 QML 模块位置，请先配置 Qt 环境变量", "#dd3737");
            }
            _pclose(fp);
        }

        if (qmlDir.isEmpty()) return;

        // qml 程序需要解析源码，获取模块信息，耗时更多
        emit AppSignal::getInstance()->sgl_system_logger_message("正在查找 QML 模块依赖，可能会持续一段时间，请等待", "#fc9153");
    }

    // qmldir 是程序源码的根目录，根据源码里面引入的内容，确定要打包哪些模块
    // qmlimport 是 qml 系统模块的主目录
    fp = _popen(QString("windeployqt %1 %2").arg(
                    fileInfo.absoluteFilePath(),
                    QString(mIsQtWidgetType ? " " : ("--qmldir " + mSourceRoot + " --qmlimport " + qmlDir))).toLocal8Bit().toStdString().data(), "r");
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
            emit AppSignal::getInstance()->sgl_system_logger_message("打包程序 【windeployqt】 执行失败，请检查 Qt 环境变量", "#dd3737");
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
    QStringList list = AppConfig::getInstance()->getValue("SearchPath", "value").split(";", Qt::SkipEmptyParts);
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

    auto findDepends = [&listDepends, &listSystemPath, &listDependsLoss, &buffer, &length, this] (const QString &item)
    {
        FILE *fp = nullptr;
        memset(buffer, 0, length);

        QString path = item;
        if (item.contains(" ")) path = QString("\"%1\"").arg(item);
        fp = _popen(QString("dumpbin /DEPENDENTS %1").arg(path).toLocal8Bit(), "r");
        if(fp)
        {
            size_t ret = fread(buffer, 1, length - 1, fp);
            if(ret > 0)
            {
                #if QT_VERSION <= 0x050000
                    QTextCodec *codec = QTextCodec::codecForName("GBK");
                    QString message = QString::fromUtf8(codec->toUnicode(QByteArray(buffer)).toUtf8());
                #else
                    QString message = QByteArray(buffer);
                #endif

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

                int dependsNumber = 0;

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

                            if (mPackMode == 1)
                            {
                                QString productName;
                                getFileInfoString(absoluteFilePath, "productName", productName);
                                if (productName.contains("Operating System")) break;;
                            }

                            dependsNumber++;
                            emit AppSignal::getInstance()->sgl_system_logger_message(QString("找到一个系统依赖库 %1 文件  【序号： %2】").arg(dll, QString::number(listDepends.size())), "#3f8f54");
                            listDepends.append(absoluteFilePath); break;
                        }
                    }

                    if (!flag)
                    {
                        listDependsLoss.append(dll);
                        emit AppSignal::getInstance()->sgl_system_logger_message(QString("系统没有找到有效 %1 的依赖 %2 文件").arg(item, dll), "#dd3737");
                    }
                }

                if (dependsNumber == 0)
                {
                    emit AppSignal::getInstance()->sgl_system_logger_message(QString("动态库依赖均为 Windows 系统库，程序已放弃"), "#3f8f54");
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

    // 循环查询
    int32_t index = 0;
    while (index < listDepends.size())
    {
        QString item = listDepends.at(index);
        emit AppSignal::getInstance()->sgl_system_logger_message(QString("开始查询 %1 的所有依赖").arg(item), "#fc9153");
        findDepends(item);
        index++;
    }
    FreeConsole();

    delete [] buffer;

    emit AppSignal::getInstance()->sgl_system_logger_message(QString("当前已经尽可能的确定所有依赖库位置，准备开始拷贝共 %1 个动态库文件").arg(QString::number(listDepends.size())), "#3f8f54");

    // 拷贝结果保存
    QStringList listCopyMessage;

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

            listCopyMessage.append(QString("%1 已存在，跳过").arg(info.absoluteFilePath()));
            continue;
        }

        bool status = QFile::copy(listDepends.at(i), QString("%1/%2").arg(fileInfo.absolutePath(), info.fileName()));
        emit AppSignal::getInstance()->sgl_system_logger_message(QString("文件 %1 拷贝 %2").arg(info.fileName(), status ? "成功" : "失败"), status ? "#3f8f54" : "dd3737");

        listCopyMessage.append(QString("%1 %2").arg(info.absoluteFilePath(), status ? "成功" : "失败"));
    }
    // 此处文件拷贝结束

    emit AppSignal::getInstance()->sgl_system_logger_message(QString("系统依赖库收集程序完成；成功【%1 个】，跳过【%2 个】，失败【%3 个】；祝你好运 ~").arg(
                                                                 QString::number(listDepends.size()),
                                                                 QString::number(ignoreNumber),
                                                                 QString::number(listDependsLoss.size())), "#3f8f54");

    // 拷贝过程文件记录
    ofstream fileDescripter;
    fileDescripter.open("copylogs.txt");

    if (fileDescripter.is_open())
    {
        QString content = listCopyMessage.join("\n");
        fileDescripter << content.toStdString().data() << endl;
    }

    std::lock_guard<std::mutex> lock(mMutex);
    mThreadPacking = false;
#elif defined Q_OS_LINUX
    Q_UNUSED(path);
#endif
}

bool WindowsPacker::getFileInfoString(const QString &fileName, const QString &name, QString &value)
{
#ifdef Q_OS_WINDOWS
    TCHAR ptszStr[1024];

    LPCWSTR fileName_wstr = reinterpret_cast<LPCWSTR>(fileName.data());
    LPCWSTR name_wstr = reinterpret_cast<LPCWSTR>(name.data());

    DWORD   dwDummyHandle = 0; // will always be set to zero
    DWORD   dwLen = 0;
    BYTE    *pVersionInfo = NULL;
    BOOL    bRetVal;

    VS_FIXEDFILEINFO    FileVersion;

    HMODULE        hVerDll;
    hVerDll = LoadLibrary((L"VERSION.dll"));
    if (hVerDll == NULL) return FALSE;

#ifdef _UNICODE
    typedef DWORD(WINAPI * Fun_GetFileVersionInfoSize)(LPCTSTR, DWORD *);
    typedef BOOL(WINAPI * Fun_GetFileVersionInfo)(LPCTSTR, DWORD, DWORD, LPVOID);
    typedef BOOL(WINAPI * Fun_VerQueryValue)(LPCVOID, LPCTSTR, LPVOID, PUINT);
#else
    typedef DWORD(WINAPI * Fun_GetFileVersionInfoSize)(LPCSTR, DWORD *);
    typedef BOOL(WINAPI * Fun_GetFileVersionInfo)(LPCSTR, DWORD, DWORD, LPVOID);
    typedef BOOL(WINAPI * Fun_VerQueryValue)(LPCVOID, LPCSTR, LPVOID, PUINT);
#endif

    Fun_GetFileVersionInfoSize        pGetFileVersionInfoSize;
    Fun_GetFileVersionInfo            pGetFileVersionInfo;
    Fun_VerQueryValue                pVerQueryValue;

#ifdef _UNICODE
    pGetFileVersionInfoSize = (Fun_GetFileVersionInfoSize)::GetProcAddress(hVerDll, "GetFileVersionInfoSizeW");
    pGetFileVersionInfo = (Fun_GetFileVersionInfo)::GetProcAddress(hVerDll, "GetFileVersionInfoW");
    pVerQueryValue = (Fun_VerQueryValue)::GetProcAddress(hVerDll, "VerQueryValueW");
#else
    pGetFileVersionInfoSize = (Fun_GetFileVersionInfoSize)::GetProcAddress(hVerDll, "GetFileVersionInfoSizeA");
    pGetFileVersionInfo = (Fun_GetFileVersionInfo)::GetProcAddress(hVerDll, "GetFileVersionInfoA");
    pVerQueryValue = (Fun_VerQueryValue)::GetProcAddress(hVerDll, "VerQueryValueA");
#endif

    struct TRANSLATION
    {
        WORD langID;            // language ID
        WORD charset;            // character set (code page)
    } Translation;

    Translation.langID = 0x0409;    //
    Translation.charset = 1252;        // default = ANSI code page

    dwLen = pGetFileVersionInfoSize(fileName_wstr, &dwDummyHandle);
    if (dwLen == 0) return false;

    pVersionInfo = new BYTE[dwLen]; // allocate version info
    bRetVal = pGetFileVersionInfo(fileName_wstr, 0, dwLen, pVersionInfo);
    if (bRetVal == FALSE) return false;

    VOID    *pVI;
    UINT    uLen;

    bRetVal = pVerQueryValue(pVersionInfo, (L"\\"), &pVI, &uLen);
    if (bRetVal == FALSE) return false;

    memcpy(&FileVersion, pVI, sizeof(VS_FIXEDFILEINFO));

    bRetVal = pVerQueryValue(pVersionInfo, (L"\\VarFileInfo\\Translation"),
                             &pVI, &uLen);
    if (bRetVal && uLen >= 4)
    {
        memcpy(&Translation, pVI, sizeof(TRANSLATION));
    }
    else
    {
        return false;
    }

    //  BREAKIF(FileVersion.dwSignature != VS_FFI_SIGNATURE);
    if (FileVersion.dwSignature != VS_FFI_SIGNATURE)
    {
        return false;
    }

    VOID        *pVal;
    UINT        iLenVal;

    TCHAR    szQuery[1024];
    _stprintf_s(szQuery, 1024, (L"\\StringFileInfo\\%04X%04X\\%s"), Translation.langID, Translation.charset, name_wstr);

    bRetVal = pVerQueryValue(pVersionInfo, szQuery, &pVal, &iLenVal);
    if (bRetVal)
    {
        _stprintf_s(ptszStr, 1024, _T("%s"), (TCHAR *)pVal);
    }
    else
    {
        _stprintf_s(ptszStr, 1024, _T("%s"), _T(""));
    }

    value = QString::fromWCharArray(ptszStr);

#elif defined Q_OS_LINUX
    Q_UNUSED(fileName); Q_UNUSED(name); Q_UNUSED(value);
#endif
    return true;
}
