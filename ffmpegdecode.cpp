#include "ffmpegdecode.h"

extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/pixdesc.h>
#include <libavutil/opt.h>
#include <libavutil/avassert.h>
#include <libavutil/imgutils.h>
}

#include <QDebug>
//#define SOFTDECODE
AVPixelFormat FFmpegDeCode::m_sEnumHWPixfmt;

void FFmpegFreePacket(AVPacket** pPkt)
{
    if(!pPkt || !(*pPkt))
        return;
    av_packet_free(pPkt);
}

void FFmpegFreeFrame(AVFrame** pFrame)
{
    if(!pFrame || !(*pFrame))
        return;
    av_frame_free(pFrame);
}

bool FFmpegDeCode::Open(AVCodecParameters *pPara)
{
    if(!pPara)
        return false;
    Close();
    //解碼器打開
    //找到解碼器
    AVCodec* pCodec = avcodec_find_decoder(pPara->codec_id);
    if(!pCodec)
    {
        avcodec_parameters_free(&pPara);
        qDebug()<<"can't find avcodec decoder:"<<pPara->codec_id;
        return false;
    }
    qDebug()<<"find avcodec decoder:"<<pPara->codec_id;
    m_Mutex.lock();
    m_pCodecContext = avcodec_alloc_context3(pCodec);
    //配置codec context 參數
    avcodec_parameters_to_context(m_pCodecContext,pPara);
    m_pCodecContext->thread_count = 8; //8 threads decode
#ifdef SOFTDECODE
    int nRet = avcodec_open2(m_pCodecContext,pCodec,nullptr); //check
#else
    m_pCodecContext->get_format = FFmpegDeCode::GetHWFormat;
//    const AVCodecHWConfig* pHWConfig = avcodec_get_hw_config(m_pCodecContext->codec,0);
//    qDebug()<<"pixformat:"<<pHWConfig->pix_fmt;
//    qDebug()<<"methods:"<<pHWConfig->methods;
//    qDebug()<<"device_type"<<pHWConfig->device_type;
    av_opt_set_int(m_pCodecContext, "refcounted_frames", 1, 0);
    AVHWDeviceType enumHWType = GetHWDeviceByName("vaapi");
    m_sEnumHWPixfmt = FindFormatByHWType(enumHWType);
    int nRet = HWDecoderInit(enumHWType);
    if(nRet != 0)
        qDebug()<<"Failed to create specified HW device";

    nRet = avcodec_open2(m_pCodecContext,pCodec,nullptr); //check
#endif
    if(nRet != 0)
    {
        avcodec_free_context(&m_pCodecContext);
        m_Mutex.unlock();
        char buf[1024] = {0};
        av_strerror(nRet,buf,sizeof(buf)-1);
        qDebug()<<"avcodec_open2 fail:"<<buf;
        return false;
    }
    m_Mutex.unlock();
    qDebug()<<"avcodec_open2 success";
    return true;
}

FFmpegDeCode::FFmpegDeCode():
    m_bIsAudio(false)
  ,m_pCodecContext(nullptr)
  ,m_pBufferRef(nullptr)
  ,m_nPts(0)
{

}

FFmpegDeCode::~FFmpegDeCode()
{

}

void FFmpegDeCode::Close()
{
    m_Mutex.lock();
    if(m_pCodecContext)
    {
        avcodec_close(m_pCodecContext);
        avcodec_free_context(&m_pCodecContext);
    }
    if(m_pBufferRef)
        av_buffer_unref(&m_pBufferRef);
    m_nPts = 0;
    m_Mutex.unlock();
}

void FFmpegDeCode::Clear()
{
    m_Mutex.lock();
    if(m_pCodecContext)
        avcodec_flush_buffers(m_pCodecContext);
    m_Mutex.unlock();
}

bool FFmpegDeCode::Send(AVPacket *pPkt)
{
    if(!pPkt || pPkt->size<=0 || !pPkt->data)
        return false;
    m_Mutex.lock();
    if(!m_pCodecContext)
    {
        m_Mutex.unlock();
        return false;
    }
    int nRet = avcodec_send_packet(m_pCodecContext,pPkt);
    if(nRet == AVERROR_EOF)
    {
        qDebug()<<"EOF";
    }
    else if(nRet == AVERROR(EAGAIN))
    {
        qDebug()<<"AVERROR(EAGAIN)";
    }
    else if(nRet == AVERROR(ENOMEM))
    {
        qDebug()<<"AVERROR(ENOMEM)";
    }
    m_Mutex.unlock();
    av_packet_free(&pPkt);
    return (nRet == 0)?true:false;
}

