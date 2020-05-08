#include "widget.h"
#include "ui_widget.h"

#include "ffmpegvideowidget.h"
#include "ffmpegdemuxthread.h"
#include <QPushButton>
#include <QFileDialog>
#include <QMessageBox>

static FFmpegDemuxThread Dt;
Widget::Widget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::Widget)
    , m_bIsSliderPress(false)
{
    ui->setupUi(this);
    ConnectUI();
    Dt.Start();
    startTimer(30);
}

Widget::~Widget()
{
    Dt.Close();
    delete ui;
}

void Widget::timerEvent(QTimerEvent *event)
{
    Q_UNUSED(event)
    if(m_bIsSliderPress)
        return;
    long long  nTotalTime = Dt.GetTotalMs();
    long long nPts = Dt.GetPts();
    //qDebug()<<"getPts: "<<nPts;
    if(nPts>nTotalTime || llabs(nPts)>nTotalTime)
        return;
    if(nTotalTime>0)
    {
        //qDebug()<<"getPts: "<<nPts;
        double dPos = (double)nPts/(double)nTotalTime;
        int v = ui->horizontalSlider->maximum()*dPos;
        qDebug()<<"v:"<<v;
        if(v < 0)
            return;
        ui->horizontalSlider->setValue(v);
    }
}

void Widget::SetPause(bool bIsPause)
{
    if(bIsPause)
        ui->BtnStart->setText("Play");
    else
        ui->BtnStart->setText("Pause");
}

void Widget::HandleOpenFile()
{
    QString strFileName = QFileDialog::getOpenFileName(nullptr,QString::fromLocal8Bit("選擇檔案："));
    if(strFileName.isEmpty())
        return;
    this->setWindowTitle(strFileName);
    if(!Dt.Open(strFileName.toLocal8Bit(),ui->Video))
    {
        QMessageBox::information(nullptr,"error","open file failed!");
        return;
    }
    SetPause(Dt.GetPauseState());
}

void Widget::HandleBtnStartClicked()
{
    bool bIsPause = !Dt.GetPauseState();
    SetPause(bIsPause);
    Dt.SetPause(bIsPause);
}

void Widget::HandleSliderPress()
{
    m_bIsSliderPress = true;
}

void Widget::HandleSliderRelease()
{
    m_bIsSliderPress = false;
    double dPos = 0.0;
    dPos = (double)ui->horizontalSlider->value()/(double)ui->horizontalSlider->maximum();
    Dt.Seek(dPos);
}

void Widget::mouseDoubleClickEvent(QMouseEvent *event)
{
    Q_UNUSED(event)
    if(isFullScreen())
        this->showNormal();
    else
        this->showFullScreen();
}

//void Widget::resizeEvent(QResizeEvent *event)
//{
//    ui->Video->resize(this->size());
//}

void Widget::ConnectUI()
{
    connect(ui->pushButton,&QPushButton::clicked,this,&Widget::HandleOpenFile);
    connect(ui->BtnStart,&QPushButton::clicked,this,&Widget::HandleBtnStartClicked);
    connect(ui->horizontalSlider,&QSlider::sliderPressed,this,&Widget::HandleSliderPress);
    connect(ui->horizontalSlider,&QSlider::sliderReleased,this,&Widget::HandleSliderRelease);
}

