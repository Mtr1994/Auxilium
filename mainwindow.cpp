#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "Public/appsignal.h"
#include "Dialog/dialogsetting.h"
#include "Public/appconfig.h"

#include <QDir>
#include <QDateTime>
#include <QFileDialog>
#include <QStandardPaths>
#include <QListView>
#include <regex>
#include <QScreen>

// test
#include <QDebug>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    init();

    setWindowTitle("Auxilium");
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::init()
{
    float pointSize = AppConfig::getInstance()->getValue("PointSize", "value").toFloat();
    setMinimumSize(pointSize * 100, pointSize * 100 * 0.618);

    setWindowFlags(Qt::Window | Qt::FramelessWindowHint);
    setAttribute(Qt::WA_TranslucentBackground);

    ui->btnMin->setID(ButtonDesigned::Button_Min);
    ui->btnClose->setID(ButtonDesigned::Button_Close);

    connect(ui->btnMin, &QPushButton::clicked, this, [this]{ showMinimized(); });
    connect(ui->btnClose, &QPushButton::clicked, this, [this] { this->close(); });

    connect(ui->btnSelectDir, &QPushButton::clicked, this, &MainWindow::slot_btn_select_exec_click);
    connect(ui->btnSelectSourceDir, &QPushButton::clicked, this, &MainWindow::slot_btn_select_source_dir_click);
    connect(ui->btnSystemSet, &QPushButton::clicked, this, &MainWindow::slot_btn_setting_click);
    connect(ui->btnStartSearch, &QPushButton::clicked, this, &MainWindow::slot_btn_start_search_click);
    connect(ui->tbLogs, &QTextEdit::customContextMenuRequested, this, &MainWindow::slot_tb_logs_custom_context_menu_requested);

    connect(AppSignal::getInstance(), &AppSignal::sgl_system_logger_message, this, &MainWindow::slot_system_logger_message);

    // 改变项目类型时，根据情况现实或隐藏源码输入框
    connect(ui->cbbClientType, &QComboBox::currentTextChanged, this, &MainWindow::slot_current_type_change);

    // 标准模式 循环查找所有库，但是不要系统库，盲目模式也是循环查找，但是附带系统库
    ui->cbbSearchMode->addItem("盲目模式", 0);
    ui->cbbSearchMode->addItem("标准模式", 1);
    ui->cbbSearchMode->setView(new QListView());
    ui->cbbSearchMode->view()->parentWidget()->setWindowFlag(Qt::NoDropShadowWindowHint);

    // 默认标准模式
    ui->cbbSearchMode->setCurrentIndex(1);

    ui->cbbClientType->addItem("Qt Widget");
    ui->cbbClientType->addItem("Qt Quick");
    ui->cbbClientType->setView(new QListView());
    ui->cbbClientType->view()->parentWidget()->setWindowFlag(Qt::NoDropShadowWindowHint);
}

void MainWindow::mousePressEvent(QMouseEvent *event)
{
    if (event->pos().x() > ui->widgetTitle->width() + ui->widgetTitle->x()
            || event->pos().y() > ui->widgetTitle->height() + layout()->contentsMargins().top() + ui->widgetTitle->y()) return;
    if (event->button() == Qt::LeftButton)
    {
        mLastMousePosition = event->globalPos();
        mMousePressed = true;
    }
}

void MainWindow::mouseReleaseEvent(QMouseEvent *event)
{
    mMousePressed = false;
    event->accept();
}

void MainWindow::mouseMoveEvent(QMouseEvent *event)
{
    if (!mMousePressed) return;
    if (!event->buttons().testFlag(Qt::LeftButton)) return;
    const QPointF position = pos() + event->globalPos() - mLastMousePosition; //the position of mainfrmae + (current_mouse_position - last_mouse_position)
    move(position.x(), position.y());
    mLastMousePosition = event->globalPos();
}

