#ifndef FFMPEGNV12WIDGET_H
#define FFMPEGNV12WIDGET_H

#include <QObject>
#include <QWidget>
#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QGLShaderProgram>
#include <QMutex>
#include <mutex>
#include <QOpenGLBuffer>
#include "IVideoCall.h"
struct AVFrame;


class FFmpegNV12Widget : public QOpenGLWidget,protected QOpenGLFunctions,public IVideoCall
{
    Q_OBJECT
public:
    void Init(int nWidth,int nHeight) override;
    FFmpegNV12Widget(QWidget* parent = nullptr);
    ~FFmpegNV12Widget();
    virtual void Repaint(AVFrame* pFrame) override; //不管成功與否都釋放frame空間
    // QOpenGLWidget interface
protected:
    void initializeGL() override;
    void resizeGL(int w, int h) override;
    void paintGL() override;
private:
    void SameSizeView(AVFrame* pFrame);
    void AlignmentView(AVFrame* pFrame);
    //shader程序
    QGLShaderProgram m_program;
    //shader中yuv變數
    GLuint unis[2] = {0};
    GLuint texs[2] = {0};
    unsigned char *datas[2] = { 0 };
    int m_nWidth;
    int m_nHeight;
    std::mutex m_Mutex;
    QOpenGLBuffer vbo;
};

#endif // FFMPEGNV12WIDGET_H
