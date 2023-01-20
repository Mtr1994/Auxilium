#ifndef LINUXPACKER_H
#define LINUXPACKER_H

#include <QObject>
#include <mutex>

class LinuxPacker : public QObject
{
    Q_OBJECT
public:
    explicit LinuxPacker(QObject *parent = nullptr);

    void pack(const QString &path, bool isWidget, int mode, const QString &sourceroot);

private:
    void threadPack(const QString &path);

private:
    // 是否正在打包
    bool mThreadPacking = false;

    // 线程锁
    std::mutex mMutex;

    // 模式
    int mPackMode = 1;

    // 程序类型
    bool mIsQtWidgetType = true;

    // 源码根目录
    QString mSourceRoot;
};

#endif // LINUXPACKER_H
