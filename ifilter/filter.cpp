//
// Created by zhangtao on 2019/9/23.
//

#include "filter.h"

filter::filter() {
    av_register_all();
    avfilter_register_all();
    avcodec_register_all();
    avdevice_register_all();

    inFormatContext = NULL;
    outFormatContext = NULL;

    avInputFormat = NULL;
    avOutputFormat = NULL;

    inCodecContext = NULL;
    outCodecContext = NULL;

    inCodec = NULL;
    outCodec = NULL;

    inStream = NULL;
    outStream = NULL;

    cout << "\nall required functions are registered successfully" << endl;
}

filter::~filter() {
    avformat_close_input(&inFormatContext);
    if (!inFormatContext) {
        cout << "\ndevice closed sucessfully";
    } else {
        cout << "\nunable to close the device";
        exit(1);
    }

    avformat_free_context(inFormatContext);
    if (!inFormatContext) {
        cout << "\navformat free successfully";
    } else {
        cout << "\nunable to free avformat context";
        exit(1);
    }

    avformat_close_input(&outFormatContext);
    if (!outFormatContext) {
        cout << "\nout file closed sucessfully";
    } else {
        cout << "\nunable to close the out file";
        exit(1);
    }

    avformat_free_context(outFormatContext);
    if (!outFormatContext) {
        cout << "\nout avformat free successfully";
    } else {
        cout << "\nunable to free out avformat context";
        exit(1);
    }
}

int filter::openDevice() {
    int value = -1;
    inFormatContext = avformat_alloc_context();

    AVDictionary *options = NULL;
    value = av_dict_set(&options, "capture_cursor", "1", 0);
    if (value < 0) {
        cout << "\nerror in setting capture_cursor values";
        exit(1);
    }

    value = av_dict_set(&options, "capture_mouse_clicks", "1", 0);
    if (value < 0) {
        cout << "\nerror in setting capture_cursor values";
        exit(1);
    }

    value = av_dict_set(&options, "pixel_format", "yuyv422", 0);
    if (value < 0) {
        cout << "\nerror in setting pixel_format values";
        exit(1);
    }

    value = av_dict_set(&options, "framerate", "30", 0);
    if (value < 0) {
        cout << "\nerror in setting pixel_format values";
        exit(1);
    }

    value = av_dict_set(&options, "video_size", "1920x1080", 0);
    if (value < 0) {
        cout << "\nerror in setting pixel_format values";
        exit(1);
    }

    value = av_dict_set(&options, "vcodec", "copy", 0);
    if (value < 0) {
        cout << "\nerror in setting pixel_format values";
        exit(1);
    }

    avInputFormat = av_find_input_format("avfoundation");
    value = avformat_open_input(&inFormatContext, "1", avInputFormat, &options);
    if (value != 0) {
        cout << "\nerror in opening input device";
        exit(1);
    }

    AVCodecContext *_inCodecContext = NULL;

    for (int i = 0; i < inFormatContext->nb_streams; i++) // find video stream posistion/index.
    {
        if (inFormatContext->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            AVCodec *dec = avcodec_find_decoder(inFormatContext->streams[i]->codecpar->codec_id);

            _inCodecContext = avcodec_alloc_context3(dec);
            if (!_inCodecContext) {
                cout << "\nCan not find in stream codec";
                exit(1);
            }

            value = avcodec_parameters_to_context(_inCodecContext, inFormatContext->streams[i]->codecpar);
            if (value < 0) {
                av_log(NULL, AV_LOG_ERROR, "Failed to copy decoder parameters to input decoder context "
                                           "for stream #%u\n", i);
                return value;
            }

            _inCodecContext->framerate = av_guess_frame_rate(inFormatContext, inFormatContext->streams[i], NULL);

            inStream = inFormatContext->streams[i];
            VideoStreamIndx = i;
            break;
        }
    }

    if (VideoStreamIndx == -1) {
        cout << "\nCan not find video stream in Device";
        exit(1);
    }


    inCodec = avcodec_find_decoder(_inCodecContext->codec_id);
    if (!inCodec) {
        cout << "\nunable to find the decoder";
        exit(1);
    }

    inCodecContext = avcodec_alloc_context3(inCodec);
    inCodecContext->width = 1920;
    inCodecContext->height = 1080;
    inCodecContext->pix_fmt = AV_PIX_FMT_YVYU422;

    AVDictionary *_options = NULL;

    value = av_dict_set(&_options, "pixel_format", "yuyv422", 0);
    if (value < 0) {
        cout << "\nerror in setting pixel_format values";
        exit(1);
    }

    value = avcodec_open2(inCodecContext, inCodec, &_options);
    if (value < 0) {
        cout << "\nunable to open the av codec. error: " << av_err2str(value);
        exit(1);
    }

    av_dump_format(inFormatContext, 0, "1", 0);

    return value;
}

