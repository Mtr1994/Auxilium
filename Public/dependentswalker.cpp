#include "dependentswalker.h"

#include <QCoreApplication>
#include <string>
#include <regex>
#include <QFile>
#include <QDir>
#include <QProcess>
#include <thread>

// test
#include <QDebug>

using namespace std;

DependentsWalker::DependentsWalker(QObject *parent) : QObject(parent)
{

}

void DependentsWalker::parse(const QString &path)
{
#ifdef Q_OS_WINDOWS
    auto func = std::bind(&DependentsWalker::dumpbin, this, path);
    std::thread th(func);
    th.detach();
#endif

#ifdef unix
    auto func = std::bind(&DependentsWalker::ldd, this, path);
    std::thread th(func);
    th.detach();
#endif
}

#ifdef unix
void DependentsWalker::ldd(const QString &path)
{
    QProcess *process = new QProcess;
    connect(process, &QProcess::readyReadStandardOutput, process, [this, &process]
    {
        QString data = process->readAllStandardOutput();

        if (!data.contains("=>"))
        {
            mDumpbinFlag = true;
            mReachRoot = true;
            mCV.notify_one();
            return;
        }

        collectDependents(data);
    });

    QString cmd = "ldd";
    while (true)
    {
        std::unique_lock<std::mutex> lock(mMutex);
        mCV.wait(lock, [this]{return mDumpbinFlag;});
        mDumpbinFlag = false;

        QStringList para = {path};
        if (mDependentsIndex < mListPath.size())
        {
            para[0] = mListPath.at(mDependentsIndex);
            mDependentsIndex++;
        }
        else if (mReachRoot && (mDependentsIndex >= mListPath.size()))
        {
            break;
        }

        emit sgl_thread_parse_message("msg", QString("正在检索第 %1 项依赖").arg(QString::number(mDependentsIndex)));

        process->start(cmd, para);
        process->waitForFinished(10000);
    }

    // 开始i文件拷贝
    int64_t size = mListPath.size();
    QFileInfo infoOrigin(path);

    for (int64_t i = 0; i < size; i++)
    {
        QFileInfo info(mListPath.at(i));

        if (QFile::exists(QString("%1/%2").arg(infoOrigin.absolutePath(), info.fileName())))
        {
            mListMessage.append("3" + mListPath.at(i));
            continue;
        }

#ifdef Q_OS_WINDOWS
        //获取产品版本，如果使操作系统的库，不进行拷贝
        QString productName = DependentsWalker::getFileDescription(mListPath.at(i).toStdWString().data()).data();
        if (productName.contains("Operating System")) continue;
#endif
        bool status = QFile::copy(mListPath.at(i), QString("%1/%2").arg(infoOrigin.absolutePath(), info.fileName()));
        if (status)
        {
            mListMessage.append("0" + mListPath.at(i));
        }
        else
        {
            mListMessage.append("1" + mListPath.at(i));
        }
    }

    emit sgl_thread_parse_message("info", mListMessage.join("\r\n"));
}
#endif

#ifdef Q_OS_WINDOWS
void DependentsWalker::dumpbin(const QString &path)
{
    QProcess *process = new QProcess;
    connect(process, &QProcess::readyReadStandardOutput, process, [this, &process]
    {
        QString data = process->readAllStandardOutput();

        if (!data.contains("Image has the following dependencies"))
        {
            mDumpbinFlag = true;
            mReachRoot = true;
            mCV.notify_one();
            return;
        }

        collectDependents(data);
    });

    QString currentPath = QCoreApplication::applicationDirPath();
    QString cmd = currentPath + "/tools/dumpbin/dumpbin.exe";

    while (true)
    {
        std::unique_lock<std::mutex> lock(mMutex);
        mCV.wait(lock, [this]{return mDumpbinFlag;});
        mDumpbinFlag = false;

        QStringList para = {"/DEPENDENTS", path};
        if (mDependentsIndex < mListPath.size())
        {
            para[1] = mListPath.at(mDependentsIndex);
            mDependentsIndex++;
        }
        else if (mReachRoot && (mDependentsIndex >= mListPath.size()))
        {
            break;
        }

        emit sgl_thread_parse_message("msg", QString("正在检索第 %1 项依赖").arg(QString::number(mDependentsIndex)));

        process->start(cmd, para);
        process->waitForFinished(10000);
    }

    // 开始i文件拷贝
    int64_t size = mListPath.size();
    QFileInfo infoOrigin(path);

    for (int64_t i = 0; i < size; i++)
    {
        QFileInfo info(mListPath.at(i));

        if (QFile::exists(QString("%1/%2").arg(infoOrigin.absolutePath(), info.fileName())))
        {
            mListMessage.append("3" + mListPath.at(i));
            continue;
        }

        //获取产品版本，如果使操作系统的库，不进行拷贝
        QString productName = DependentsWalker::getFileDescription(mListPath.at(i).toStdWString().data()).data();
        if (productName.contains("Operating System")) continue;

        bool status = QFile::copy(mListPath.at(i), QString("%1/%2").arg(infoOrigin.absolutePath(), info.fileName()));
        if (status)
        {
            mListMessage.append("0" + mListPath.at(i));
        }
        else
        {
            mListMessage.append("1" + mListPath.at(i));
        }
    }

    emit sgl_thread_parse_message("info", mListMessage.join("\r\n"));
}
#endif

