#include "ffmpegdemux.h"

extern "C"{
#include "libavformat/avformat.h"
#include "libavcodec/avcodec.h"
}

#include <QDebug>

double static r2d(AVRational r)
{
    return r.den == 0 ? 0 :static_cast<double>(r.num)/static_cast<double>(r.den);
}

FFmpegDemux::FFmpegDemux():
    m_pFormatcontext(nullptr)
  ,m_nVideoStreamIdx(0)
  ,m_nAudioStreamIdx(0)
  ,m_nTotalTimeMs(0)
  ,m_nWidth(0)
  ,m_nHeight(0)
  ,m_nSampleRate(0)
  ,m_nChannels(0)
{
    static bool s_bFirst = true;
    static QMutex s_mutex;
    s_mutex.lock();
    if(s_bFirst)
    {
        av_register_all();
        avformat_network_init();
        s_bFirst = false;
    }
    s_mutex.unlock();
}

FFmpegDemux::~FFmpegDemux()
{

}

bool FFmpegDemux::Open(QString strURL)
{
    Close();
    //for open local file
    av_register_all();
    //for rtsp rtmp http stream video protocol.
    avformat_network_init();
    //decode encode codec
    avcodec_register_all();

    //parameter setting
    AVDictionary* opt = nullptr;
    av_dict_set(&opt,"rtsp_transport","tcp",0); //rtsp以tcp協議開啟
    av_dict_set(&opt,"max_delay","500",0); //網路延遲時間
    m_Mutex.lock();
    int ret = avformat_open_input(
                &m_pFormatcontext,
                strURL.toStdString().c_str(),
                0,
                &opt //參數設定 如rtsp delay time
                );
    if(ret!=0)
    {
        m_Mutex.unlock();
        char buff[1024] = {0};
        av_strerror(ret,buff,sizeof(buff)-1);
        qInfo()<<"open "<<strURL<<"failed, "<<buff;
        return false;
    }
    else
        qInfo()<<"open "<<strURL<<"sucess";
    //獲取stream info
    ret = avformat_find_stream_info(m_pFormatcontext,nullptr);
    //時長
    m_nTotalTimeMs = m_pFormatcontext->duration/(AV_TIME_BASE/1000);
    qInfo()<<"total time ms = "<<m_nTotalTimeMs;
    //dump 詳細訊息
    av_dump_format(m_pFormatcontext,0,strURL.toStdString().c_str(),0);
    m_nVideoStreamIdx = av_find_best_stream(m_pFormatcontext,AVMEDIA_TYPE_VIDEO,-1,-1,nullptr,0);
    AVStream* as = m_pFormatcontext->streams[m_nVideoStreamIdx];
    m_nWidth = as->codecpar->width;
    m_nHeight = as->codecpar->height;
    qDebug()<<"duration:"<<m_pFormatcontext->streams[m_nVideoStreamIdx]->duration;
    qDebug()<<"=====================================================";
    qDebug()<<"codec_id: "<<as->codecpar->codec_id;
    //AVSampleFormat
    qDebug()<<"format: "<<as->codecpar->format;
    qDebug()<<m_nVideoStreamIdx<<": "<<"video info";    
    qDebug()<<"width:"<<as->codecpar->width;  //此屬性未必有資訊
    qDebug()<<"height:"<<as->codecpar->height;//此屬性未必有資訊
    qDebug()<<"video fps = "<<r2d(as->avg_frame_rate); //avg_frame_rate為一數組 紀錄分子和分母以確保精確度

    //獲取音訊index
    m_nAudioStreamIdx = av_find_best_stream(m_pFormatcontext,AVMEDIA_TYPE_AUDIO,-1,-1,nullptr,0);
    as = m_pFormatcontext->streams[m_nAudioStreamIdx];
    m_nSampleRate = as->codecpar->sample_rate;
    m_nChannels = as->codecpar->channels;
    qDebug()<<"duration:"<<m_pFormatcontext->streams[m_nAudioStreamIdx]->duration;
    qDebug()<<"=====================================================";
    qDebug()<<"codec_id: "<<as->codecpar->codec_id;
    qDebug()<<"format: "<<as->codecpar->format;
    qDebug()<<m_nAudioStreamIdx<<": "<<"audio info";
    qDebug()<<"sample rate: "<<as->codecpar->sample_rate;
    qDebug()<<"channels: "<<as->codecpar->channels;
    qDebug()<<"frame size:"<<as->codecpar->frame_size; //單通道定量樣本數量
    //1024(frame size) * 2(雙通道) * format(以8bit為基準) 4 = 8192
    m_Mutex.unlock();
    return true;
}

