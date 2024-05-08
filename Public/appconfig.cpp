#include "appconfig.h"

#if QT_VERSION <= 0x050000
#include <QTextCodec>
#endif

#include <QFile>

AppConfig::AppConfig(QObject *parent) : QObject(parent)
{

}

void AppConfig::init()
{
    if (!QFile::exists("conf.ini"))
    {
        QFile file("conf.ini");
        if (!file.open(QIODevice::ReadWrite | QIODevice::Text)) return;
    }

    mSetting = new QSettings("conf.ini", QSettings::IniFormat);
    #if QT_VERSION <= 0x050000
    mSetting->setIniCodec(QTextCodec::codecForName("utf-8"));
    #endif
}

AppConfig *AppConfig::getInstance()
{
    static AppConfig config;
    return &config;
}

QString AppConfig::getValue(const QString& entry, const QString& item)
{
    if (nullptr == mSetting) return "";

    QString value = mSetting->value(entry + "/" + item).toString();
    return value;
}

void AppConfig::setValue(const QString& pEntry, const QString& pItem, const QString& pValue)
{
    if (nullptr == mSetting) return;
    mSetting->setValue(pEntry + "/" + pItem, pValue);
}
