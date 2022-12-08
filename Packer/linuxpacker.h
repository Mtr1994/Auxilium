#ifndef LINUXPACKER_H
#define LINUXPACKER_H

#include <QObject>
#include <mutex>

class LinuxPacker : public QObject
{
    Q_OBJECT
public:
    explicit LinuxPacker(QObject *parent = nullptr);

    void pack(const QString &path, bool isWidget, bool isSimpleMode);

private:
    void threadPack(const QString &path);

private:
    // 是否正在打包
    bool mThreadPacking = false;

    // 线程锁
    std::mutex mMutex;

    // 模式
    bool mIsSimpleMode = true;

    // 程序类型
    bool mIsQtWidgetType = true;

};

#endif // LINUXPACKER_H
