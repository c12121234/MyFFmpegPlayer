#ifndef FFMPEGSLIDER_H
#define FFMPEGSLIDER_H

#include <QSlider>

class FFmpegSlider : public QSlider
{
public:
    FFmpegSlider(QWidget* parent = nullptr);

    // QWidget interface
protected:
    void mousePressEvent(QMouseEvent *event) override;
};

#endif // FFMPEGSLIDER_H
