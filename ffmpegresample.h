#ifndef FFMPEGRESAMPLE_H
#define FFMPEGRESAMPLE_H

#include <QMutex>
#include <mutex>
struct AVCodecParameters;
struct SwrContext;
struct AVFrame;
class FFmpegResample
{
public:
    FFmpegResample();
    ~FFmpegResample();
    //輸出參數和輸入參數一致 除了採樣格式 輸出為S16 釋放pPara
    virtual bool Open(AVCodecParameters* pPara,bool bClear = false);
    virtual void Close();
    //返回重採樣後的大小 不管成功與否都釋放pInputData
    virtual int Resample(AVFrame* pInputData,unsigned char* pOutputData);
protected:
    std::mutex m_Mutex;
    SwrContext* m_pSwrctx;
    int m_nOutFormat; //AVSampleFormat AV_SAMPLE_FMT_S16 預設為1
};

#endif // FFMPEGRESAMPLE_H
