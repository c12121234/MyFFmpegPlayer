#include "ffmpegaudio.h"
#include "ffmpegdecode.h"
#include "ffmpegaudioplay.h"
#include "ffmpegresample.h"
#include <QDebug>

void FFmpegAudio::run()
{
    unsigned char* pcm = new unsigned char[1024*1024*10];
    while(!m_bIsExit)
    {
        m_AudioMutex.lock();
//        if(m_PacketsList.empty() || !m_pDecode || !m_pAudioPlay || !m_pResample)
//        {
//            m_AudioMutex.unlock();
//            msleep(1);
//            continue;
//        }
//        AVPacket* pPkt = m_PacketsList.front();
//        m_PacketsList.pop_front();
        if(m_bIsPause)
        {
            m_AudioMutex.unlock();
            msleep(5);
            continue;
        }
        AVPacket* pPkt = this->Pop();
        bool bRet = m_pDecode->Send(pPkt);
//        if(!bRet)
//        {
//            m_AudioMutex.unlock();
//            msleep(1);
//            continue;
//        }
        //可能一次send 多個recv
        while(!m_bIsExit)
        {
            AVFrame* pFrame = m_pDecode->Recv();
            //減去緩衝中未播放的時間 單位ms
            m_nPts = m_pDecode->GetPts() - m_pAudioPlay->GetNoPlayMs();
            if(!pFrame)
                break;
            //resample
            int nSize = m_pResample->Resample(pFrame,pcm);
            //play audio
            while(!m_bIsExit)
            {
                if(nSize<=0)
                    break;
                int nFree = m_pAudioPlay->GetFree();
                if(nFree<nSize || m_bIsPause)
                {
                    msleep(1);
                    continue;
                }
                m_pAudioPlay->Write(pcm,nSize);
                break;
            }
        }
        m_AudioMutex.unlock();
        msleep(1);
    }
    delete pcm;
}

FFmpegAudio::FFmpegAudio():
  m_pAudioPlay(nullptr)
  , m_pResample(nullptr)
  , m_nPts(0)
  , m_bIsPause(false)
{
    if(!m_pResample)
        m_pResample = new FFmpegResample();
    if(!m_pAudioPlay)
        m_pAudioPlay = FFmpegAudioPlay::Get();
}

FFmpegAudio::~FFmpegAudio()
{
    m_bIsExit = true;
    wait();
}

bool FFmpegAudio::Open(AVCodecParameters *pPara, int nSampleRate, int nChannels)
{
    if(!pPara)
        return false;
    Clear();
    m_AudioMutex.lock();
    m_nPts = 0; //因為多次開啟open 每次開啟重置pts值 (若有close function 也可寫在close內)

    bool bRet = true;
    if(!m_pResample->Open(pPara,false))
    {
        qInfo()<<"FFmpegResample open failed.";
        bRet = false;
    }
    m_pAudioPlay->SetSampleRate(nSampleRate);
    m_pAudioPlay->SetChannels(nChannels);
    if(!m_pAudioPlay->Open())
    {
        qInfo()<<"FFmpegAudio open failed.";
        bRet = false;
    }
    if(!m_pDecode->Open(pPara))
    {
        qInfo()<<"audio FFmpegDecode open failed.";
        bRet = false;
    }
    m_AudioMutex.unlock();
    qInfo()<<"FFmpegAudio open:"<<bRet;
    return bRet;
}

void FFmpegAudio::Close()
{
    this->FFmpegDecodeThread::Close();
    if(m_pResample)
    {
        m_pResample->Close();
        m_AudioMutex.lock();
        delete m_pResample;
        m_pResample = nullptr;
        m_AudioMutex.unlock();
    }
    if(m_pAudioPlay)
    {
        m_pAudioPlay->Close();
        m_AudioMutex.lock();
        m_pAudioPlay = nullptr;
        m_AudioMutex.unlock();
    }
}

void FFmpegAudio::Clear()
{
    this->FFmpegDecodeThread::Clear();
    m_AudioMutex.lock();
    if(m_pAudioPlay)
        m_pAudioPlay->Clear();
    m_AudioMutex.unlock();
}

long long FFmpegAudio::GetPts()
{
    return m_nPts;
}

void FFmpegAudio::SetPts(long long nPts)
{
    m_AudioMutex.lock();
    m_nPts = nPts;
    m_AudioMutex.unlock();
}

void FFmpegAudio::SetPause(bool bIsPause)
{
    m_AudioMutex.lock();
    m_bIsPause = bIsPause;
    if(m_pAudioPlay)
        m_pAudioPlay->SetPause(bIsPause);
    m_AudioMutex.unlock();
}

bool FFmpegAudio::GetPauseState()
{
    return m_bIsPause;
}
