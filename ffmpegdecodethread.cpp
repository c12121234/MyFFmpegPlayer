#include "ffmpegdecode.h"
#include "ffmpegdecodethread.h"
extern void FFmpegFreePacket(AVPacket** pPkt);

FFmpegDecodeThread::FFmpegDecodeThread():
    m_pDecode(nullptr)
{
    if(!m_pDecode)
        m_pDecode = new FFmpegDeCode();
}

FFmpegDecodeThread::~FFmpegDecodeThread()
{
    m_bIsExit = true;
    wait();
}


void FFmpegDecodeThread::Push(AVPacket *pPkt)
{
    if(!pPkt)
        return;
    while(!m_bIsExit)
    {
        m_Mutex.lock();
        if(m_PacketsList.size()<m_nMaxBuffer)
        {
            m_PacketsList.push_back(pPkt);
            m_Mutex.unlock();
            break;
        }
        m_Mutex.unlock();
        msleep(1);
    }
}

AVPacket *FFmpegDecodeThread::Pop()
{
    m_Mutex.lock();
    if(m_PacketsList.empty())
    {
        m_Mutex.unlock();
        return nullptr;
    }
    AVPacket* pPkt = m_PacketsList.front();
    m_PacketsList.pop_front();
    m_Mutex.unlock();
    return pPkt;
}

void FFmpegDecodeThread::Clear()
{
    m_Mutex.lock();
    m_pDecode->Clear();
    while(!m_PacketsList.empty())
    {
        AVPacket* pPkt = m_PacketsList.front();
        FFmpegFreePacket(&pPkt);
        m_PacketsList.pop_front();
    }
    m_Mutex.unlock();
}

void FFmpegDecodeThread::Close()
{
    Clear();
    m_bIsExit = true;
    wait();
    if(m_pDecode)
    {
        m_pDecode->Close();
        m_Mutex.lock();
        delete m_pDecode;
        m_pDecode = nullptr;
        m_Mutex.unlock();
    }

}
