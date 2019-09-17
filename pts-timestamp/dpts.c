//
// Created by zhangtao on 2019/9/16.
//
#include <stdio.h>
#include "dpts.h"

int VideoStreamIndx = -1;
const char *out_file = "/Users/zhangtao/tmp/small.mp4";

int init() {
    av_register_all();
    avfilter_register_all();
    avcodec_register_all();
    avdevice_register_all();

    inFormatContext = NULL;
    inCodecContext = NULL;
    inCodec = NULL;

    printf("\nall required functions are registered successfully");
    return 0;
}

int initInput() {
    int value = -1;

    value = avformat_open_input(&inFormatContext, out_file, NULL, NULL);
    if (value) {
        printf("\nAlloc InputFormatContext Error. %s", av_err2str(value));
        exit(1);
    }

    if ((value = avformat_find_stream_info(inFormatContext, NULL)) < 0) {
        av_log(NULL, AV_LOG_ERROR, "Cannot find stream information\n");
        return value;
    }

    AVRational _time_base;

    for (int i = 0; i < inFormatContext->nb_streams; i++) // find video stream posistion/index.
    {

        if (inFormatContext->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            AVCodec *dec = avcodec_find_decoder(inFormatContext->streams[i]->codecpar->codec_id);
            printf("%d\n", dec->id);

            inCodecContext = avcodec_alloc_context3(dec);
            if (!inCodecContext) {
                printf("\nCan not find in stream codec");
                exit(1);
            }

            inCodecContext->flags |= AV_CODEC_FLAG_TRUNCATED;
            inCodecContext->pix_fmt = AV_PIX_FMT_YVYU422;
            _time_base = inFormatContext->streams[i]->time_base;

            value = avcodec_parameters_to_context(inCodecContext, inFormatContext->streams[i]->codecpar);
            if (value < 0) {
                av_log(NULL, AV_LOG_ERROR, "Failed to copy decoder parameters to input decoder context "
                                           "for stream #%u\n", i);
                return value;
            }

            inCodecContext->framerate = av_guess_frame_rate(inFormatContext, inFormatContext->streams[i], NULL);

            VideoStreamIndx = i;
            break;
        }
    }

    if (VideoStreamIndx == -1) {
        printf("Can not find video stream \n");
        exit(1);
    }


    value = avcodec_open2(inCodecContext, inCodec, NULL);
    if (value < 0) {
        printf("\n%d\n", __LINE__);
        printf("unable to open the av codec. error: ", av_err2str(value));
        exit(1);
    }

    inCodecContext->time_base = _time_base;
    printf("%d %d\n", inCodecContext->time_base.num, inCodecContext->time_base.den);

    av_dump_format(inFormatContext, 0, out_file, 0);

    printf("%"PRId64" %f \n", inFormatContext->duration, av_q2d(inCodecContext->time_base));

    return 0;
}

int analysis() {

    AVPacket *ipacket = NULL;
    ipacket = av_packet_alloc();

    AVFrame *iframe = NULL;
    iframe = av_frame_alloc();

    int i = 0;
    int value = -1;

    AVRational _time_base = inFormatContext->streams[VideoStreamIndx]->time_base;

    while (av_read_frame(inFormatContext, ipacket) >= 0) {

        if (ipacket->stream_index == VideoStreamIndx) {
            i++;
            value = avcodec_send_packet(inCodecContext, ipacket);
            if (value < 0) {
                fprintf(stderr, "Error sending a packet for decoding\n");
                exit(1);
            }

            while (value >= 0) {
                value = avcodec_receive_frame(inCodecContext, iframe);
                if (value == AVERROR(EAGAIN) || value == AVERROR_EOF) {
                    break;
                } else if (value < 0) {
                    fprintf(stderr, "Error during decoding\n");
                    exit(1);
                }

                printf("receive frame %d pts: %ld timestamp: %f \n",
                       inCodecContext->frame_number,
                       iframe->pts,
                       iframe->pts * av_q2d(_time_base)
                );
            }
        }
    }

    return 0;
}