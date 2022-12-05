#include "buttondesigned.h"

#include <QPainter>
#include <QPainterPath>

//test
#include <QDebug>

ButtonDesigned::ButtonDesigned(QWidget *parent) : QPushButton(parent)
{
    setMouseTracking(true);
}

ButtonDesigned::ButtonDesigned(int id, QWidget *parent) : QPushButton(parent), mID(id)
{
    setMouseTracking(true);
}

void ButtonDesigned::setID(int id)
{
    if (id < 0 || id > Button_None) return;
    mID = id;
}

void ButtonDesigned::paintEvent(QPaintEvent *e)
{
    switch (mID) {
    case Button_Min:
        paintMin();
        break;
    case Button_Close:
        paintClose();
        break;
    default:
        paintNone();
        break;
    }

    e->accept();
}

void ButtonDesigned::mousePressEvent(QMouseEvent *event)
{
    mMousePressed = true;
    QPushButton::mousePressEvent(event);
}

void ButtonDesigned::mouseReleaseEvent(QMouseEvent *event)
{
    mMousePressed = false;
    QPushButton::mouseReleaseEvent(event);
}

void ButtonDesigned::paintMin()
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, false);

    QPen pen(QBrush(QColor(80, 80, 80)), 1.0);
    pen.setColor(QColor(80, 80, 80));
    pen.setCapStyle(Qt::RoundCap);
    pen.setJoinStyle(Qt::RoundJoin);

    float width = this->width();
    float height = this->height();

    if (this->underMouse())
    {
        pen.setColor(QColor(252, 145, 83));
        pen.setBrush(QBrush(QColor(252, 145, 83)));
    }

    painter.setPen(pen);

    QPointF point1((width - height / 3.0) / 2.0, height * 0.5);
    QPointF point2((width + height / 3.0) / 2.0, height * 0.5);

    QPainterPath path;
    path.moveTo(point1);
    path.lineTo(point2);

    painter.drawPath(path);
}

void ButtonDesigned::paintClose()
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    QPen pen(QBrush(QColor(80, 80, 80)), 1.2);
    pen.setColor(QColor(80, 80, 80));
    pen.setCapStyle(Qt::RoundCap);
    pen.setJoinStyle(Qt::RoundJoin);

    float width = this->width();
    float height = this->height();

    if (this->underMouse())
    {
        pen.setColor(QColor(252, 145, 83));
        pen.setBrush(QBrush(QColor(252, 145, 83)));
    }

    painter.setPen(pen);

    QPointF point1((width - height / 3.0) / 2.0, height / 3.0);
    QPointF point2((width + height / 3.0) / 2.0, height / 3.0 * 2);
    QPointF point3((width + height / 3.0) / 2.0, height / 3.0);
    QPointF point4((width - height / 3.0) / 2.0, height / 3.0 * 2);

    QPainterPath path;
    path.moveTo(point1);
    path.lineTo(point2);
    path.moveTo(point3);
    path.lineTo(point4);

    painter.drawPath(path);
}

void ButtonDesigned::paintNone()
{
    return;
}
