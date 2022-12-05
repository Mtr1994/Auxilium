#ifndef WINDOWSPACKER_H
#define WINDOWSPACKER_H

#include <QObject>
#include <mutex>

class WindowsPacker : public QObject
{
    Q_OBJECT
public:
    explicit WindowsPacker(QObject *parent = nullptr);

    void pack(const QString &path);

private:
    void threadPack(const QString &path);

private:
    // 是否正在打包
    bool mThreadPacking = false;

    // 线程锁
    std::mutex mMutex;
};

#endif // WINDOWSPACKER_H
