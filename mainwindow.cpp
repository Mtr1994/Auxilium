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

    connect(&mProcessExec, &QProcess::readyReadStandardOutput, this, &MainWindow::slot_copy_library);
#ifdef Q_OS_WINDOWS
    connect(this, &MainWindow::sgl_thread_search_finish, this, &MainWindow::slot_thread_search_finish, Qt::QueuedConnection);
#endif

#ifdef unix
    connect(this, &MainWindow::sgl_search_finish, this, &MainWindow::slot_thread_search_finish);
#endif

    connect(ui->btnPack, &QPushButton::clicked, this, &MainWindow::slot_Pack_clicked);
    connect(ui->btnSelect, &QPushButton::clicked, this, &MainWindow::slot_Select_clicked);
    connect(ui->btnManual, &QPushButton::clicked, this, &MainWindow::slot_Manual_clicked);
    connect(ui->btnReaded, &QPushButton::clicked, this, &MainWindow::slot_Readed_clicked);

    ui->widgetManual->setVisible(false);

    // test
    ui->tbFileName->setText("/home/mtr/Project/QtPackTool/bin/QtPackTool");
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
        ui->statusbar->showMessage("文件不存在");
        return;
    }

    ui->statusbar->clearMessage();

    QFileInfo info(path);
    mExecPath = info.absoluteDir().absolutePath();

#ifdef unix
    // 系统工具调用
    QString cmd1 = "ldd";
    QStringList para1 = {path};
    mProcessExec.start(cmd1, para1);
#endif

#ifdef Q_OS_WINDOWS
    QString currentPath = QApplication::applicationDirPath();

    // 官方工具调用
    QProcess deployProcess;
    QString cmd1 = currentPath + "/../tools/windeployqt.exe";
    QStringList para1 = {"--no-translations", "--verbose", "0", path};

    deployProcess.startDetached(cmd1, para1);

    QString cmd2 = currentPath + "/../tools/dumpbin/dumpbin.exe";
    QStringList para2 = {"/DEPENDENTS", path};

    mProcessExec.start(cmd2, para2);
#endif
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

void MainWindow::slot_copy_library()
{
    QString data = mProcessExec.readAllStandardOutput();

#ifdef unix
    ui->statusbar->showMessage("开始拷贝依赖项...", -1);
    QStringList output = data.replace('\n', ' ').replace('\t', ' ').split(' ', Qt::SkipEmptyParts);

    uint64_t len = output.length();
    QStringList listMessage;
    for (uint64_t i = 0; i < len - 2; i++)
    {
        if (output.at(i).contains(".so")) // is library
        {
            if (output.at(i + 1) == "=>") // has path
            {
                if (output.at(i + 2).contains(".so")) // what i need
                {
                    QString path = output.at(i + 2);
                    bool status = QFile::copy(path, QString("%1/%2").arg(mExecPath, output.at(i)));
                    if (status) listMessage.append("0" + path);
                    else listMessage.append("1" + path);
                }
            }
        }
    }

    emit sgl_search_finish(listMessage);
#endif

#ifdef Q_OS_WINDOWS

    if (!data.contains("Image has the following dependencies")) return;
    string output = data.remove("\r\n").toStdString();

    regex pattern("(.*?)Image has the following dependencies:(.*)Summary(.*)");
    smatch results;
    if (!std::regex_match(output, results, pattern)) return;
    if (results.size() != 4) return;

    QString librarys = QString::fromStdString(results[2]);

    QStringList listLibrary = librarys.split(" ", Qt::SkipEmptyParts);
    QStringList environment = QProcess::systemEnvironment();
    QStringList systemPathList;
    for(auto &env : environment)
    {
        if (env.startsWith("PATH=", Qt::CaseInsensitive))
        {
            systemPathList = env.split(';');
            break;
        }
    }

    auto func = std::bind(&MainWindow::search, this, std::placeholders::_1, std::placeholders::_2);
    std::thread th(func, systemPathList, listLibrary);
    th.detach();
#endif
}

void MainWindow::slot_thread_search_finish(const QStringList &list)
{
    ui->textMessage->clear();
    int missingCount = 0;
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

#ifdef Q_OS_WINDOWS
void MainWindow::search(const QStringList &listDir, const QStringList &listDll)
{
    ui->statusbar->showMessage("开始拷贝依赖项...", -1);
    QStringList listMessage;
    bool searchFlag = false;

    for (auto &library : listDll)
    {
        searchFlag = false;
        if (QFile::exists(QString("%1/%2").arg(mExecPath, library)))
        {
            searchFlag = true;
            listMessage.append("3" + library);
        }
        else
        {
            for (auto &path : listDir)
            {
                QDir dir(path);
                QStringList fileList = dir.entryList({"*.dll"}, QDir::Files);
                if (fileList.contains(library, Qt::CaseInsensitive))
                {
                    searchFlag = true;
                    bool status = QFile::copy(QString("%1/%2").arg(path, library), QString("%1/%2").arg(mExecPath, library));
                    if (status) listMessage.append("0" + path + "\\" + library);
                    else listMessage.append("1" + path + "\\" + library);
                }

                if (searchFlag) break;
            }
        }

        if (!searchFlag) listMessage.append("2" + library);
    }

    emit sgl_thread_search_finish(listMessage);
}
#endif
