#ifndef DIALOGSETTING_H
#define DIALOGSETTING_H

#include <QDialog>
#include <QMouseEvent>

namespace Ui {
class DialogSetting;
}

class QGraphicsDropShadowEffect;
class DialogSetting : public QDialog
{
    Q_OBJECT

public:
    explicit DialogSetting(QWidget *parent = nullptr);
    ~DialogSetting();

    void init();

protected:
    void mousePressEvent(QMouseEvent* event);
    void mouseReleaseEvent(QMouseEvent* event);
    void mouseMoveEvent(QMouseEvent* event);

private:
    Ui::DialogSetting *ui;

    QPointF mLastMousePosition;
    bool mMousePressed = false;

    // 背景阴影
    QGraphicsDropShadowEffect *mShadowEffect = nullptr;
};

#endif // DIALOGSETTING_H
