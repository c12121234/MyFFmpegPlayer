#ifndef FFMPEGVIDEO_H
#define FFMPEGVIDEO_H

#include <list>
#include <QThread>
#include <QMutex>
#include <mutex>
#include "IVideoCall.h"
#include "ffmpegdecodethread.h"
struct AVPacket;
struct AVCodecParameters;
class FFmpegDeCode;

class FFmpegVideo : public FFmpegDecodeThread
{
public:
    FFmpegVideo();
    virtual ~FFmpegVideo();
    virtual bool Open(AVCodecParameters* pPara,IVideoCall* pVideoCall,int nWidth,int nHeight); //打開解碼器 不管成功與否都釋放parameters空間
    //解碼pts 如果接收到的解碼數據 pts>=nSeekPts return true 並且顯示畫面
    virtual bool RepaintPts(AVPacket* pPkt,long long nSeekPts);
    void run() override;

    void SetSynPts(long long nPts);
    void SetPause(bool bIsPause);
    bool GetPauseState();

protected:
    std::mutex m_VideoMutex;
    IVideoCall* m_pVideoCall;
    long long m_nSynPts; //同步時間 由外部傳入
    bool m_bIsPause;
};

#endif // FFMPEGVIDEO_H