void MainWindow::slot_btn_start_search_click()
{
    QString rootPath = ui->tbRootDir->text().trimmed();
    // 确认可执行程序是否存在
    if (!QFile(rootPath).exists())
    {
        emit AppSignal::getInstance()->sgl_system_logger_message("请选择有效的可执行文件路径", "#fc9153");
        return;
    }

    std::string strReg = rootPath.toStdString();
    std::regex reg("[\u4e00-\u9fa5]");
    std::smatch match;
    if(std::regex_search(strReg, match, reg))
    {
        emit AppSignal::getInstance()->sgl_system_logger_message("可执行文件路径中不能存在中文，请检查后重试", "#dd3737");
        return;
    }

    bool isWidget = ui->cbbClientType->currentIndex() == 0;

    // qml 程序需要确认 源码位置是否存在
    QString sourceRootPath = ui->tbSourceDir->text().trimmed();
    if (!isWidget)
    {
        if (sourceRootPath.isEmpty() || !QDir(sourceRootPath).exists())
        {
            emit AppSignal::getInstance()->sgl_system_logger_message("请选择有效的项目源码路径", "#fc9153");
            return;
        }
    }

    int packMode = ui->cbbSearchMode->currentData().toInt();

#ifdef Q_OS_LINUX
    mLinuxPacker.pack(rootPath, isWidget, packMode, sourceRootPath);
#elif defined Q_OS_WINDOWS
    mWindowsPacker.pack(rootPath, isWidget, packMode, sourceRootPath);
#endif
}

void MainWindow::slot_btn_select_exec_click()
{
    QString desk = QStandardPaths::writableLocation(QStandardPaths::DesktopLocation);
#ifdef Q_OS_LINUX
    QString path = QFileDialog::getOpenFileName(this, tr("选择可执行文件"), desk, "可执行文件 (*)");
#elif defined Q_OS_WINDOWS
    QString path = QFileDialog::getOpenFileName(this, tr("选择可执行文件"), desk, "可执行文件 (*.exe)");
#endif
    if (path.isEmpty()) return;

    ui->tbRootDir->setText(path);
}

void MainWindow::slot_btn_setting_click()
{
    DialogSetting dialog(this);
    dialog.setGeometry(this->x() + (this->width() - dialog.width()) * 0.5, this->y() + (this->height() - dialog.height()) * 0.5, dialog.width(), dialog.height());
    dialog.exec();
}

void MainWindow::slot_current_type_change(const QString & name)
{
    ui->widgetSourceBase->setVisible(name.contains("Quick"));
}

void MainWindow::slot_btn_select_source_dir_click()
{
    QString desk = QStandardPaths::writableLocation(QStandardPaths::DesktopLocation);
    QString dir = QFileDialog::getExistingDirectory(this, tr("选择文件夹"), desk);
    if (dir.isEmpty()) return;

    ui->tbSourceDir->setText(dir);
}

void MainWindow::slot_system_logger_message(const QString &msg, const QString &color)
{
    QString styleMessage = msg;
    if (!color.isEmpty())
    {
        styleMessage = QString("&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span style=\"color:%1\">%2</span>").arg(color, msg);
    }
    ui->tbLogs->append(QString("%1:      %2").arg(QDateTime::currentDateTime().toString("hh:mm:ss"), styleMessage));
}

void MainWindow::slot_tb_logs_custom_context_menu_requested(const QPoint &pos)
{
    Q_UNUSED(pos);
    QMenu menu(this);
    menu.setWindowFlags(Qt::NoDropShadowWindowHint | menu.windowFlags() | Qt::FramelessWindowHint);
    menu.setAttribute(Qt::WA_TranslucentBackground);
    QAction actionCopy("复制");
    // 选中才能复制
    connect(&actionCopy, &QAction::triggered, this, [this]() { ui->tbLogs->copy(); });
    QAction actionClear("全部清理");
    connect(&actionClear, &QAction::triggered, this, [this]() { ui->tbLogs->clear(); });

    menu.addAction(&actionCopy);
    menu.addAction(&actionClear);

    menu.exec(QCursor::pos());
}

