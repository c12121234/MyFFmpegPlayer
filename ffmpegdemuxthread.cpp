#include "ffmpegdemuxthread.h"
#include "ffmpegvideo.h"
#include "ffmpegaudio.h"
#include "ffmpegdemux.h"
#include "ffmpegdecode.h"
#include <QDebug>
FFmpegDemuxThread::FFmpegDemuxThread():
    m_pDemux(nullptr)
  ,m_pVideoThread(nullptr)
  ,m_pAudioThread(nullptr)
  ,m_bIsExit(false)
  ,m_nPts(0)
  ,m_nTotalMs(0)
{

}

FFmpegDemuxThread::~FFmpegDemuxThread()
{
    m_bIsExit = true;
    wait();
}

bool FFmpegDemuxThread::Open(const char *strUrl, IVideoCall *pVideoCall)
{
    if(strUrl == nullptr || strUrl[0]=='\0')
        return false;
    m_Mutex.lock();
    //open demux
    bool bRet = m_pDemux->Open(strUrl);
    if(!bRet)
    {
        qDebug()<<"m_pDemux->Open(strUrl) failed.";
        return false;
    }
    //open video decoder and thread
    if(!m_pVideoThread->Open(m_pDemux->CopyVPara(),pVideoCall,m_pDemux->GetWidth(),m_pDemux->GetHeight()))
    {
        bRet = false;
        qDebug()<<"m_pVideoThread->Open failed.";
    }

    if(!m_pAudioThread->Open(m_pDemux->CopyAPara(),m_pDemux->GetSampleRate(),m_pDemux->GetChannels()))
    {
        bRet = false;
        qDebug()<<"m_pAudioThread->Open failed.";
    }
    m_nTotalMs = m_pDemux->GetTotalTimeMs();
    m_Mutex.unlock();
    qDebug()<<"FFmpegDemuxThread::Open :"<<bRet;
    return bRet;
}

void FFmpegDemuxThread::Start()
{
    m_Mutex.lock();
    if(!m_pDemux)
        m_pDemux = new FFmpegDemux();
    if(!m_pAudioThread)
        m_pAudioThread = new FFmpegAudio();
    if(!m_pVideoThread)
        m_pVideoThread = new FFmpegVideo();
    QThread::start();
    if(m_pVideoThread)
        m_pVideoThread->start();
    if(m_pAudioThread)
        m_pAudioThread->start();
    m_Mutex.unlock();
}

void FFmpegDemuxThread::Close()
{
    m_bIsExit = true;
    wait();
    if(m_pVideoThread)
        m_pVideoThread->Close();
    if(m_pAudioThread)
        m_pAudioThread->Close();
    m_Mutex.lock();
    if(m_pVideoThread)
    {
        delete m_pVideoThread;
        m_pVideoThread = nullptr;
    }
    if(m_pAudioThread)
    {
        delete m_pAudioThread;
        m_pAudioThread = nullptr;
    }
    m_Mutex.unlock();
}

void FFmpegDemuxThread::Clear()
{
    m_Mutex.lock();
    if(m_pDemux)
        m_pDemux->Clear();
    if(m_pAudioThread)
        m_pAudioThread->Clear();
    if(m_pVideoThread)
        m_pVideoThread->Clear();
    m_Mutex.unlock();
}

void FFmpegDemuxThread::Seek(double dPos)
{
    Clear();
    m_Mutex.lock();
    bool bPauseStatus = m_bIsPause;
    m_Mutex.unlock();
    SetPause(true);
    m_Mutex.lock();
    if(m_pDemux)
        m_pDemux->Seek(dPos);
    //實際要顯示的位置pts
    long long nSeekPts = dPos*m_pDemux->GetTotalTimeMs();
    while(!m_bIsExit)
    {
        AVPacket* pPkt = m_pDemux->ReadVideo();
        if(!pPkt)
            break;
        if(m_pVideoThread->RepaintPts(pPkt,nSeekPts)) //如果解碼到nSeekPts
        {
            m_nPts = nSeekPts;
            break;
        }

//        bool bRet = m_pVideoThread->m_pDecode->Send(pPkt);
//        if(!bRet)
//            break;
//        AVFrame* pFrame = m_pVideoThread->m_pDecode->Recv();
//        if(!pFrame)
//            continue;
//        if(pFrame->pts >= nSeekPts)
//        {
//            m_nPts = pFrame->pts;
//            m_pVideoThread->m_pVideoCall->Repaint(pFrame);
//            break;
//        }
//        av_frame_free(&pFrame);
    }
    m_Mutex.unlock();
    //seek時非暫停狀態
    if(!bPauseStatus)
        SetPause(false);
}

void FFmpegDemuxThread::run()
{
    while(!m_bIsExit)
    {       
        m_Mutex.lock();
        if(m_bIsPause)
        {
            m_Mutex.unlock();
            msleep(5);
            continue;
        }
        if(!m_pDemux)
        {
            m_Mutex.unlock();
            msleep(1);
            continue;
        }
        //音視訊同步
        if(m_pVideoThread && m_pAudioThread)
        {
            m_nPts = m_pAudioThread->GetPts();
            //m_pVideoThread->SetSynPts(m_pAudioThread->GetPts());
            m_pVideoThread->SetSynPts(m_nPts);
        }

        AVPacket* pPkt = m_pDemux->Read();
        if(!pPkt)
        {
            m_Mutex.unlock();
            msleep(1);
            continue;
        }
        //判斷數據是否為音訊
        if(m_pDemux->IsAudio(pPkt))
        {
            if(m_pAudioThread)
                m_pAudioThread->Push(pPkt);
        }
        else
        {
            if(m_pVideoThread)
                m_pVideoThread->Push(pPkt);
        }
        m_Mutex.unlock();
        msleep(1);
    }
}

long long FFmpegDemuxThread::GetPts()
{
    return m_nPts;
}

long long FFmpegDemuxThread::GetTotalMs()
{
    return m_nTotalMs;
}

void FFmpegDemuxThread::SetPause(bool bIsPause)
{
    m_Mutex.lock();
    m_bIsPause = bIsPause;
    if(m_pAudioThread)
        m_pAudioThread->SetPause(bIsPause);
    if(m_pVideoThread)
        m_pVideoThread->SetPause(bIsPause);
    m_Mutex.unlock();
}

bool FFmpegDemuxThread::GetPauseState()
{
    return m_bIsPause;
}
