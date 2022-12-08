#ifndef DIALOGSETTING_H
#define DIALOGSETTING_H

#include <QDialog>
#include <QMouseEvent>
#include <QStandardItemModel>

namespace Ui {
class DialogSetting;
}

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

private slots:
    void slot_btn_cancel_click();

    void slot_btn_ok_click();

    void slot_btn_add_click();

    void slot_btn_remove_click();

private:
    Ui::DialogSetting *ui;

    QPointF mLastMousePosition;
    bool mMousePressed = false;

    // 模型
    QStandardItemModel mModelPath;
};

#endif // DIALOGSETTING_H
