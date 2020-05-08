#include "ffmpegslider.h"
#include <QMouseEvent>
FFmpegSlider::FFmpegSlider(QWidget *parent):
    QSlider(parent)
{

}

void FFmpegSlider::mousePressEvent(QMouseEvent *event)
{
    double dPos = (double)event->pos().x()/(double)this->width();
    this->setValue(dPos*this->maximum());
    //原有事件處理
    //QSlider::mousePressEvent(event);
    QSlider::sliderReleased();
}
