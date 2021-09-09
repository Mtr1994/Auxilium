#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QCloseEvent>
#include <QProcess>
#include <QStringList>

#include "Public/dependentswalker.h"

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
    void closeEvent(QCloseEvent *event) override;

private:
    void finish(const std::string& msg);

private slots:
    void slot_Pack_clicked();

    void slot_Select_clicked();

    void slot_Manual_clicked();

    void slot_Readed_clicked();

    void slot_thread_parse_message(const QString& title, const QString& msg);

private:
    Ui::MainWindow *ui;

    // 工具类
    DependentsWalker mDependentsWalker;

};
#endif // MAINWINDOW_H