int filter::init_outputfile() {
    int value = -1;

    AVCodecContext *enc_ctx;

    avOutputFormat = av_guess_format(NULL, out_file, NULL);
    if (!avOutputFormat) {
        cout << "\nGuass OutFormatFormat Error";
        exit(1);
    }

    avformat_alloc_output_context2(&outFormatContext, avOutputFormat, NULL, NULL);
    if (!outFormatContext) {
        cout << "\nAlloc OutFormatContext Error";
        exit(1);
    }

    outCodec = avcodec_find_encoder(AV_CODEC_ID_H264);
    if (!outCodec) {
        cout << "\nCan not find out codec";
        exit(1);
    }

    outStream = avformat_new_stream(outFormatContext, NULL);
    if (!outStream) {
        cout << "Alloc OutStream Error";
        exit(-1);
    }

    outStream->id = outFormatContext->nb_streams - 1;

    outCodecContext = avcodec_alloc_context3(outCodec);
    outCodecContext->codec_id = AV_CODEC_ID_H264;
    outCodecContext->codec_type = AVMEDIA_TYPE_VIDEO;
    outCodecContext->pix_fmt = AV_PIX_FMT_YUV420P;
    outCodecContext->bit_rate = 400000; // 2500000
    outCodecContext->width = 1920;
    outCodecContext->height = 1080;
    outCodecContext->gop_size = 3;
    outCodecContext->max_b_frames = 2;
    outCodecContext->time_base = (AVRational) {1, 25};

    outFormatContext->video_codec = outCodec;

    value = avcodec_open2(outCodecContext, outCodec, NULL);
    if (value < 0) {
        av_log(NULL, AV_LOG_ERROR, "Could not open out codec '%s'.", av_err2str(value));
        exit(1);
    }

    avcodec_parameters_from_context(outStream->codecpar, outCodecContext);
    av_dump_format(outFormatContext, 0, out_file, 1);

    if (!(outFormatContext->oformat->flags & AVFMT_NOFILE)) {
        value = avio_open(&outFormatContext->pb, out_file, AVIO_FLAG_WRITE);
        if (value < 0) {
            av_log(NULL, AV_LOG_ERROR, "Could not open output file '%s'. err: %s", out_file, av_err2str(value));
            return value;
        }
    }

    /* init muxer, write output file header */
    value = avformat_write_header(outFormatContext, NULL);
    if (value < 0) {
        av_log(NULL, AV_LOG_ERROR, "Error occurred when opening output file. error: %s\n", av_err2str(value));
        return value;
    }

    return value;
}

