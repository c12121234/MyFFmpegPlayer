#include "ffmpegaudioplay.h"
#include <QAudioFormat>
#include <QAudioOutput>
#include <QMutex>
#include <QDebug>

class CFFmpegAudioPlay : public FFmpegAudioPlay
{
public:
    QAudioOutput* m_pOutput = nullptr;
    QIODevice* m_pIO = nullptr;
    QMutex m_Mutex;

    void Close() override
    {
        m_Mutex.lock();
        if(m_pIO)
        {
            m_pIO->close();
            m_pIO = nullptr;
        }
        if(m_pOutput)
        {
            m_pOutput->stop();
            delete m_pOutput;
            m_pOutput = nullptr;
        }
        m_Mutex.unlock();
    }


    bool Open() override
    {
        Close();
        QAudioFormat fmt;
        fmt.setSampleRate(m_nSampleRate);
        fmt.setSampleSize(m_nSampleSize);
        fmt.setChannelCount(m_nChannels);
        fmt.setCodec("audio/pcm");
        fmt.setByteOrder(QAudioFormat::LittleEndian);
        fmt.setSampleType(QAudioFormat::SignedInt);
        m_Mutex.lock();
        //m_pOutput = new QAudioOutput(fmt);
        QAudioDeviceInfo info(QAudioDeviceInfo::defaultOutputDevice());
        QAudioFormat PreferInfo = info.preferredFormat();
        qInfo()<<"PreferInfo codec:"<<PreferInfo.codec();
        qInfo()<<"PreferInfo sampleRate"<<PreferInfo.sampleRate();
        qInfo()<<"PreferInfo sampleSize"<<PreferInfo.sampleSize();
        qInfo()<<"PreferInfo sampleType"<<PreferInfo.sampleType();
        qInfo()<<"PreferInfo channelCount"<<PreferInfo.channelCount();
        qInfo()<<"PreferInfo byteOrder"<<PreferInfo.byteOrder();
        for(auto ele:info.supportedSampleSizes())
            qInfo()<<"support samplesizes:"<<ele;
        if (!info.isFormatSupported(fmt))
        {
            qWarning()<<"raw audio format not supported by backend, cannot play audio.";
            fmt = info.nearestFormat(fmt);
            qInfo()<<"fixed codec:"<<fmt.codec();
            qInfo()<<"fixed sampleRate"<<fmt.sampleRate();
            qInfo()<<"fixed sampleSize"<<fmt.sampleSize();
            qInfo()<<"fixed sampleType"<<fmt.sampleType();
            qInfo()<<"fixed channelCount"<<fmt.channelCount();
            qInfo()<<"fixed byteOrder"<<fmt.byteOrder();
        }

        m_pOutput = new QAudioOutput(fmt);
        m_pIO = m_pOutput->start();
        qDebug()<<"error:"<<m_pOutput->error();
        m_Mutex.unlock();
        return (m_pIO != nullptr) ? true :false;
    }
    bool Write(const unsigned char *pData, int nDataSize) override
    {
        if(!pData || nDataSize<=0)
            return false;
        m_Mutex.lock();
        if(!m_pIO || !m_pOutput)
        {
            m_Mutex.unlock();
            return false;
        }
        int nSize = m_pIO->write((char*)pData,nDataSize);
        if(nDataSize != nSize)
            return false;
        m_Mutex.unlock();
        return true;
    }
    int GetFree() override
    {
        m_Mutex.lock();
        if(!m_pOutput)
        {
            m_Mutex.unlock();
            return 0;
        }
        int nFree = m_pOutput->bytesFree();
        m_Mutex.unlock();
        return nFree;
    }
    long long GetNoPlayMs() override
    {
        m_Mutex.lock();
        if(!m_pOutput)
        {
            m_Mutex.unlock();
            return 0;
        }
        long long nPts = 0;
        //還未播放的字節數大小
        double dSize = m_pOutput->bufferSize() - m_pOutput->bytesFree();
        //一秒音頻大小 samplesize以8為一字節(byte)
        double dSecPerSize = m_nSampleRate * (m_nSampleSize/8) * m_nChannels;
        if(dSecPerSize<=0)
            nPts = 0;
        else
        {
            nPts = (dSize/dSecPerSize)*1000; //turn to ms
        }
        m_Mutex.unlock();
        return nPts;
    }

    void SetPause(bool bIsPause) override
    {
        m_Mutex.lock();
        if(!m_pOutput)
        {
            m_Mutex.unlock();
            return;
        }
        if(bIsPause)
        {
            m_pOutput->suspend();
        }
        else
        {
            m_pOutput->resume();
        }
        m_Mutex.unlock();
    }
    void Clear() override
    {
        m_Mutex.lock();
        if(m_pIO)
        {
            m_pIO->reset();
        }
        m_Mutex.unlock();
    }
};

FFmpegAudioPlay *FFmpegAudioPlay::Get()
{
    static CFFmpegAudioPlay play;
    return &play;
}

FFmpegAudioPlay::FFmpegAudioPlay():
    m_nSampleRate(48000)
  , m_nSampleSize(16)
  , m_nChannels(2)
{

}

FFmpegAudioPlay::~FFmpegAudioPlay()
{

}

int FFmpegAudioPlay::GetSampleRate()
{
    return m_nSampleRate;
}

void FFmpegAudioPlay::SetSampleRate(int nSampleRate)
{
    m_nSampleRate = nSampleRate;
}

int FFmpegAudioPlay::GetSampleSize()
{
    return m_nSampleSize;
}

void FFmpegAudioPlay::SetSampleSize(int nSampleSize)
{
    m_nSampleSize = nSampleSize;
}

int FFmpegAudioPlay::GetChannels()
{
    return m_nChannels;
}

void FFmpegAudioPlay::SetChannels(int nChannels)
{
    m_nChannels = nChannels;
}



