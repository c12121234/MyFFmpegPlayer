#include "widget.h"
#include "ffmpegdemux.h"
#include "ffmpegdecode.h"
#include "ffmpegvideowidget.h"
#include "ffmpegresample.h"
#include "ffmpegaudioplay.h"
#include "ffmpegaudio.h"
#include "ffmpegvideo.h"
#include "ffmpegdemuxthread.h"
#include <QApplication>
#include <QDebug>
#include <QThread>

class TestThread :public QThread
{
public:
    FFmpegDemux demux;
    FFmpegDeCode vDecode;
    FFmpegDeCode aDecode;
    FFmpegResample reSample;
    FFmpegVideoWidget* video = nullptr;
    FFmpegAudio audio;
    FFmpegVideo videoThread;
    void Init()
    {
        qInfo()<<demux.Open("30fps.mp4");
        //qInfo()<<demux.Open(strURL);
        qInfo()<<"CopyVPara = "<<demux.CopyVPara();
        qInfo()<<"CopyAPara = "<<demux.CopyAPara();
        //qInfo()<<"seek = "<<demux.Seek(0.9);
        //////////////////////////////////解碼測試
        //qInfo()<<"vdecode.open() = "<<vDecode.Open(demux.CopyVPara());
        //qInfo()<<"adecode.open() = "<<aDecode.Open(demux.CopyAPara());
        //qInfo()<<"reSample.open() = "<<reSample.Open(demux.CopyAPara());
        //FFmpegAudioPlay::Get()->SetSampleRate(demux.GetSampleRate());
        //FFmpegAudioPlay::Get()->SetChannels(demux.GetChannels());
        //m_bAudioOpen = FFmpegAudioPlay::Get()->Open();
        qInfo()<<"audio open:"<<audio.Open(demux.CopyAPara(),demux.GetSampleRate(),demux.GetChannels());
        qInfo()<<"video open:"<<videoThread.Open(demux.CopyVPara(),video,demux.GetWidth(),demux.GetHeight());
        audio.start();
        videoThread.start();
    }
    unsigned char* pcm = new unsigned char[1024*1024];
    void run() override
    {
        while(true)
        {
            AVPacket* pPkt = demux.Read();
            if(demux.IsAudio(pPkt))
            {
                audio.Push(pPkt);
//                aDecode.Send(pPkt);
//                AVFrame* pFrame = aDecode.Recv();
//                int nLength = reSample.Resample(pFrame,pcm);
//                qInfo()<<"reSample:"<<nLength;
//                while(nLength>0)
//                {
//                    if(FFmpegAudioPlay::Get()->GetFree()>=nLength)
//                    {
//                        FFmpegAudioPlay::Get()->Write(pcm,nLength);
//                        break;
//                    }
//                    msleep(1);
//                }
            }
            else
            {
                //vDecode.Send(pPkt);
                //AVFrame* pFrame = vDecode.Recv();
                //video->Repaint(pFrame);
                //msleep(40);
                videoThread.Push(pPkt);
            }
            if(!pPkt)
                break;
        }
    }
};


int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    Widget w;
    w.show();
    //FFmpegDemuxThread dt;
    //FFmpegVideoWidget* video = w.GetVideo();
    //dt.Open("30fps.mp4",video);
    //dt.Start();
    //TestThread tt;

//    FFmpegDemux demux;
//    //QString strURL = "rtmp://cbs-live.gscdn.com/cbs-live/cbs-live.stream";
//    qInfo()<<demux.Open("30fps.mp4");
//    //qInfo()<<demux.Open(strURL);
//    qInfo()<<"CopyVPara = "<<demux.CopyVPara();
//    qInfo()<<"CopyAPara = "<<demux.CopyAPara();
//    qInfo()<<"seek = "<<demux.Seek(0.9);
//    //////////////////////////////////解碼測試
//    FFmpegDeCode vDecode;
//    qInfo()<<"vdecode.open() = "<<vDecode.Open(demux.CopyVPara());
//    FFmpegDeCode aDecode;
//    qInfo()<<"adecode.open() = "<<aDecode.Open(demux.CopyAPara());
    /*
    while(true)
    {
        AVPacket* pPkt = demux.Read();
        if(demux.IsAudio(pPkt))
        {
            aDecode.Send(pPkt);
            AVFrame* pFrame = aDecode.Recv();
        }
        else
        {
            vDecode.Send(pPkt);
            AVFrame* pFrame = vDecode.Recv();
        }
        if(!pPkt)
            break;
    }*/
    //w.InitVideoWidget(tt.demux.GetWidth(),tt.demux.GetHeight());
    //tt.video = w.GetVideo();
    //tt.Init();
    //tt.start();
    return a.exec();
}
