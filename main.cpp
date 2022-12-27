#include "mainwindow.h"
#include "Public/appconfig.h"

#include <QApplication>
#include <QScreen>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    // 初始化配置文件
    AppConfig::getInstance()->init();

    // 加载样式
#ifdef Q_OS_LINUX
    qApp->setStyleSheet("file:///:/Resource/qss/style_linux.qss");
#elif defined Q_OS_WINDOWS
    qApp->setStyleSheet("file:///:/Resource/qss/style_windows.qss");
#endif

    MainWindow w;
    w.show();

    // 处理因为 QSS 设置主窗口大小导致的程序默认不居中的问题
    QSize screenSize = a.primaryScreen()->availableSize();
    w.move((screenSize.width() - w.width()) / 2, (screenSize.height() - w.height()) / 2);

    return a.exec();
}
