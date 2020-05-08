#ifndef FFMPEGDECODE_H
#define FFMPEGDECODE_H

#include <QMutex>
#include <mutex>
extern "C"
{
#include "libavutil/hwcontext.h"
}
struct AVCodecParameters;
struct AVCodecContext;
struct AVPacket;
struct AVFrame;
struct AVBufferRef;

extern void FFmpegFreePacket(AVPacket** pPkt);
extern void FFmpegFreeFrame(AVFrame** pFrame);

static const char *const hw_type_names[] = {
    "cuda", // = [AV_HWDEVICE_TYPE_CUDA]
    "drm",// = [AV_HWDEVICE_TYPE_DRM]
    "dxva2",// = [AV_HWDEVICE_TYPE_DXVA2]
    "d3d11va",// = [AV_HWDEVICE_TYPE_D3D11VA]
    "opencl",// = [AV_HWDEVICE_TYPE_OPENCL]
    "qsv",// = [AV_HWDEVICE_TYPE_QSV]
    "vaapi",// = [AV_HWDEVICE_TYPE_VAAPI]
    "vdpau",// = [AV_HWDEVICE_TYPE_VDPAU]
    "videotoolbox",// = [AV_HWDEVICE_TYPE_VIDEOTOOLBOX]
    "mediacodec",// = [AV_HWDEVICE_TYPE_MEDIACODEC]
};

class FFmpegDeCode
{
public:
    virtual bool Open(AVCodecParameters* pPara); //打開解碼器 不管成功與否都釋放parameters空間
    FFmpegDeCode();
    virtual ~FFmpegDeCode();
    virtual void Close();
    virtual void Clear();

    //發送到解碼thread 不管成功與否都釋放pPkt空間(對象和媒體)
    virtual bool Send(AVPacket* pPkt);
    //獲取解碼數據 一次Send可能需要多次Recv  ///獲取緩衝中的數據--> send null再Recv多次
    //每次複製一份 由調用者釋放av_frame_free
    virtual AVFrame* Recv();

    bool GetIsAudio();

    int HWDecoderInit(AVHWDeviceType enumType);
    AVPixelFormat FindFormatByHWType(const enum AVHWDeviceType enumType);
    AVHWDeviceType GetHWDeviceByName(QString strDeviceName);

    static AVPixelFormat m_sEnumHWPixfmt;
    static AVPixelFormat GetHWFormat(AVCodecContext *pCodecctx,const enum AVPixelFormat *pPixFmt);

    void SetPts(long long nPts);
    long long GetPts();
protected:
    bool m_bIsAudio;
    std::mutex m_Mutex;
    AVCodecContext* m_pCodecContext;
    AVBufferRef* m_pBufferRef;
    AVHWDeviceType m_enumHWDeviceType;
    long long m_nPts; //當前解碼到的pts
};

#endif // FFMPEGDECODE_H
