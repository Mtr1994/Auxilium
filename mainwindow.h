#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QCloseEvent>

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
    void on_btnPack_clicked();

    void on_btnSelect_clicked();

    void on_btnManual_clicked();

    void on_btnReaded_clicked();

private:
    Ui::MainWindow *ui;
};
#endif // MAINWINDOW_H
