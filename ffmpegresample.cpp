#include "ffmpegresample.h"
extern "C"
{
#include "libswresample/swresample.h"
#include "libavcodec/avcodec.h"
}
#include <QDebug>

FFmpegResample::FFmpegResample():
    m_pSwrctx(nullptr)
  , m_nOutFormat(1)
{

}

FFmpegResample::~FFmpegResample()
{

}

bool FFmpegResample::Open(AVCodecParameters *pPara, bool bClear)
{
    if(!pPara)
        return false;
    m_Mutex.lock();
    //如果m_pSwrctx為null,會自動分配空間
    m_pSwrctx = swr_alloc_set_opts(m_pSwrctx,
                av_get_default_channel_layout(2),                 //輸出格式
                (AVSampleFormat)m_nOutFormat,                     //輸出樣本格式
                pPara->sample_rate,                               //輸出採樣率
                av_get_default_channel_layout(pPara->channels),   //輸入格式
                (AVSampleFormat)pPara->format,
                pPara->sample_rate,
                0,0);
    if(bClear)
        avcodec_parameters_free(&pPara);
    int nRet = swr_init(m_pSwrctx);
    m_Mutex.unlock();
    if(nRet!=0)
    {
        char buff[1024] = {0};
        av_strerror(nRet,buff,sizeof(buff)-1);
        qInfo()<<"swr_init failed! :"<<buff;
        return false;
    }
    unsigned char* pPCM = nullptr;
    return true;
}

void FFmpegResample::Close()
{
    m_Mutex.lock();
    if(m_pSwrctx)
        swr_free(&m_pSwrctx);
    m_Mutex.unlock();
}

int FFmpegResample::Resample(AVFrame *pInputData, unsigned char *pOutputData)
{
    if(!pInputData)
        return 0;
    if(!pOutputData)
    {
        av_frame_free(&pInputData);
        return 0;
    }
    uint8_t* data[2] = {0};
    data[0] = pOutputData;
    int nRet = swr_convert(m_pSwrctx,
                   data,pInputData->nb_samples,
                   (const uint8_t**)pInputData->data,pInputData->nb_samples);
    int nOutSize = nRet * pInputData->channels * av_get_bytes_per_sample((AVSampleFormat)m_nOutFormat);
    av_frame_free(&pInputData);
    if(nRet<=0)
        return nRet;

    return nOutSize;
}
