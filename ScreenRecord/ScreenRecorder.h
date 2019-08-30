//
// Created by zhangtao on 2019/8/30.
//

#ifndef SCREENRECORD_SCREENRECORDER_H
#define SCREENRECORD_SCREENRECORDER_H


#include <iostream>

#ifdef __cplusplus
extern "C"
{
#endif
#include "libavcodec/avcodec.h"
#include "libavcodec/avfft.h"
#include "libavdevice/avdevice.h"
#include "libavfilter/avfilter.h"
#include "libavfilter/buffersink.h"
#include "libavfilter/buffersrc.h"
#include "libavformat/avformat.h"
#include "libavformat/avio.h"
#include "libavutil/opt.h"
#include "libavutil/common.h"
#include "libavutil/channel_layout.h"
#include "libavutil/imgutils.h"
#include "libavutil/mathematics.h"
#include "libavutil/samplefmt.h"
#include "libavutil/time.h"
#include "libavutil/opt.h"
#include "libavutil/pixdesc.h"
#include "libavutil/file.h"
#include "libswscale/swscale.h"
#ifdef __cplusplus
};
#endif

class ScreenRecorder {
private:
    AVInputFormat *pAVInputFormat;
    AVOutputFormat *output_format;

    AVCodecContext *pAVCodecContext;

    AVFormatContext *pAVFormatContext;

    AVFrame *pAVFrame;
    AVFrame *outFrame;

    AVCodec *pAVCodec;
    AVCodec *outAVCodec;

    AVPacket *pAVPacket;
    AVPacket *outPacket;

    AVDictionary *options;

    AVOutputFormat *outAVOutputFormat;
    AVFormatContext *outAVFormatContext;
    AVCodecContext *outAVCodecContext;

    AVStream *video_st;
    AVFrame *outAVFrame;

    const char *dev_name;
    const char *output_file;

    double video_pts;

    int out_size;
    int codec_id;
    int value;
    int VideoStreamIndx;

public:

    ScreenRecorder();

    ~ScreenRecorder();

    int openDevice();

    int init_outputfile();

    int CaptureVideoFrames();

};

#endif //SCREENRECORD_SCREENRECORDER_H
