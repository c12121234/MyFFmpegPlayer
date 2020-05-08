#ifndef FFMPEGDEMUX_H
#define FFMPEGDEMUX_H
#include <QObject>
#include <QMutex>
#include <mutex>
struct AVFormatContext;
struct AVPacket;
struct AVCodecParameters;
class FFmpegDemux
{
public:
    FFmpegDemux();
    virtual ~FFmpegDemux();
    virtual bool Open(QString strURL);

    //空間需要調用者釋放 釋放AVPacket對象空間和數據空間 av_packet_free
    virtual AVPacket* Read();
    virtual AVPacket* ReadVideo(); //只讀視訊 音訊丟棄 空間釋放
    //seek位置 0.0~1.0
    virtual bool Seek(double dPos);

    //獲取視訊參數 返回的空間需要清理 avcodec_parameters_free
    virtual AVCodecParameters* CopyVPara();
    //獲取音訊參數 返回的空間需要清理 avcodec_parameters_free
    virtual AVCodecParameters* CopyAPara();
    //清空讀取緩存
    virtual void Clear();
    virtual void Close();

    virtual bool IsAudio(AVPacket* pPkt);

    int GetTotalTimeMs();
    int GetWidth();
    int GetHeight();
    int GetSampleRate();
    int GetChannels();
protected:
    std::mutex m_Mutex;
    AVFormatContext* m_pFormatcontext;
    int m_nVideoStreamIdx;
    int m_nAudioStreamIdx;
    int m_nTotalTimeMs;
    int m_nWidth;
    int m_nHeight;
    int m_nSampleRate;
    int m_nChannels;
};

#endif // FFMPEGDEMUX_H
