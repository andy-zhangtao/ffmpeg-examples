//
// Created by zhangtao on 2019/9/27.
//

#ifndef FADETEMPLATE_COPY_H
#define FADETEMPLATE_COPY_H

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
#include "libavutil/timestamp.h"

AVFormatContext *inputFormatContext;
AVFormatContext *outputFormatContext;

typedef struct StreamContext {
    AVCodecContext *dec_ctx;
    AVCodecContext *enc_ctx;
} StreamContext;
static StreamContext *stream_ctx;

int videoIdx;

int copyOpenInput();

int copyOpenOutput();

int copy();

#endif //FADETEMPLATE_COPY_H
