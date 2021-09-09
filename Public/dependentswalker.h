#ifndef DEPENDENTSWALKER_H
#define DEPENDENTSWALKER_H

#include <QObject>
#include <QStringList>
#include <mutex>
#include <condition_variable>

#ifdef Q_OS_WINDOWS
#include<Windows.h>
#endif

class DependentsWalker : public QObject
{
    Q_OBJECT
public:
    explicit DependentsWalker(QObject *parent = nullptr);

    void parse(const QString &path);

signals:
    void sgl_thread_parse_message(const QString& title, const QString& msg);

private:
#ifdef Q_OS_WINDOWS
    void dumpbin(const QString &path);
    static std::string getFileDescription(LPCWSTR path);
#endif

    void collectDependents(const QString &data);

private:
    // 记录已找到的依赖项
    QStringList mListPath;
    QStringList mListLibrary;

    // 记录未找到的依赖
    QStringList mListMessage;

    int64_t mDependentsIndex = 0;

    //
    std::mutex mMutex;
    std::condition_variable mCV;

    //
    bool mDumpbinFlag = true;

    bool mReachRoot = false;
};

#endif // DEPENDENTSWALKER_H
