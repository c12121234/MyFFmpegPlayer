#include "ffmpegnv12widget.h"

#include <QDebug>
#include <QFileDialog>
#include <QFile>
#include <QTimer>

extern "C"
{
#include "libavutil/frame.h"
#include "libavutil/hwcontext.h"
#include "libavutil/imgutils.h"
#include "libavcodec/avcodec.h"
#include "libavcodec/vaapi.h"

}
extern const char* vString;
extern const char* tString;

#define GET_STR(x) #x
#define A_VER 3
#define T_VER 2

//頂點shader

const char *v12String = GET_STR(
    attribute vec4 vertexIn;
    attribute vec4 textureIn;
    varying vec4 textureOut;
    void main(void)
    {
        gl_Position = vertexIn;
        textureOut = textureIn;
    }
);

//像素shader
const char *t12String = GET_STR(
        varying mediump vec4 textureOut;
        uniform sampler2D tex_y;
        uniform sampler2D tex_uv;
        void main(void)
        {
            vec3 yuv;
            vec3 rgb;
            yuv.x = texture2D(tex_y, textureOut.st).r-0.0625;
            yuv.y = texture2D(tex_uv, textureOut.st).r - 0.5;
            yuv.z = texture2D(tex_uv, textureOut.st).g - 0.5;
            rgb = mat3(1.0, 1.0, 1.0,
                0.0, -0.39465, 2.03211,
                1.13983, -0.58060, 0.0) * yuv;
            gl_FragColor = vec4(rgb, 1.0);
        }

    );

void FFmpegNV12Widget::Init(int nWidth, int nHeight)
{
    m_Mutex.lock();
    m_nWidth = nWidth;
    m_nHeight = nHeight;
    if(datas[0])
        delete datas[0];
    if(datas[1])
        delete datas[1];

    datas[0] = new unsigned char[m_nWidth*m_nHeight];   //Y
    datas[1] = new unsigned char[m_nWidth*m_nHeight/2];	//UV
    m_Mutex.unlock();
}

FFmpegNV12Widget::FFmpegNV12Widget(QWidget *parent):
    QOpenGLWidget(parent)
  ,m_nWidth(0)
  ,m_nHeight(0)
{

}

FFmpegNV12Widget::~FFmpegNV12Widget()
{

}

void FFmpegNV12Widget::Repaint(AVFrame *pFrame)
{
    if(!pFrame)
        return;
    m_Mutex.lock();
    if(!datas[0] || m_nWidth*m_nHeight == 0||
            pFrame->width != m_nWidth || pFrame->height != m_nHeight)
    {
        av_frame_free(&pFrame);
        m_Mutex.unlock();
        return;
    }
    //qDebug()<<"after linesize:"<<pFrame->linesize[0];
    //qDebug()<<"after frame format: "<<pFrame->format;
    switch ((AVPixelFormat)pFrame->format)
    {
    case AV_PIX_FMT_NV12:
    {
        if(m_nWidth == pFrame->linesize[0])
            SameSizeView(pFrame);
        else
            AlignmentView(pFrame);
        break;
    }
    default:
        qDebug()<<"undefined format.";
        break;
    }
    m_Mutex.unlock();
    av_frame_free(&pFrame);
    //行對齊問題
    update();
    //qDebug()<<"update Repaint";
}

void FFmpegNV12Widget::initializeGL()
{
    qInfo()<<"initializeGL";
    m_Mutex.lock();
    //初始化opengl函數 由QOpenGLFunctions繼承
    initializeOpenGLFunctions();
    //program加載頂點和像素shader腳本

    m_program.addShaderFromSourceCode(QGLShader::Vertex,v12String);
    m_program.addShaderFromSourceCode(QGLShader::Fragment,t12String);
    m_program.link();

    GLfloat points[]{
        -1.0f, 1.0f,
        1.0f, 1.0f,
        1.0f, -1.0f,
        -1.0f, -1.0f,

        0.0f,0.0f,
        1.0f,0.0f,
        1.0f,1.0f,
        0.0f,1.0f
    };
    vbo.create();
    vbo.bind();
    vbo.allocate(points,sizeof(points));

    //GLuint ids[2];
    glGenTextures(2,unis);
    //unis[0] = ids[0];
    //unis[1] = ids[1];
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    m_Mutex.unlock();
}

void FFmpegNV12Widget::resizeGL(int w, int h)
{
    m_Mutex.lock();
    qDebug()<<"resizeGL"<<w<<" "<<h;
    m_Mutex.unlock();
}

void FFmpegNV12Widget::paintGL()
{
    //qDebug()<<"paintGL";
    m_Mutex.lock();
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glDisable(GL_DEPTH_TEST);

    m_program.bind();
    vbo.bind();
    m_program.enableAttributeArray("vertexIn");
    m_program.enableAttributeArray("textureIn");
    m_program.setAttributeBuffer("vertexIn",GL_FLOAT, 0, 2, 2*sizeof(GLfloat));
    m_program.setAttributeBuffer("textureIn",GL_FLOAT,2 * 4 * sizeof(GLfloat),2,2*sizeof(GLfloat));

    glActiveTexture(GL_TEXTURE0 + 1);
    glBindTexture(GL_TEXTURE_2D,unis[0]);
    glTexImage2D(GL_TEXTURE_2D,0,GL_RED,m_nWidth,m_nHeight,0,GL_RED,GL_UNSIGNED_BYTE,datas[0]);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glActiveTexture(GL_TEXTURE0 + 0);
    glBindTexture(GL_TEXTURE_2D,unis[1]);
    glTexImage2D(GL_TEXTURE_2D,0,GL_RG,m_nWidth/2,m_nHeight/2,0,GL_RG,GL_UNSIGNED_BYTE,datas[1]);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    m_program.setUniformValue("tex_uv",0);
    m_program.setUniformValue("tex_y",1);
    glDrawArrays(GL_QUADS,0,4);
    m_program.disableAttributeArray("vertexIn");
    m_program.disableAttributeArray("textureIn");
    m_program.release();
    m_Mutex.unlock();
}

void FFmpegNV12Widget::SameSizeView(AVFrame *pFrame)
{
    memcpy(datas[0],pFrame->data[0],m_nWidth*m_nHeight);
    memcpy(datas[1],pFrame->data[1],m_nWidth*m_nHeight/2);
}

void FFmpegNV12Widget::AlignmentView(AVFrame *pFrame)
{
    for(int i =0;i<m_nHeight;++i)//y
        memcpy(datas[0]+m_nWidth*i,pFrame->data[0]+pFrame->linesize[0]*i,m_nWidth);
    for(int i =0;i<m_nHeight/2;i+=2)//top half
        memcpy(datas[1]+m_nWidth/2*i,pFrame->data[1]+pFrame->linesize[1]*i/2,m_nWidth);
    for(int i =0;i<m_nHeight/2;i+=2)//bottom half
        memcpy(datas[1]+(m_nWidth*m_nHeight/4)+m_nWidth/2*i,
                pFrame->data[1]+(m_nWidth*m_nHeight/4)+pFrame->linesize[1]*i/2,m_nWidth);

}
