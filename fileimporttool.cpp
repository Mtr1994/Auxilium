#include "fileimporttool.h"

#include <fstream>
#include <string>
#include <QString>
#include <QFile>
#include <QDir>

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
        }
        else if (list.at(2).contains("platforms"))
        {
            QDir dir(newPath + "platforms/");
            if (!dir.exists())
            {
                if(!dir.mkdir(newPath + "platforms/")) continue;
            }
            QFile::copy(list.at(2), newPath + "platforms/" + libName);
        }
        else if (list.at(2).contains("styles"))
        {
            QDir dir(newPath + "styles/");
            if (!dir.exists())
            {
                if(!dir.mkdir(newPath + "styles/")) continue;
            }
            QFile::copy(list.at(2), newPath + "styles/" + libName);
        }
        else if (list.at(2).contains("translations"))
        {
            QDir dir(newPath + "translations/");
            if (!dir.exists())
            {
                if(!dir.mkdir(newPath + "translations/")) continue;
            }
            QFile::copy(list.at(2), newPath + "translations/" + libName);
        }
        else if (list.at(2).indexOf("C:\\Windows\\System32", 0, Qt::CaseInsensitive) >= 0 && !copysystemdll)
        {
            // 系统的库
            continue;
        }
        else
        {
            // 第三方库
            QFile::copy(list.at(2), newPath + libName);
        }
    }
    file.close();
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
#endif

    func("依赖库收集完成");
}
