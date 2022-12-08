#include "linuxpacker.h"
#include "Public/appsignal.h"

#include <QFile>
#include <QFileInfo>

LinuxPacker::LinuxPacker(QObject *parent)
    : QObject{parent}
{

}

void LinuxPacker::pack(const QString &path, bool isWidget, bool isSimpleMode)
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

    mIsSimpleMode = isSimpleMode;
    mIsQtWidgetType = isWidget;

    auto func = std::bind(&LinuxPacker::threadPack, this, path);
    std::thread th(func);
    th.detach();
}

void LinuxPacker::threadPack(const QString &path)
{
#ifdef Q_OS_LINUX
    // 文件详细信息
    QFileInfo fileInfo(path);

    emit AppSignal::getInstance()->sgl_system_logger_message("开始运行 Qt 打包流程 【windeployqt】", "#fc9153");

    bool collectQtDependsFlag = false;

    // 先运行系统的 windeployqt 程序，没有就提示找不到程序
    FILE *fp = nullptr;
    char buf[1024] = { 0 };
#else
    Q_UNUSED(path);
    emit AppSignal::getInstance()->sgl_system_logger_message("系统类型不匹配，请联系管理员", "#dd3737");
#endif
}
