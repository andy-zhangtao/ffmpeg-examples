//
// Created by zhangtao on 2019/9/27.
//

#include "transfer.h"

const char *co_int_file = "/Users/zhangtao/tmp/60.mp4";
const char *co_out_file = "/Users/zhangtao/tmp/60-fade.mp4";

int copyOpenInput() {
    int ret;
    inputFormatContext = NULL;
    avformat_open_input(&inputFormatContext, co_int_file, NULL, NULL);
    if (!inputFormatContext) {
        av_log(NULL, AV_LOG_ERROR, "Cannot open input file\n");
        return ret;
    }


    ret = avformat_find_stream_info(inputFormatContext, NULL);
    if (ret < 0) {
        av_log(NULL, AV_LOG_ERROR, "Cannot find stream info\n");
        return ret;
    }


    stream_ctx = av_mallocz_array(inputFormatContext->nb_streams, sizeof(*stream_ctx));

    for (int i = 0; i < inputFormatContext->nb_streams; i++) {

        AVStream *inStream = inputFormatContext->streams[i];

        if (inStream->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            videoIdx = i;
            AVCodec *dec = avcodec_find_decoder(inStream->codecpar->codec_id);
            if (!dec) {
                av_log(NULL, AV_LOG_ERROR, "Cannot find decodec info\n");
                return ret;
            }

            AVCodecContext *dec_ctx = avcodec_alloc_context3(dec);
            if (!dec_ctx) {
                av_log(NULL, AV_LOG_ERROR, "Cannot generate decode context info\n");
                return ret;
            }

            ret = avcodec_parameters_to_context(dec_ctx, inStream->codecpar);
            if (ret < 0) {
                av_log(NULL, AV_LOG_ERROR, "Cannot copy parameter to  decode context info\n");
                return ret;
            }

            dec_ctx->framerate = av_guess_frame_rate(inputFormatContext, inStream, NULL);
            dec_ctx->time_base = inStream->time_base;

            ret = avcodec_open2(dec_ctx, dec, NULL);
            if (ret < 0) {
                av_log(NULL, AV_LOG_ERROR, "Cannot open decode \n");
                return ret;
            }

            stream_ctx[0].dec_ctx = dec_ctx;
        }
    }

    return 0;
}


int copyOpenOutput() {
    int ret;
    outputFormatContext = NULL;

    avformat_alloc_output_context2(&outputFormatContext, NULL, NULL, co_out_file);
    if (!outputFormatContext) {
        av_log(NULL, AV_LOG_ERROR, "Cannot open output file\n");
        return ret;
    }

    AVStream *outStream = avformat_new_stream(outputFormatContext, NULL);
    if (!outStream) {
        av_log(NULL, AV_LOG_ERROR, "Cannot open output stream\n");
        return ret;
    }

    AVStream *inStream = inputFormatContext->streams[videoIdx];
    AVCodecContext *dec_ctx = stream_ctx[0].dec_ctx;

    AVCodec *enc = avcodec_find_encoder(inStream->codecpar->codec_id);
    if (!enc) {
        av_log(NULL, AV_LOG_ERROR, "Cannot open encodec\n");
        return ret;
    }

    AVCodecContext *enc_ctx = avcodec_alloc_context3(enc);
    if (!enc_ctx) {
        av_log(NULL, AV_LOG_ERROR, "Cannot open encodec content\n");
        return ret;
    }

    enc_ctx->width = dec_ctx->width;
    enc_ctx->height = dec_ctx->height;
    enc_ctx->framerate = dec_ctx->framerate;
    enc_ctx->time_base = dec_ctx->time_base;
    enc_ctx->sample_aspect_ratio = dec_ctx->sample_aspect_ratio;
    enc_ctx->pix_fmt = dec_ctx->pix_fmt;

    outStream->time_base = enc_ctx->time_base;

    if (outputFormatContext->oformat->flags & AVFMT_GLOBALHEADER)
        enc_ctx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;


    ret = avcodec_open2(enc_ctx, enc, NULL);
    if (ret < 0) {
        av_log(NULL, AV_LOG_ERROR, "Cannot open encodec content\n");
    }

    ret = avcodec_parameters_from_context(outStream->codecpar, enc_ctx);
    if (ret < 0) {
        av_log(NULL, AV_LOG_ERROR, "Cannot open copy paramter to  encodec content\n");
        return ret;
    }

    stream_ctx[0].enc_ctx = enc_ctx;
    av_dump_format(outputFormatContext, 0, co_out_file, 1);

    if (!(outputFormatContext->oformat->flags & AVFMT_NOFILE)) {
        ret = avio_open(&outputFormatContext->pb, co_out_file, AVIO_FLAG_WRITE);
        if (ret < 0) {
            av_log(NULL, AV_LOG_ERROR, "Could not open output file '%s'", co_out_file);
            return ret;
        }
    }

    /* init muxer, write output file header */
    ret = avformat_write_header(outputFormatContext, NULL);
    if (ret < 0) {
        av_log(NULL, AV_LOG_ERROR, "Error occurred when opening output file\n");
        return ret;
    }

    return ret;
}

int copy() {
    av_log_set_level(AV_LOG_VERBOSE);

    int ret, i = 0;

    ret = copyOpenInput();
    if (ret < 0)
        return ret;

    ret = copyOpenOutput();
    if (ret < 0)
        return ret;

    AVPacket packet = {.data = NULL, .size = 0};

    AVFrame *frame = NULL;
    int got_frame;

    while (1) {
        ret = av_read_frame(inputFormatContext, &packet);
        if (ret < 0)
            break;

        if (packet.stream_index == videoIdx) {
            frame = av_frame_alloc();

            av_packet_rescale_ts(&packet,
                                 inputFormatContext->streams[videoIdx]->time_base,
                                 stream_ctx[0].dec_ctx->time_base);

            ret = avcodec_decode_video2(stream_ctx[0].dec_ctx, frame, &got_frame, &packet);
            if (ret < 0) {
                av_frame_free(&frame);
                av_log(NULL, AV_LOG_ERROR, "Decoding failed. %s\n", av_err2str(ret));
                continue;
            }

            if (got_frame) {
                frame->pts = frame->best_effort_timestamp;
                frame->pict_type = AV_PICTURE_TYPE_NONE;

                AVPacket enc_pkt;
                enc_pkt.data = NULL;
                enc_pkt.size = 0;

                av_init_packet(&enc_pkt);

                ret = avcodec_encode_video2(stream_ctx[0].enc_ctx, &enc_pkt, frame, &got_frame);
                if (ret < 0) {
                    av_log(NULL, AV_LOG_ERROR, "Encoding failed\n");
                }

                av_frame_free(&frame);

                enc_pkt.stream_index = videoIdx;
                av_packet_rescale_ts(&enc_pkt,
                                     stream_ctx[0].enc_ctx->time_base,
                                     outputFormatContext->streams[videoIdx]->time_base);
                ret = av_interleaved_write_frame(outputFormatContext, &enc_pkt);
                if (ret < 0)
                    av_log(NULL, AV_LOG_ERROR, "Cannot write packet to output file. %s\n", av_err2str(ret));
                i++;
            }
        }

        av_packet_unref(&packet);
    }

    av_packet_unref(&packet);
    av_write_trailer(outputFormatContext);
    av_log(NULL, AV_LOG_VERBOSE, "write %d frame to output file\n", i);
    return 0;
}