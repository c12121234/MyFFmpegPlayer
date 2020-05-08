#include "ffmpegvideo.h"
#include "ffmpegdecode.h"
#include <QDebug>
FFmpegVideo::FFmpegVideo(): 
  m_pVideoCall(nullptr)
  , m_nSynPts(0)
,m_bIsPause(false)
{

}

FFmpegVideo::~FFmpegVideo()
{

}

bool FFmpegVideo::Open(AVCodecParameters *pPara, IVideoCall *pVideoCall, int nWidth, int nHeight)
{
    if(!pPara)
        return false;
    Clear();
    m_VideoMutex.lock();
    m_nSynPts = 0;
    m_pVideoCall = pVideoCall;
    if(pVideoCall)
    {
        pVideoCall-> Init(nWidth,nHeight);
    }
    m_VideoMutex.unlock();
    bool bRet = true;
    if(!m_pDecode->Open(pPara))
    {
        qInfo()<<"audio FFmpegDecode open failed.";
        bRet = false;
    }
    qInfo()<<"FFmpegAudio open:"<<bRet;
    return bRet;
}

bool FFmpegVideo::RepaintPts(AVPacket *pPkt, long long nSeekPts)
{
    m_VideoMutex.lock();
    bool bRet = m_pDecode->Send(pPkt);
    if(!bRet)
    {
        m_VideoMutex.unlock();
        return true;//return true 結束解碼
    }

    AVFrame* pFrame = m_pDecode->Recv();
    if(!pFrame)
    {
        m_VideoMutex.unlock();
        return false;
    }

    if(m_pDecode->GetPts() >= nSeekPts)
    {
        if(m_pVideoCall)
            m_pVideoCall->Repaint(pFrame);
        m_VideoMutex.unlock();
        return true;
    }
    FFmpegFreeFrame(&pFrame);
    m_VideoMutex.unlock();
    return false;
}

void FFmpegVideo::run()
{
    while(!m_bIsExit)
    {
        m_VideoMutex.lock();
        if(m_bIsPause)
        {
            m_VideoMutex.unlock();
            msleep(5);
            continue;
        }
        long long nPts = m_pDecode->GetPts();
        if(m_nSynPts>0 && m_nSynPts <nPts)
        {
            m_VideoMutex.unlock();
            msleep(1);
            continue;
        }
//        if(m_PacketsList.empty() || !m_pDecode)
//        {
//            m_VideoMutex.unlock();
//            msleep(1);
//            continue;
//        }
        //音視訊同步


//        AVPacket* pPkt = m_PacketsList.front();
//        m_PacketsList.pop_front();
        AVPacket* pPkt = Pop();
        bool bRet = m_pDecode->Send(pPkt);
//        if(!bRet)
//        {
//            m_VideoMutex.unlock();
//            msleep(1);
//            continue;
//        }
        //可能一次send 多個recv
        while(!m_bIsExit)
        {
            AVFrame* pFrame = m_pDecode->Recv();
            if(!pFrame)
                break;
            //show video frame
            //qDebug()<<"audio diff video pts:"<<m_nSynPts-nPts;
            if(m_pVideoCall)
            {
                m_pVideoCall->Repaint(pFrame);
                msleep(10);
            }
            else
                msleep(1);
        }
        m_VideoMutex.unlock();
        msleep(1);
    }
}

void FFmpegVideo::SetSynPts(long long nPts)
{
    m_VideoMutex.lock();
    m_nSynPts = nPts;
    m_VideoMutex.unlock();
}

void FFmpegVideo::SetPause(bool bIsPause)
{
    m_VideoMutex.lock();
    m_bIsPause = bIsPause;
    m_VideoMutex.unlock();
}

bool FFmpegVideo::GetPauseState()
{
    return m_bIsPause;
}
