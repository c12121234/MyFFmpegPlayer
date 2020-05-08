#ifndef FFMPEGAUDIOPLAY_H
#define FFMPEGAUDIOPLAY_H


class FFmpegAudioPlay
{
public:
    static FFmpegAudioPlay* Get();
    //打開音頻播放
    virtual bool Open() = 0;
    virtual void Close() = 0;
    virtual void Clear() = 0;
    virtual bool Write(const unsigned char* pData,int nDataSize) = 0;
    virtual int GetFree() = 0;
    virtual long long GetNoPlayMs() = 0; //返回緩衝中未播放的時間 ms
    virtual void SetPause(bool bIsPause) = 0;
    FFmpegAudioPlay();
    virtual ~FFmpegAudioPlay();
    int GetSampleRate();
    void SetSampleRate(int nSampleRate);
    int GetSampleSize();
    void SetSampleSize(int nSampleSize);
    int GetChannels();
    void SetChannels(int nChannels);
protected:
    int m_nSampleRate;
    int m_nSampleSize;
    int m_nChannels;
};

#endif // FFMPEGAUDIOPLAY_H
