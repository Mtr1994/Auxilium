#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <thread>
#include <QFile>
#include <QFileDialog>
#include <QScreen>
#include <QFileInfo>
#include <QStandardPaths>
#include <regex>

using namespace std;

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

    connect(ui->btnPack, &QPushButton::clicked, this, &MainWindow::slot_Pack_clicked);
    connect(ui->btnSelect, &QPushButton::clicked, this, &MainWindow::slot_Select_clicked);
    connect(ui->btnManual, &QPushButton::clicked, this, &MainWindow::slot_Manual_clicked);
    connect(ui->btnReaded, &QPushButton::clicked, this, &MainWindow::slot_Readed_clicked);

    connect(&mDependentsWalker, &DependentsWalker::sgl_thread_parse_message, this, &MainWindow::slot_thread_parse_message, Qt::QueuedConnection);

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

void MainWindow::slot_Pack_clicked()
{
    ui->statusbar->showMessage("正在检索依赖项...", -1);

    QString path = ui->tbFileName->text();
    QFile file(path);
    if (!file.exists())
    {
        ui->statusbar->showMessage("可执行文件不存在");
        return;
    }

    ui->statusbar->clearMessage();

    QString currentPath = QApplication::applicationDirPath();

#ifdef Q_OS_WINDOWS
    // 官方工具调用
    QProcess deployProcess;
    QString cmd1 = currentPath + "/../tools/windeployqt.exe";
    QStringList para1 = {"--no-translations", "--verbose", "0", path};

    deployProcess.startDetached(cmd1, para1);
#endif

    mDependentsWalker.parse(path);
}

void MainWindow::slot_Select_clicked()
{
    QString desktop = QStandardPaths::writableLocation(QStandardPaths::DesktopLocation);
#ifdef Q_OS_WINDOWS
    QString path = QFileDialog::getOpenFileName(this, "选择可执行文件", desktop, "可执行文件 (*.exe)");
#endif

#ifdef unix
    QString path = QFileDialog::getOpenFileName(this, "选择可执行文件", desktop);
#endif
    if (path.isEmpty()) return;

    ui->tbFileName->setText(path);
}

void MainWindow::slot_Manual_clicked()
{
    ui->widget->setVisible(false);
    ui->widgetManual->setVisible(true);
}

void MainWindow::slot_Readed_clicked()
{
    ui->widgetManual->setVisible(false);
    ui->widget->setVisible(true);
}

void MainWindow::slot_thread_parse_message(const QString &title, const QString &msg)
{
    if (title == "info")
    {
        ui->textMessage->clear();
        int missingCount = 0;
        QStringList list = msg.split("\r\n");
        for (auto& info : list)
        {
            QString msg = info.midRef(1, -1).toString();
            if (info.startsWith("0"))
            {
                ui->textMessage->append("<font color='green'>" + msg + "    拷贝完成");
            }
            else if (info.startsWith("1"))
            {
                missingCount++;
                ui->textMessage->append("<font color='orange'>" + msg + "    拷贝失败");
            }
            else if (info.startsWith("3"))
            {
                ui->textMessage->append("<font color='green'>" + msg + "    已存在");
            }
            else
            {
                missingCount++;
                ui->textMessage->append("<font color='red'>" + msg + "    未找到");
            }
        }

        if(missingCount == 0)
        {
            ui->statusbar->setStyleSheet("color: green;");
            ui->statusbar->showMessage("依赖库拷贝完成", 5000);
        }
        else
        {
            ui->statusbar->setStyleSheet("color: red;");
            ui->statusbar->showMessage("依赖库拷贝完成，存在未找到依赖，请查看详细信息", 10000);
        }
    }
    else
    {
        ui->statusbar->showMessage(msg);
    }
}