int filter::init_filter() {
    int ret = -1;
    char args[512];
    AVFilterGraph *filter_graph;
//    AVFilterContext *buffer_ctx;
    const AVFilter *buffer;
    AVFilterContext *movie_ctx;
    const AVFilter *movie;
    AVFilterContext *overlay_ctx;
    const AVFilter *overlay;
//    AVFilterContext *buffersink_ctx;
    const AVFilter *buffersink;

    const AVFilter *noise;
    AVFilterContext *noise_ctx;

    AVDictionary *options_dict = NULL;


    filter_graph = avfilter_graph_alloc();
    if (!filter_graph) {
        fprintf(stderr, "Unable to create filter graph.\n");
        return AVERROR(ENOMEM);
    }

    buffer = avfilter_get_by_name("buffer");
    if (!buffer) {
        fprintf(stderr, "Could not find the buffer filter.\n");
        return AVERROR_FILTER_NOT_FOUND;
    }

//    buffer_ctx = avfilter_graph_alloc_filter(filter_graph, buffer, "in");
//    if (!buffer_ctx) {
//        fprintf(stderr, "Could not allocate the buffer instance.\n");
//        return AVERROR(ENOMEM);
//    }

    AVRational time_base = inFormatContext->streams[VideoStreamIndx]->time_base;

    snprintf(args, sizeof(args),
             "video_size=%dx%d:pix_fmt=%d:time_base=%d/%d:pixel_aspect=%d/%d",
             inCodecContext->width, inCodecContext->height, inCodecContext->pix_fmt,
             time_base.num, time_base.den,
             inCodecContext->sample_aspect_ratio.num, inCodecContext->sample_aspect_ratio.den);

//    ret = avfilter_init_str(buffer_ctx, args);
//    if (ret < 0) {
//        fprintf(stderr, "Could not initialize the abuffer filter.\n");
//        return ret;
//    }
    ret = avfilter_graph_create_filter(&buffersrc_ctx, buffer, "in", args, NULL, filter_graph);
    if (ret < 0) {
        av_log(NULL, AV_LOG_ERROR, "Error create buffer filters. err: %s\n", av_err2str(ret));
        return ret;
    }

    movie = avfilter_get_by_name("movie");
    if (!movie) {
        fprintf(stderr, "Could not find the movie filter.\n");
        return AVERROR_FILTER_NOT_FOUND;
    }

    ret = avfilter_graph_create_filter(&movie_ctx, movie, "mi", "t.png", NULL, filter_graph);
    if (ret < 0) {
        av_log(NULL, AV_LOG_ERROR, "Error create movie filters. err: %s\n", av_err2str(ret));
        return ret;
    }

//    movie_ctx = avfilter_graph_alloc_filter(filter_graph, movie, "mi");
//    if (!movie_ctx) {
//        fprintf(stderr, "Could not allocate the movie instance.\n");
//        return AVERROR(ENOMEM);
//    }
//
//    av_dict_set(&options_dict, "filename", "t.png", 0);
//    ret = avfilter_init_dict(movie_ctx, &options_dict);
//    if (ret < 0) {
//        fprintf(stderr, "Could not initialize the movie filter.\n");
//        return ret;
//    }
//
//    av_dict_free(&options_dict);

    overlay = avfilter_get_by_name("overlay");
    if (!overlay) {
        fprintf(stderr, "Could not find the overlay filter.\n");
        return AVERROR_FILTER_NOT_FOUND;
    }

    ret = avfilter_graph_create_filter(&overlay_ctx, overlay, "oi", "40:40", NULL, filter_graph);
    if (ret < 0) {
        av_log(NULL, AV_LOG_ERROR, "Error create overlay filters. err: %s\n", av_err2str(ret));
        return ret;
    }

//    noise = avfilter_get_by_name("noise");
//    if (!noise) {
//        fprintf(stderr, "Could not find the noise filter.\n");
//        return AVERROR_FILTER_NOT_FOUND;
//    }
//
//    ret = avfilter_graph_create_filter(&noise_ctx, noise, "noise", "alls=20:allf=t+u", NULL, filter_graph);
//    if (ret < 0) {
//        av_log(NULL, AV_LOG_ERROR, "Error create noise filters. err: %s\n", av_err2str(ret));
//        return ret;
//    }

//    overlay_ctx = avfilter_graph_alloc_filter(filter_graph, overlay, "oi");
//    if (!overlay_ctx) {
//        fprintf(stderr, "Could not allocate the overlay instance.\n");
//        return AVERROR(ENOMEM);
//    }
//
//    av_dict_set(&options_dict, "x", "10", 0);
//    av_dict_set(&options_dict, "y", "20", 0);
//    ret = avfilter_init_dict(overlay_ctx, &options_dict);
//    if (ret < 0) {
//        fprintf(stderr, "Could not initialize the overlay filter.\n");
//        return ret;
//    }
//
//    av_dict_free(&options_dict);

    buffersink = avfilter_get_by_name("buffersink");
    if (!buffersink) {
        fprintf(stderr, "Could not find the buffersink filter.\n");
        return AVERROR_FILTER_NOT_FOUND;
    }

    ret = avfilter_graph_create_filter(&buffersink_ctx, buffersink, "out", NULL, NULL, filter_graph);
    if (ret < 0) {
        av_log(NULL, AV_LOG_ERROR, "Error create buffersink filters. err: %s\n", av_err2str(ret));
        return ret;
    }

//    buffersink_ctx = avfilter_graph_alloc_filter(filter_graph, buffersink, "out");
//    if (!buffersink_ctx) {
//        fprintf(stderr, "Could not allocate the buffersink instance.\n");
//        return AVERROR(ENOMEM);
//    }
//
//    ret = avfilter_init_str(buffersink_ctx, NULL);
//    if (ret < 0) {
//        fprintf(stderr, "Could not initialize the buffersink instance.\n");
//        return ret;
//    }

    ret = avfilter_link(buffersrc_ctx, 0, overlay_ctx, 0);
    if (ret >= 0)
        ret = avfilter_link(movie_ctx, 0, overlay_ctx, 1);
    if (ret >= 0)
        ret = avfilter_link(overlay_ctx, 0, buffersink_ctx, 0);
    if (ret < 0) {
        av_log(NULL, AV_LOG_ERROR, "Error connecting filters. err: %s\n", av_err2str(ret));
        return ret;
    }

    ret = avfilter_graph_config(filter_graph, NULL);
    if (ret < 0) {
        av_log(NULL, AV_LOG_ERROR, "Error configuring the filter graph. err: %s\n", av_err2str(ret));
        return ret;
    }

    av_log(NULL, AV_LOG_INFO, "%s\n", avfilter_graph_dump(filter_graph, NULL));
//    *graph = filter_graph;
//    *src   = abuffer_ctx;
//    *sink  = abuffersink_ctx;

    return ret;
}