AVPacket *FFmpegDemux::Read()
{
    m_Mutex.lock();
    if(!m_pFormatcontext)
    {
        m_Mutex.unlock();
        return nullptr;
    }
    AVPacket* pPkt = av_packet_alloc();
    //讀取1 frame 並分配空間
    int nRet = av_read_frame(m_pFormatcontext,pPkt);
    if(nRet!=0)
    {
        m_Mutex.unlock();
        av_packet_free(&pPkt);
        return nullptr;
    }
    //pts/dts turn to ms
    pPkt->pts = pPkt->pts*(1000 * (r2d(m_pFormatcontext->streams[pPkt->stream_index]->time_base)));
    pPkt->dts = pPkt->dts*(1000 * (r2d(m_pFormatcontext->streams[pPkt->stream_index]->time_base)));
    //qDebug()<<"pts:"<<pPkt->pts<<" "<<flush;
    m_Mutex.unlock();
    return pPkt;
}

AVPacket *FFmpegDemux::ReadVideo()
{
    m_Mutex.lock();
    if(!m_pFormatcontext)
    {
        m_Mutex.unlock();
        return nullptr;
    }
    m_Mutex.unlock();
    AVPacket* pPkt = nullptr;
    for(int i = 0;i<20;++i) //magic number 20 ,prevent block.
    {
        pPkt = Read();
        if(!pPkt)
            break;
        if(pPkt->stream_index == m_nVideoStreamIdx)
        {
            break;
        }
        av_packet_free(&pPkt);
    }
    return pPkt;
}

bool FFmpegDemux::Seek(double dPos)
{
    m_Mutex.lock();
    if(!m_pFormatcontext)
    {
        m_Mutex.unlock();
        return false;
    }
    //清理讀取緩衝
    avformat_flush(m_pFormatcontext);
    long long nSeekPos = 0;
    nSeekPos = m_pFormatcontext->streams[m_nVideoStreamIdx]->duration * dPos;
    int nRet = av_seek_frame(m_pFormatcontext,m_nVideoStreamIdx,nSeekPos,AVSEEK_FLAG_BACKWARD|AVSEEK_FLAG_FRAME);
    m_Mutex.unlock();
    return (nRet >= 0)?true:false;

    //注意 此seek功能只會跳到關鍵frame 若要準確seek到位置必須和解碼模組關聯 故不在此作業
}

AVCodecParameters *FFmpegDemux::CopyVPara()
{
    m_Mutex.lock();
    if(!m_pFormatcontext)
    {
        m_Mutex.unlock();
        return nullptr;
    }
    AVCodecParameters* pA = avcodec_parameters_alloc();
    avcodec_parameters_copy(pA,m_pFormatcontext->streams[m_nVideoStreamIdx]->codecpar);
    m_Mutex.unlock();
    return pA;
}

AVCodecParameters *FFmpegDemux::CopyAPara()
{
    m_Mutex.lock();
    if(!m_pFormatcontext)
    {
        m_Mutex.unlock();
        return nullptr;
    }
    AVCodecParameters* pA = avcodec_parameters_alloc();
    avcodec_parameters_copy(pA,m_pFormatcontext->streams[m_nAudioStreamIdx]->codecpar);
    m_Mutex.unlock();
    return pA;
}

void FFmpegDemux::Clear()
{
    m_Mutex.lock();
    if(!m_pFormatcontext)
    {
        m_Mutex.unlock();
        return;
    }
    avformat_flush(m_pFormatcontext);
    m_Mutex.unlock();
}

void FFmpegDemux::Close()
{
    m_Mutex.lock();
    if(!m_pFormatcontext)
    {
        m_Mutex.unlock();
        return;
    }
    avformat_close_input(&m_pFormatcontext);
    //媒體總時長(毫秒)
    m_nTotalTimeMs = 0;
    m_Mutex.unlock();
}

bool FFmpegDemux::IsAudio(AVPacket *pPkt)
{
    if(!pPkt)
        return false;
    if(pPkt->stream_index == m_nVideoStreamIdx)
        return false;
    return true;
}

int FFmpegDemux::GetTotalTimeMs()
{
    return m_nTotalTimeMs;
}

int FFmpegDemux::GetWidth()
{
    return m_nWidth;
}

int FFmpegDemux::GetHeight()
{
    return m_nHeight;
}

int FFmpegDemux::GetSampleRate()
{
    return m_nSampleRate;
}

int FFmpegDemux::GetChannels()
{
    return m_nChannels;
}
