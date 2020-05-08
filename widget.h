#ifndef WIDGET_H
#define WIDGET_H

#include <QWidget>

QT_BEGIN_NAMESPACE
namespace Ui { class Widget; }
QT_END_NAMESPACE

class FFmpegVideoWidget;

class Widget : public QWidget
{
    Q_OBJECT

public:
    Widget(QWidget *parent = nullptr);
    ~Widget();
    void timerEvent(QTimerEvent *event) override;
    void SetPause(bool bIsPause);
public slots:
    void HandleOpenFile();
    void HandleBtnStartClicked();
    void HandleSliderPress();
    void HandleSliderRelease();
protected:
    void mouseDoubleClickEvent(QMouseEvent *event) override;

private:
    Ui::Widget *ui;
    bool m_bIsSliderPress;
    void ConnectUI();
};
#endif // WIDGET_H
