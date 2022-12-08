#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "Public/appsignal.h"
#include "Dialog/dialogsetting.h"

#include <QDir>
#include <QDateTime>
#include <QFileDialog>
#include <QStandardPaths>
#include <QListView>

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
    setWindowFlags(Qt::Window | Qt::FramelessWindowHint);
    setAttribute(Qt::WA_TranslucentBackground);

    ui->btnMin->setID(ButtonDesigned::Button_Min);
    ui->btnClose->setID(ButtonDesigned::Button_Close);

    connect(ui->btnMin, &QPushButton::clicked, this, [this]{ showMinimized(); });
    connect(ui->btnClose, &QPushButton::clicked, this, [this] { this->close(); });

    connect(ui->btnSelectDir, &QPushButton::clicked, this, &MainWindow::slot_btn_select_exec_click);
    connect(ui->btnSystemSet, &QPushButton::clicked, this, &MainWindow::slot_btn_setting_click);
    connect(ui->btnStartSearch, &QPushButton::clicked, this, &MainWindow::slot_btn_start_search_click);
    connect(ui->tbLogs, &QTextEdit::customContextMenuRequested, this, &MainWindow::slot_tb_logs_custom_context_menu_requested);

    connect(AppSignal::getInstance(), &AppSignal::sgl_system_logger_message, this, &MainWindow::slot_system_logger_message);

    ui->cbbSearchMode->addItem("简洁模式");
    ui->cbbSearchMode->addItem("循环模式");
    ui->cbbSearchMode->setView(new QListView());
    ui->cbbSearchMode->view()->parentWidget()->setWindowFlag(Qt::NoDropShadowWindowHint);

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
    bool isWidget = ui->cbbClientType->currentIndex() == 0;
    bool isSimpleMode = ui->cbbSearchMode->currentIndex() == 0;

#ifdef Q_OS_LINUX
    mLinuxPacker.pack(ui->tbRootDir->text().trimmed(), isWidget, isSimpleMode);
#elif defined Q_OS_WINDOWS
    mWindowsPacker.pack(ui->tbRootDir->text().trimmed(), isWidget, isSimpleMode);
#endif
}

void MainWindow::slot_btn_select_exec_click()
{
    QString desk = QStandardPaths::writableLocation(QStandardPaths::DesktopLocation);
    QString dir = QFileDialog::getOpenFileName(this, tr("选择文件夹"), desk, "可执行文件 (*.exe)");
    if (dir.isEmpty()) return;

    ui->tbRootDir->setText(dir);
}

void MainWindow::slot_btn_setting_click()
{
    DialogSetting dialog(this);
    dialog.exec();
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

