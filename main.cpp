#include "mainwindow.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWindow w;
    w.show();

    // 加载样式
    qApp->setStyleSheet("file:///:/style/style.qss");

    return a.exec();
}
