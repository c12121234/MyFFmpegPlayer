#ifndef FFMPEGAUDIO_H
#define FFMPEGAUDIO_H

#include <QThread>
#include <QMutex>
#include <list>
#include "ffmpegdecodethread.h"
#include <mutex>
struct AVCodecParameters;
struct AVPacket;
class FFmpegDeCode;
class FFmpegAudioPlay;
class FFmpegResample;

class FFmpegAudio : public FFmpegDecodeThread
{
public:
    void run() override;
    FFmpegAudio();
    virtual ~FFmpegAudio();
    virtual bool Open(AVCodecParameters* pPara,int nSampleRate,int nChannels); //打開解碼器 不管成功與否都釋放parameters空間
    void Close() override;//停止線程 清理資源
    void Clear() override;
    long long GetPts();
    void SetPts(long long nPts);
    void SetPause(bool bIsPause);
    bool GetPauseState();
protected:
    FFmpegAudioPlay* m_pAudioPlay;
    FFmpegResample* m_pResample;
    std::mutex m_AudioMutex;
    long long m_nPts; //當前音頻播放的pts
    bool m_bIsPause;
};

#endif // FFMPEGAUDIO_H
