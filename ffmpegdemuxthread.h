#ifndef FFMPEGDEMUXTHREAD_H
#define FFMPEGDEMUXTHREAD_H

#include <QThread>
#include <QMutex>
#include "IVideoCall.h"
#include <mutex>
class FFmpegDemux;
class FFmpegVideo;
class FFmpegAudio;

class FFmpegDemuxThread : public QThread
{
public:
    FFmpegDemuxThread();
    virtual ~FFmpegDemuxThread();
    virtual bool Open(const char* strUrl,IVideoCall* pVideoCall); //創建對象並打開
    virtual void Start(); //啟動所有線程
    virtual void Close(); //關閉線程清理資源
    virtual void Clear();
    virtual void Seek(double dPos);
    void run() override;

    long long GetPts();
    long long GetTotalMs();
    void SetPause(bool bIsPause);
    bool GetPauseState();
protected:
    FFmpegDemux* m_pDemux;
    FFmpegVideo* m_pVideoThread;
    FFmpegAudio* m_pAudioThread;
    std::mutex m_Mutex;
    bool m_bIsExit;
    bool m_bIsPause;
    long long m_nPts;
    long long m_nTotalMs;
};

#endif // FFMPEGDEMUXTHREAD_H