void DependentsWalker::collectDependents(const QString& data)
{
#ifdef unix
    QString tmpData = data;
    QStringList output = tmpData.replace('\n', ' ').replace('\t', ' ').split(' ', Qt::SkipEmptyParts);

    uint64_t len = output.length();
    QStringList listMessage;
    for (uint64_t i = 0; i < len - 2; i++)
    {
        if (output.at(i).contains(".so")) // this is a library
        {
            if (mListLibrary.contains(output.at(i))) continue;
            mListLibrary.append(output.at(i));

            if (QFile::exists(output.at(i)))
            {
                listMessage.append("3" + output.at(i));
                continue;
            }

            if (output.at(i + 1) == "=>") // has path
            {
                if (output.at(i + 2).contains(".so")) // what i need, the path of library
                {
                    QString path = output.at(i + 2);
                    mListPath.append(path);
                    mReachRoot = false;
                }
            }
        }
    }

    mDumpbinFlag = true;
    mCV.notify_one();
#endif

#ifdef Q_OS_WINDOWS
    QString tmpData = data;
    string output = tmpData.remove("\r\n").remove("Image has the following delay load dependencies:").toStdString();

    regex pattern("(.*?)Image has the following dependencies:(.*)Summary(.*)");
    smatch results;
    if (!std::regex_match(output, results, pattern)) return;
    if (results.size() < 4) return;

    QString librarys = QString::fromStdString(results[2]);

    QStringList listLibrary = librarys.split(" ", Qt::SkipEmptyParts);
    if (listLibrary.size() == 0) return;
    QStringList listSystemPath = QProcessEnvironment::systemEnvironment().value("Path").split(';');

    bool searchFlag = false;

    for (auto &library : listLibrary)
    {
        searchFlag = false;

        if (mListLibrary.contains(library)) continue;
        mListLibrary.append(library);

        for (auto &path : listSystemPath)
        {
            QDir dir(path);
            QStringList fileList = dir.entryList({"*.dll"}, QDir::Files);
            if (fileList.contains(library, Qt::CaseInsensitive))
            {
                searchFlag = true;
                mListPath.append(QString("%1/%2").arg(path, library));
                mReachRoot = false;
            }

            if (searchFlag) break;
        }

        if (!searchFlag) mListMessage.append("2" + library);
    }

    mDumpbinFlag = true;
    mCV.notify_one();
#endif
}

#ifdef Q_OS_WINDOWS
// 函数参考网址：https://www.cnblogs.com/yzhuang/p/13754332.html
std::string DependentsWalker::getFileDescription(LPCWSTR path)
{
    std::string description = "";

    //获取版本信息大小
    DWORD dwSize = GetFileVersionInfoSize(path, NULL);
    if (dwSize > 0)
    {
        TCHAR *pBuf = new TCHAR[dwSize + 1];
        memset(pBuf, 0, dwSize + 1);
        //获取版本信息
        GetFileVersionInfo(path, NULL, dwSize, pBuf);

        // Read the list of languages and code pages.
        LPVOID lpBuffer = NULL;
        UINT uLen = 0;

        UINT nQuerySize;
        DWORD* pTransTable;
        ::VerQueryValue(pBuf, TEXT("\\VarFileInfo\\Translation"), (void **)&pTransTable, &nQuerySize);
        DWORD m_dwLangCharset = MAKELONG(HIWORD(pTransTable[0]), LOWORD(pTransTable[0]));

        char* SubBlock = new char[50];
        sprintf_s(SubBlock, 50, "\\StringFileInfo\\%08lx\\ProductName", m_dwLangCharset);

        QString block = QString(SubBlock);
        VerQueryValue(pBuf, LPCWSTR(block.toStdWString().data()), &lpBuffer, &uLen);
        if (uLen) description = QString::fromStdWString((TCHAR*)lpBuffer).toStdString();
        delete[]pBuf;
    }

    return description;
}

#endif
