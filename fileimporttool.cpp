#include "fileimporttool.h"

#include <fstream>
#include <string>
#include <QString>
#include <QFile>
#include <QDir>

#ifdef Q_OS_WINDOWS
#include <fileapi.h>
#include <winver.h>
#include <atlconv.h>
#endif

//test
#include <QDebug>
#include <thread>

using namespace std;

FileImportTool::FileImportTool(QObject *parent) : QObject(parent)
{

}

void FileImportTool::import(std::string path, std::string name, bool copysystemdll, std::function<void(const std::string& msg)> func)
{
#ifdef Q_OS_WINDOWS
    ifstream file(path + "/" + name, ios::in);
    if(!file) return func("文件打开失败");
    string str;
    while (getline(file, str))
    {
        QString line = QString::fromStdString(str);
        QStringList list = line.split(' ');
        if (list.length() != 3) continue;
        if (list.at(0) != "Module" || list.at(1) != "loaded:") continue;
        if (list.at(2).indexOf('\\') < 0) continue;

        QStringList libs = list.at(2).split('\\');
        QString libName = libs.last();

        QString newPath = QString::fromStdString(path) + "/";
        // Qt 的库
        if (list.at(2).contains("iconengines"))
        {
            QDir dir(newPath + "iconengines/");
            if (!dir.exists())
            {
                if(!dir.mkdir(newPath + "iconengines/")) continue;
            }
            QFile::copy(list.at(2), newPath + "iconengines/" + libName);
            continue;
        }
        else if (list.at(2).contains("imageformats"))
        {
            QDir dir(newPath + "imageformats/");
            if (!dir.exists())
            {
                if(!dir.mkdir(newPath + "imageformats/")) continue;
            }
            QFile::copy(list.at(2), newPath + "imageformats/" + libName);
            continue;
        }
        else if (list.at(2).contains("platforms"))
        {
            QDir dir(newPath + "platforms/");
            if (!dir.exists())
            {
                if(!dir.mkdir(newPath + "platforms/")) continue;
            }
            QFile::copy(list.at(2), newPath + "platforms/" + libName);
            continue;
        }
        else if (list.at(2).contains("styles"))
        {
            QDir dir(newPath + "styles/");
            if (!dir.exists())
            {
                if(!dir.mkdir(newPath + "styles/")) continue;
            }
            QFile::copy(list.at(2), newPath + "styles/" + libName);
            continue;
        }
        else if (list.at(2).contains("translations"))
        {
            QDir dir(newPath + "translations/");
            if (!dir.exists())
            {
                if(!dir.mkdir(newPath + "translations/")) continue;
            }
            QFile::copy(list.at(2), newPath + "translations/" + libName);
            continue;
        }
        else if (list.at(2).indexOf("C:\\Windows\\System32", 0, Qt::CaseInsensitive) >= 0)
        {
            // 系统的库
            if (!copysystemdll) continue;       // 不拷贝系统库
            if (list.size() > 5) continue;      // 目录太深了，假设为不重要的库
            if (list.at(2).contains(' ')) continue;   // 跳过带空格的目录
            if (libName.contains("Windows") || libName.contains("windows")) continue; // 不拷贝平台库

            //获取产品版本，如果使操作系统的库，不进行拷贝
            QString productName = FileImportTool::getFileDescription(list.at(2).toStdWString().data()).data();
            if (productName.contains("Operating System")) continue;
        }

        // 第三方库
        QFile::copy(list.at(2), newPath + libName);
    }
    file.close();

    remove(path.data());
#endif

#ifdef unix
    ifstream file(path + "/" + name, ios::in);
    if(!file) return func("日志文件打开失败");
    string str;
    while (getline(file, str))
    {
        QString line = QString::fromStdString(str);
        QStringList list = line.split(' ');
        if (list.length() != 3) continue;
        if (list.at(0) != "Library" || list.at(2) != "loaded.\r") continue;
        if (list.at(1).indexOf('/') < 0) continue;

        QStringList libs = list.at(1).split('/');
        QString libName = libs.last();

        QString newPath = QString::fromStdString(path) + "/";
        // Qt 的库
        if (list.at(1).contains("iconengines"))
        {
            QDir dir(newPath + "iconengines/");
            if (!dir.exists())
            {
                if(!dir.mkdir(newPath + "iconengines/")) continue;
            }
            QFile::copy(list.at(1), newPath + "iconengines/" + libName);
        }
        else if (list.at(1).contains("imageformats"))
        {
            QDir dir(newPath + "imageformats/");
            if (!dir.exists())
            {
                if(!dir.mkdir(newPath + "imageformats/")) continue;
            }
            QFile::copy(list.at(1), newPath + "imageformats/" + libName);
        }
        else if (list.at(1).contains("platforms"))
        {
            QDir dir(newPath + "platforms/");
            if (!dir.exists())
            {
                if(!dir.mkdir(newPath + "platforms/")) continue;
            }
            QFile::copy(list.at(1), newPath + "platforms/" + libName);
        }
        else if (list.at(1).contains("styles"))
        {
            QDir dir(newPath + "styles/");
            if (!dir.exists())
            {
                if(!dir.mkdir(newPath + "styles/")) continue;
            }
            QFile::copy(list.at(1), newPath + "styles/" + libName);
        }
        else if (list.at(1).contains("translations"))
        {
            QDir dir(newPath + "translations/");
            if (!dir.exists())
            {
                if(!dir.mkdir(newPath + "translations/")) continue;
            }
            QFile::copy(list.at(1), newPath + "translations/" + libName);
        }
        else if (list.at(1).indexOf("/lib/x86_64-linux-gnu", 0, Qt::CaseInsensitive) >= 0 && !copysystemdll)
        {
            // 系统的库
            continue;
        }
        else
        {
            // 第三方库
            QFile::copy(list.at(1), newPath + libName);
        }
    }

    file.close();

    remove(path.data());
#endif

    func("依赖库收集完成");
}

#ifdef Q_OS_WINDOWS
// 函数参考网址：https://www.cnblogs.com/yzhuang/p/13754332.html

std::string FileImportTool::getFileDescription(LPCWSTR path)
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