AVFrame *FFmpegDeCode::Recv()
{
    m_Mutex.lock();
    if(!m_pCodecContext)
    {
        m_Mutex.unlock();
        return nullptr;
    }
    AVFrame* pFrame = av_frame_alloc();
#ifndef SOFTDECODE
    AVFrame* pSWFrame = av_frame_alloc();
#endif
    AVFrame* pTempFrame = nullptr;
    int nRet = avcodec_receive_frame(m_pCodecContext,pFrame);    
    m_nPts = (pFrame->pts>=INT_MAX || pFrame->pts <=INT_MIN)?0:pFrame->pts;
    //m_nPts = pFrame->pts;
    //qDebug()<<"pts: "<<m_nPts;
    m_Mutex.unlock();
    if(nRet == AVERROR_EOF)
    {
        qDebug()<<"EOF";
    }
    else if(nRet == AVERROR(EAGAIN))
    {
        //qDebug()<<"AVERROR(EAGAIN)";
    }
    else if(nRet == AVERROR(ENOMEM))
    {
        qDebug()<<"AVERROR(ENOMEM)";
    }
#ifdef SOFTDECODE
    if (nRet != 0)
    {
        av_frame_free(&pFrame);
        return nullptr;
    }
#endif
    //qDebug()<<"before frame linesize: "<<pFrame->linesize[0]<<flush;
    //qDebug()<<"before frame format: "<<pFrame->format;
    ///////////////refactor later
#ifndef SOFTDECODE
    if(nRet >= 0)
    {
        if (!pFrame || !pSWFrame)
        {
            fprintf(stderr, "Can not alloc frame\n");
            nRet = AVERROR(ENOMEM);
            av_frame_free(&pFrame);
            av_frame_free(&pSWFrame);
            return nullptr;
        }
        if (nRet == AVERROR(EAGAIN) || nRet == AVERROR_EOF)
        {
            av_frame_free(&pFrame);
            av_frame_free(&pSWFrame);
            return nullptr;
        }
        if (pFrame->format == m_sEnumHWPixfmt)
        {
            /* retrieve data from GPU to CPU */
            if ((nRet = av_hwframe_transfer_data(pSWFrame, pFrame, 0)) < 0)
            {
                qDebug()<<"Error transferring the data to system memory";
                av_frame_free(&pFrame);
                av_frame_free(&pSWFrame);
                return nullptr;
            }
            pTempFrame = pSWFrame;
            av_frame_free(&pFrame);
        }
        else
            pTempFrame = pFrame;
    }
#endif

#ifndef SOFTDECODE
    return pTempFrame;
#else
    return pFrame;
#endif
}

bool FFmpegDeCode::GetIsAudio()
{
    return m_bIsAudio;
}

int FFmpegDeCode::HWDecoderInit(AVHWDeviceType enumType)
{
    int nRet = 0;
    nRet = av_hwdevice_ctx_create(&m_pBufferRef,enumType,nullptr,nullptr,0);
    if(nRet<0)
    {
        qDebug()<<"Failed to create specified HW device";
        return nRet;
    }
    m_pCodecContext->hw_device_ctx = av_buffer_ref(m_pBufferRef);
    return nRet;
}

AVPixelFormat FFmpegDeCode::FindFormatByHWType(const AVHWDeviceType enumType)
{
    enum AVPixelFormat fmt;
    switch (enumType)
    {
        case AV_HWDEVICE_TYPE_VAAPI:
            fmt = AV_PIX_FMT_VAAPI;
            break;
        case AV_HWDEVICE_TYPE_DXVA2:
            fmt = AV_PIX_FMT_DXVA2_VLD;
            break;
        case AV_HWDEVICE_TYPE_D3D11VA:
            fmt = AV_PIX_FMT_D3D11;
            break;
        case AV_HWDEVICE_TYPE_VDPAU:
            fmt = AV_PIX_FMT_VDPAU;
            break;
        case AV_HWDEVICE_TYPE_VIDEOTOOLBOX:
            fmt = AV_PIX_FMT_VIDEOTOOLBOX;
            break;
        default:
            fmt = AV_PIX_FMT_NONE;
            break;
    }
    return fmt;
}

AVPixelFormat FFmpegDeCode::GetHWFormat(AVCodecContext *pCodecctx, const AVPixelFormat *pPixFmt)
{
    const enum AVPixelFormat *p;
    for (p = pPixFmt; *p != -1; p++)
    {
        if (*p == m_sEnumHWPixfmt)
            return *p;
    }
    qDebug()<<"Failed to get HW surface format.";
    return AV_PIX_FMT_NONE;
}

void FFmpegDeCode::SetPts(long long nPts)
{
    m_Mutex.lock();
    m_nPts = nPts;
    m_Mutex.unlock();
}

long long FFmpegDeCode::GetPts()
{
    return m_nPts;
}

AVHWDeviceType FFmpegDeCode::GetHWDeviceByName(QString strDeviceName)
{
    AVHWDeviceType type;
    type = av_hwdevice_find_type_by_name(strDeviceName.toStdString().c_str());
    return type;
}
