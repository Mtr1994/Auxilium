#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "fileimporttool.h"

#include <thread>
#include <QFile>
#include <QFileDialog>
#include <QScreen>
#include <QFileInfo>

//test
#include <QDebug>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    init();

    setWindowTitle("程序打包工具 (QT 开发者专用)");
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::init()
{
    QRect rect = QGuiApplication::screens().at(0)->availableGeometry();

    float width = rect.width() * 0.1 < 960 ? 960 : rect.width() * 0.1;
    float height = rect.width() * 0.1 < 640 ? 640 : rect.width() * 0.1;

    resize(width, height);

    ui->widgetManual->setVisible(false);
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    Q_UNUSED(event);
}

void MainWindow::finish(const std::string& msg)
{
    ui->statusbar->showMessage(msg.data());
}

void MainWindow::on_btnPack_clicked()
{
    QString path = ui->tbFileName->text();
    QFile file(path);
    if (!file.exists())
    {
        ui->statusbar->showMessage("文件不存在");
        return;
    }

    ui->statusbar->clearMessage();

    QFileInfo info(file);
    auto func = std::bind(&MainWindow::finish, this, std::placeholders::_1);
    std::thread t1(FileImportTool::import, info.absolutePath().toStdString(), info.fileName().toStdString(), ui->cbbSystemDll->isChecked(), func);
    t1.detach();
}

void MainWindow::on_btnSelect_clicked()
{
    QString path = QFileDialog::getOpenFileName(this, "选择日志文件", "D:/");
    if (path.isEmpty()) return;

    ui->tbFileName->setText(path);
}

void MainWindow::on_btnManual_clicked()
{
    ui->widget->setVisible(false);
    ui->widgetManual->setVisible(true);
}

void MainWindow::on_btnReaded_clicked()
{
    ui->widgetManual->setVisible(false);
    ui->widget->setVisible(true);
}
