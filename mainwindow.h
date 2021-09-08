#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QCloseEvent>
#include <QProcess>

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

signals:
    void sgl_thread_search_finish(const QStringList& list);

protected:
    void closeEvent(QCloseEvent *event) override;

private:
    void finish(const std::string& msg);

private slots:
    void slot_Pack_clicked();

    void slot_Select_clicked();

    void slot_Manual_clicked();

    void slot_Readed_clicked();

    void slot_copy_library();

    void slot_thread_search_finish(const QStringList& list);

private:
    void search(const QStringList &listDir, const QStringList &listDll);

private:
    Ui::MainWindow *ui;

    // 可执行文件的绝对路径
    QString mExecPath;

    // 用于执行 dumpbin 程序
    QProcess mProcessExec;

};
#endif // MAINWINDOW_H
