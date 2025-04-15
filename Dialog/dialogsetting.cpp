#include "dialogsetting.h"
#include "ui_dialogsetting.h"
#include "Public/appconfig.h"

#include <QGraphicsDropShadowEffect>

// test
#include <QDebug>

DialogSetting::DialogSetting(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::DialogSetting)
{
    ui->setupUi(this);

    init();
}

DialogSetting::~DialogSetting()
{
    delete ui;
}

void DialogSetting::init()
{
    setWindowTitle("动态库检索路径管理");

    setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);
    setAttribute(Qt::WA_TranslucentBackground);

    ui->btnMin->setID(ButtonDesigned::Button_Min);
    ui->btnClose->setID(ButtonDesigned::Button_Close);

    connect(ui->btnMin, &QPushButton::clicked, this, [this]{ showMinimized(); });
    connect(ui->btnClose, &QPushButton::clicked, this, [this] { close(); });

    connect(ui->btnCancel, &QPushButton::clicked, this, &DialogSetting::slot_btn_cancel_click);
    connect(ui->btnOk, &QPushButton::clicked, this, &DialogSetting::slot_btn_ok_click);

    connect(ui->btnAddItem, &QPushButton::clicked, this, &DialogSetting::slot_btn_add_click);
    connect(ui->btnDelItem, &QPushButton::clicked, this, &DialogSetting::slot_btn_remove_click);

    ui->lvTargetPath->setModel(&mModelPath);

    // 读取配置文件
    QStringList list = AppConfig::getInstance()->getValue("SearchPath", "value").split(";", Qt::SkipEmptyParts);
    for (auto &item : list)
    {
        mModelPath.appendRow(new QStandardItem(item.trimmed()));
    }
    ui->lvTargetPath->setTextElideMode(Qt::ElideMiddle);
    ui->lvTargetPath->setCurrentIndex(QModelIndex());

    // 这个阴影通过父子关系西东删除，不需要手动删除
    QGraphicsDropShadowEffect *shadowEffect = new QGraphicsDropShadowEffect(ui->widgetSettingBase);
    shadowEffect->setOffset(0, 2);
    shadowEffect->setColor(QColor(214, 214, 214));
    shadowEffect->setBlurRadius(12);
    ui->widgetSettingBase->setGraphicsEffect(shadowEffect);

    ui->widgetSettingBase->setAttribute(Qt::WA_TranslucentBackground);
}

void DialogSetting::mousePressEvent(QMouseEvent *event)
{
    if (event->pos().x() > ui->widgetSettingTitle->width() + ui->widgetSettingTitle->x()
            || event->pos().y() > ui->widgetSettingTitle->height() + this->layout()->contentsMargins().top() + ui->widgetSettingTitle->y()) return;
    if (event->button() == Qt::LeftButton)
    {
        mLastMousePosition = event->globalPosition();
        mMousePressed = true;
    }
}

void DialogSetting::mouseReleaseEvent(QMouseEvent *event)
{
    mMousePressed = false;
    event->accept();
}

void DialogSetting::mouseMoveEvent(QMouseEvent *event)
{
    if (!mMousePressed) return;
    if (!event->buttons().testFlag(Qt::LeftButton)) return;
    const QPointF position = pos() + event->globalPosition() - mLastMousePosition;
    move(position.x(), position.y());
    mLastMousePosition = event->globalPosition();
}

void DialogSetting::slot_btn_cancel_click()
{
    done(0);
}

void DialogSetting::slot_btn_ok_click()
{
    // 保存配置文件
    int size = mModelPath.rowCount();

    QStringList listPaths;
    for (int i = 0; i < size; i++)
    {
        QStandardItem *item = mModelPath.item(i, 0);
        if (nullptr == item) continue;

        if (listPaths.contains(item->text().trimmed())) continue;
        listPaths.append(item->text().trimmed());
    }

    AppConfig::getInstance()->setValue("SearchPath", "value", listPaths.join(";"));
    done(1);
}

void DialogSetting::slot_btn_add_click()
{
    mModelPath.appendRow(new QStandardItem("新建路径 （ 双击文本进行编辑操作 ）"));
}

void DialogSetting::slot_btn_remove_click()
{
    if(!ui->lvTargetPath->selectionModel()->hasSelection()) return;

    QModelIndex index = ui->lvTargetPath->currentIndex();
    mModelPath.removeRow(index.row());
    ui->lvTargetPath->setCurrentIndex(QModelIndex());
}
