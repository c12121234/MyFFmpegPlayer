#include "ffmpegvideowidget.h"

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

//準備yuv數據
//ffmpeg -i input.mp4 -t 10 -s 240x128 -pix_fmt yuv420p out240x128.yuv
//自動加雙引號
#define GET_STR(x) #x
#define A_VER 3
#define T_VER 4

//頂點shader

const char *vString = GET_STR(
    attribute vec4 vertexIn;
    attribute vec2 textureIn;
    varying vec2 textureOut;
    void main(void)
    {
        gl_Position = vertexIn;
        textureOut = textureIn;
    }
);




//像素shader
const char *tString = GET_STR(
        varying vec2 textureOut;
        uniform sampler2D tex_y;
        uniform sampler2D tex_u;
        uniform sampler2D tex_v;
        void main(void)
        {
            vec3 yuv;
            vec3 rgb;
            yuv.x = texture2D(tex_y, textureOut).r;
            yuv.y = texture2D(tex_u, textureOut).r - 0.5;
            yuv.z = texture2D(tex_v, textureOut).r - 0.5;
            rgb = mat3(1.0, 1.0, 1.0,
                0.0, -0.39465, 2.03211,
                1.13983, -0.58060, 0.0) * yuv;
            gl_FragColor = vec4(rgb, 1.0);
        }

    );

void FFmpegVideoWidget::Init(int nWidth, int nHeight)
{
    m_Mutex.lock();
    m_nWidth = nWidth;
    m_nHeight = nHeight;
    delete datas[0];
    delete datas[1];
    delete datas[2];

    datas[0] = new unsigned char[m_nWidth*m_nHeight];   //Y
    datas[1] = new unsigned char[m_nWidth*m_nHeight/4];	//U
    datas[2] = new unsigned char[m_nWidth*m_nHeight/4];	//V

    if(texs[0])
    {
        glDeleteTextures(3,texs);
    }
    //創建材質
    glGenTextures(3,texs);
    //Y
    glBindTexture(GL_TEXTURE_2D, texs[0]);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, m_nWidth, m_nHeight, 0, GL_RED, GL_UNSIGNED_BYTE, 0);

    //U
    glBindTexture(GL_TEXTURE_2D, texs[1]);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, m_nWidth/2, m_nHeight / 2, 0, GL_RED, GL_UNSIGNED_BYTE, 0);

    //V
    glBindTexture(GL_TEXTURE_2D, texs[2]);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, m_nWidth / 2, m_nHeight / 2, 0, GL_RED, GL_UNSIGNED_BYTE, 0);
    m_Mutex.unlock();
}

FFmpegVideoWidget::FFmpegVideoWidget(QWidget *parent):
    QOpenGLWidget(parent)
  ,m_nWidth(0)
  ,m_nHeight(0)
{

}

FFmpegVideoWidget::~FFmpegVideoWidget()
{

}

void FFmpegVideoWidget::Repaint(AVFrame *pFrame)
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
    qDebug()<<"after linesize:"<<pFrame->linesize[0];
    qDebug()<<"after frame format: "<<pFrame->format;
//    int nSize = av_image_get_buffer_size((AVPixelFormat)pFrame->format, pFrame->width,
//                                            pFrame->height, 1);
    switch ((AVPixelFormat)pFrame->format)
    {
    case AV_PIX_FMT_YUV420P:
        if(m_nWidth == pFrame->linesize[0])
            SameSizeView(pFrame);
        else
            AlignmentView(pFrame);
        break;
    case AV_PIX_FMT_NV12:
    {
        memcpy(datas[0],pFrame->data[0],m_nWidth*m_nHeight);
        for(int i =0,u=0,v=0;i<m_nWidth*m_nHeight/2;++i)
        {
            if(i%2==0)
                datas[1][u++] = pFrame->data[1][i];
            else
                datas[2][v++] = pFrame->data[1][i];
        }
//        for(int i = 1,v=0;i<m_nHeight/2;i+=2)
//            memcpy(datas[2]+m_nWidth/2*v,pFrame->data[1]+pFrame->linesize[1]*v,m_nWidth);
//        unsigned char* TempU = new unsigned char[m_nWidth*m_nHeight/4];
//        unsigned char* TempV = new unsigned char[m_nWidth*m_nHeight/4];
//        for(int i = 0,u =0,v=0;i<m_nWidth*m_nHeight/2;++i)
//        {
//            if(i%2==0)
//            {
//                TempU[u++] = pFrame->data[1][i];
//            }
//                //memcpy(datas[1][u++],pFrame->data[1][i],sizeof(pFrame->data[1][i]));
//                //datas[1][u++] = pFrame->data[1][i];
//            else if(i%2==1)
//            {
//                TempV[v++] = pFrame->data[1][i];
//            }
//                //memcpy(datas[1][v++],pFrame->data[1][i],sizeof(pFrame->data[1][i]));
//                //datas[2][v++] = pFrame->data[1][i];
//        }
//        memcpy(datas[1],TempU,sizeof(TempU));
//        memcpy(datas[1],TempV,sizeof(TempV));
//        delete[] TempU;
//        delete[] TempV;
        break;
    }
    case AV_PIX_FMT_VAAPI_VLD:
        qDebug()<<"frame->data[3]:"<<pFrame->data[3];
        qDebug()<<"AV_PIX_FMT_VAAPI_VLD";
        break;
    default:
        qDebug()<<"undefined format.";
        break;
    }
    m_Mutex.unlock();
    av_frame_free(&pFrame);
    //行對齊問題    
    update();
    qDebug()<<"update Repaint";
}

