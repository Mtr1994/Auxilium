#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "Packer/windowspacker.h"

#include <QMainWindow>
#include <QMouseEvent>
#include <QTimer>
#include <QMovie>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    void init();

protected:
    void mousePressEvent(QMouseEvent* event);
    void mouseReleaseEvent(QMouseEvent* event);
    void mouseMoveEvent(QMouseEvent* event);

    void slot_btn_select_exec_click();

    void slot_btn_widget_pack_click();

    void slot_btn_quick_pack_click();

    void slot_btn_setting_click();

private slots:
    void slot_system_logger_message(const QString &msg, const QString &color = "");

private:
    Ui::MainWindow *ui;

    QPointF mLastMousePosition;
    bool mMousePressed = false;

    // Windows 打包对象
    WindowsPacker mWindowsPacker;

};
#endif // MAINWINDOW_H
