#ifndef BUTTONDESIGN_H
#define BUTTONDESIGN_H

#include <QPushButton>
#include <QPaintEvent>
#include <QMouseEvent>

class ButtonDesigned : public QPushButton
{
    Q_OBJECT
public:
    enum {Button_Min, Button_Close, Button_None};
    explicit ButtonDesigned(QWidget *parent = nullptr);
    explicit ButtonDesigned(int id, QWidget *parent = nullptr);

    void setID(int id);

protected:
    void paintEvent(QPaintEvent *e) override;

    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

private:
    void paintMin();
    void paintClose();

    // 什么都不显示
    void paintNone();

private:
    uint8_t mID;

    bool mMousePressed = false;
};

#endif // BUTTONDESIGN_H