void FFmpegVideoWidget::initializeGL()
{
    qInfo()<<"initializeGL";
    m_Mutex.lock();
    //初始化opengl函數 由QOpenGLFunctions繼承
    initializeOpenGLFunctions();
    //program加載頂點和像素shader腳本
    qInfo()<<m_program.addShaderFromSourceCode(QGLShader::Fragment,tString);
    qInfo()<<m_program.addShaderFromSourceCode(QGLShader::Vertex,vString);

    //設置頂點座標變量
    m_program.bindAttributeLocation("vertexIn",A_VER);
    //設置材質座標
    m_program.bindAttributeLocation("textureIn",T_VER);


    //編譯shader
    qInfo()<<"program link() = "<<m_program.link();
    qInfo()<<"program bind() = "<<m_program.bind();

    //傳遞頂點和材質座標
    //頂點
    static const GLfloat ver[] = {
        -1.0f,-1.0f,
        1.0f,-1.0f,
        -1.0f,1.0f,
        1.0f,1.0f
    };
    //材質
    static const GLfloat tex[] = {
        0.0f,1.0f,
        1.0f,1.0f,
        0.0f,0.0f,
        1.0f,0.0f
    };
    //頂點
    glVertexAttribPointer(A_VER,2,GL_FLOAT,0,0,ver);
    glEnableVertexAttribArray(A_VER);

    //材質
    glVertexAttribPointer(T_VER,2,GL_FLOAT,0,0,tex);
    glEnableVertexAttribArray(T_VER);

    //從shader獲取材質
    unis[0] = m_program.uniformLocation("tex_y");
    unis[1] = m_program.uniformLocation("tex_u");
    unis[2] = m_program.uniformLocation("tex_v");

    m_Mutex.unlock();
}

void FFmpegVideoWidget::resizeGL(int w, int h)
{
    m_Mutex.lock();
    qInfo()<<"resizeGL"<<w<<" "<<h;
    m_Mutex.unlock();
}

void FFmpegVideoWidget::paintGL()
{
    qDebug()<<"paintGL";
    m_Mutex.lock();
    //y
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D,texs[0]);
    glTexSubImage2D(GL_TEXTURE_2D,0,0,0,m_nWidth,m_nHeight,GL_RED,GL_UNSIGNED_BYTE,datas[0]);
    glUniform1i(unis[0],0);

    //u
    glActiveTexture(GL_TEXTURE0+1);
    glBindTexture(GL_TEXTURE_2D,texs[1]);
    glTexSubImage2D(GL_TEXTURE_2D,0,0,0,m_nWidth/2,m_nHeight/2,GL_RED,GL_UNSIGNED_BYTE,datas[1]);
    glUniform1i(unis[1],1);

    //v
    glActiveTexture(GL_TEXTURE0+2);
    glBindTexture(GL_TEXTURE_2D,texs[2]);
    glTexSubImage2D(GL_TEXTURE_2D,0,0,0,m_nWidth/2,m_nHeight/2,GL_RED,GL_UNSIGNED_BYTE,datas[2]);
    glUniform1i(unis[2],2);

    glDrawArrays(GL_TRIANGLE_STRIP,0,4);
    m_Mutex.unlock();
}

void FFmpegVideoWidget::SameSizeView(AVFrame *pFrame)
{
    memcpy(datas[0],pFrame->data[0],m_nWidth*m_nHeight);
    memcpy(datas[1],pFrame->data[1],m_nWidth*m_nHeight/4);
    memcpy(datas[2],pFrame->data[2],m_nWidth*m_nHeight/4);
}

void FFmpegVideoWidget::AlignmentView(AVFrame *pFrame)
{
    for(int i =0;i<m_nHeight;++i)//y
        memcpy(datas[0]+m_nWidth*i,pFrame->data[0]+pFrame->linesize[0]*i,m_nWidth);
    for(int i =0;i<m_nHeight/2;++i)//u
        memcpy(datas[0]+m_nWidth/2*i,pFrame->data[1]+pFrame->linesize[1]*i,m_nWidth);
    for(int i =0;i<m_nHeight/2;++i)//v
        memcpy(datas[0]+m_nWidth/2*i,pFrame->data[2]+pFrame->linesize[2]*i,m_nWidth);
}
