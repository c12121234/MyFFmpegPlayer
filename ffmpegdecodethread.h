#ifndef FFMPEGDECODETHREAD_H
#define FFMPEGDECODETHREAD_H

#include <list>
#include <mutex>
#include <QThread>
#include <QMutex>
#include "IVideoCall.h"
struct AVPacket;
struct AVCodecParameters;
class FFmpegDeCode;


class FFmpegDecodeThread : public QThread
{
public:
    FFmpegDecodeThread();
    virtual ~FFmpegDecodeThread();
    virtual void Push(AVPacket* pPkt);
    virtual AVPacket* Pop();//取出一幀數據 並出list 若無則返回nullptr
    virtual void Clear();//清理list
    virtual void Close();//清理資源 停止線程

    int m_nMaxBuffer = 100;
    bool m_bIsExit = false;

protected:
    std::list<AVPacket*>m_PacketsList;
    FFmpegDeCode* m_pDecode;
    std::mutex m_Mutex;
};

#endif // FFMPEGDECODETHREAD_H