int filter::CaptureVideoFrames() {
    int value = -1;
    AVPacket *ipacket = NULL;
    AVPacket *opacket = NULL;
    ipacket = av_packet_alloc();
    opacket = av_packet_alloc();

    AVFrame *iframe = NULL;
    AVFrame *_oframe = NULL;
    AVFrame *oframe = NULL;
    iframe = av_frame_alloc();
    _oframe = av_frame_alloc();
    oframe = av_frame_alloc();

    int nbytes = av_image_get_buffer_size(
            outCodecContext->pix_fmt,
            outCodecContext->width,
            outCodecContext->height,
            32);
    uint8_t *video_outbuf = (uint8_t *) av_malloc(nbytes);
    if (video_outbuf == NULL) {
        cout << "\nunable to allocate memory";
        exit(1);
    }
    av_log(NULL, AV_LOG_INFO, "outCodecContext->width %3d\n", outCodecContext->width);
    value = av_image_fill_arrays(
            _oframe->data,
            _oframe->linesize,
            video_outbuf,
            AV_PIX_FMT_YUV420P,
            outCodecContext->width,
            outCodecContext->height,
            1); // returns : the size in bytes required for src
    if (value < 0) {
        cout << "\nerror in filling image array";
    }

    av_log(NULL, AV_LOG_INFO, "oFrame->linesize %3d\n", _oframe->linesize);

    SwsContext *swsCtx_;
    av_image_alloc(_oframe->data, _oframe->linesize, outCodecContext->width, outCodecContext->height,
                   AV_PIX_FMT_YUV420P,
                   1);

    swsCtx_ = sws_getContext(inCodecContext->width,
                             inCodecContext->height,
                             inCodecContext->pix_fmt,
                             outCodecContext->width,
                             outCodecContext->height,
                             outCodecContext->pix_fmt,
                             SWS_BICUBIC, NULL, NULL, NULL);

    int ii = 0;
    int no_frames = 200;
    int j = 0;

    while (av_read_frame(inFormatContext, ipacket) >= 0) {
        if (ii++ == no_frames)
            break;
        if (ipacket->stream_index == VideoStreamIndx) {
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

                printf("receive frame %3d\n", inCodecContext->frame_number);

                sws_scale(swsCtx_, iframe->data, iframe->linesize, 0, inCodecContext->height, _oframe->data,
                          _oframe->linesize);

                _oframe->format = AV_PIX_FMT_YUV420P;
                _oframe->width = outCodecContext->width;
                _oframe->height = outCodecContext->height;

                int64_t now = av_gettime();
                const AVRational codecTimebase = outStream->time_base;
                _oframe->pts = av_rescale_q(now, (AVRational) {1, 1000000}, codecTimebase);

                if (value = av_buffersrc_add_frame_flags(buffersrc_ctx, _oframe, AV_BUFFERSRC_FLAG_KEEP_REF); value <
                                                                                                              0) {
                    av_log(NULL, AV_LOG_ERROR, "Error while feeding the filtergraph. error: %s\n", av_err2str(value));
                    break;
                }

                while (value >= 0) {
                    value = av_buffersink_get_frame(buffersink_ctx, oframe);
                    if (value == AVERROR(EAGAIN) || value == AVERROR_EOF)
                        break;

                    value = avcodec_send_frame(outCodecContext, oframe);
                    if (value < 0) {
                        fprintf(stderr, "%s Error sending a frame for encoding\n", av_err2str(value));
                        exit(1);
                    }

                    av_init_packet(opacket);
                    opacket->data = NULL;    // packet data will be allocated by the encoder
                    opacket->size = 0;

                    while (value >= 0) {
                        value = avcodec_receive_packet(outCodecContext, opacket);

                        if (value == AVERROR(EAGAIN) || value == AVERROR_EOF) {
                            break;
                        } else if (value < 0) {
                            fprintf(stderr, "Error during encoding\n");
                            exit(1);
                        }

                        if (opacket->pts != AV_NOPTS_VALUE)
                            opacket->pts = av_rescale_q(opacket->pts, outCodecContext->time_base, outStream->time_base);

                        printf("Write frame %3d (size= %2d) pts=%3d\n", j++, opacket->size / 1000, opacket->pts);

                        if (value = av_interleaved_write_frame(outFormatContext, opacket); value != 0) {
                            cout << "\nerror in writing video frame. " << value << av_err2str(value) << "\n";
                        }


                        av_packet_unref(opacket);
                    }
                }

                av_packet_unref(opacket);
            }
        }
    }

    value = av_write_trailer(outFormatContext);
    if (value < 0) {
        cout << "\nerror in writing av trailer";
        exit(1);
    }

    av_free(video_outbuf);
}