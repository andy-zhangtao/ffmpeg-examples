//
// Created by zhangtao on 2019/9/23.
//

#ifndef IFILTER_FILTER_H
#define IFILTER_FILTER_H

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

using namespace std;

class filter {

private:
    const char *out_file = "/tmp/out.mp4";
    const char *filter_descr = "movie=t.png[wm];[in][wm]overlay=10:20[out]";
    int VideoStreamIndx = -1;

    AVFormatContext *inFormatContext;
    AVFormatContext *outFormatContext;

    AVInputFormat *avInputFormat;
    AVOutputFormat *avOutputFormat;

    AVCodecContext *inCodecContext;
    AVCodecContext *outCodecContext;

    AVCodec *inCodec;
    AVCodec *outCodec;

    AVStream *outStream;
    AVStream *inStream;

    AVFilterGraph *filter_graph;
    AVFilterContext *buffersink_ctx;
    AVFilterContext *buffersrc_ctx;

public:

    filter();

    ~filter();

    int openDevice();

    int init_filter();

    int init_outputfile();

    int CaptureVideoFrames();

};


#endif //IFILTER_FILTER_H
