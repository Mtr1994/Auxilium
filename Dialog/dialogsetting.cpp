#include "dialogsetting.h"
#include "ui_dialogsetting.h"

#include <QGraphicsEffect>

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

    QGraphicsDropShadowEffect* shadow = new QGraphicsDropShadowEffect(this);
    shadow->setOffset(0, 0);
    shadow->setColor(QColor(96, 96, 96));
    shadow->setBlurRadius(12);

    setGraphicsEffect(shadow);
}

void DialogSetting::mousePressEvent(QMouseEvent *event)
{
    if (event->pos().x() > ui->widgetSettingTitle->width() + ui->widgetSettingTitle->x()
            || event->pos().y() > ui->widgetSettingTitle->height() + this->layout()->contentsMargins().top() + ui->widgetSettingTitle->y()) return;
    if (event->button() == Qt::LeftButton)
    {
        mLastMousePosition = event->globalPos();
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
    const QPointF position = pos() + event->globalPos() - mLastMousePosition;
    move(position.x(), position.y());
    mLastMousePosition = event->globalPos();
}
