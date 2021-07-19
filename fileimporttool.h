#ifndef FILEIMPORTTOOL_H
#define FILEIMPORTTOOL_H

#include <QObject>
#include <functional>

#ifdef Q_OS_WINDOWS
#include<Windows.h>
#endif

class FileImportTool : public QObject
{
    Q_OBJECT
public:
    explicit FileImportTool(QObject *parent = nullptr);

    static void import(std::string path, std::string name, bool copysystemdll, std::function<void(const std::string& msg)> func);

public:
    static std::string getFileDescription(LPCWSTR path);

};

#endif // FILEIMPORTTOOL_H
