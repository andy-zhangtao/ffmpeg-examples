# Screen Record H.264
>目前在网络传输视频/音频流都一般会采用H.264进行编码，所以尝试调用FFMPEG API完成Mac录屏功能，同时编码为H.264格式。

在上一篇文章中，通过调用FFmpeg API完成了Mac平台下的录屏功能。在本篇中，对上次的录屏进行优化，将采集到的视频流编码为H.264格式，同时设定FPS和分辨率。

因为是对上次录屏功能的优化，因此处理思路仍然分为三部分:

1. 打开输入设备(默认的屏幕设备)
2. 初始化输出设备(mp4文件)
3. 内容转码

和上次使用的API对比，本次主要增加了涉及到H.264参数设定和H.264 pts/dts 设定的API:

1. avcodec_parameters_from_context
2. av_rescale_q

## 初始化输入设备

仍然采用上篇中打开设备的方法:

1. 通过`av_find_input_format("avfoundation")`获取AVInputFormat。
2. 通过`avformat_open_input` 打开指定的屏幕设备。

然后FFmpeg会返回此设备中的数据流，而FFmpeg处理数据流一般都遵循:确定codec(编码 or 解码)->初始化codec上下文参数->打开codec，这三步。 针对输入设备也就是下面的顺序：

```shell
avcodec_find_decoder -> avcodec_alloc_context3 -> avcodec_open2
```

`AVInputFormat`会有多个数据流(视频流/音频流)，所以首先找到需要处理的流:
```c++
codecpar->codec_type == AVMEDIA_TYPE_VIDEO
```

然后依次调用`avcodec_find_decoder`,`avcodec_alloc_context3`和`avcodec_open2`来初始化codec。

## 初始化输出设备

最后是将视频数据编码为H.264，并封装到MP4容器中。所以文件名仍设定为`out.mp4`。

打开输出设备的方法和打开输入设备方法类似:

```c++
avcodec_find_encoder -> avcodec_alloc_context3 -> avcodec_open2 -> avformat_write_header
```

最后的`avformat_write_header`不是必须的，只有当容器格式要求写Header时才会调用。与上篇中不同的时，明确指定输出CodecContext的编码器类型:

```c++
    outCodecContext->codec_id = AV_CODEC_ID_H264;
    outCodecContext->codec_type = AVMEDIA_TYPE_VIDEO;
    outCodecContext->pix_fmt = AV_PIX_FMT_YUV420P;
    outCodecContext->bit_rate = 400000; // 2500000
    outCodecContext->width = 1920;
    outCodecContext->height = 1080;
```

同时H.264对pts和dts有要求，因此需要设定timebase:

```c++
    outCodecContext->time_base = (AVRational) {1, 25};
```

## 转码

有了上次的经验之后，处理转码就会稍显简单。 处理流程大致为:

```shell
    while av_read_frame
        |
        +---> avcodec_send_packet
                    |
                    +----> while avcodec_receive_frame
                                     | 对每一数据帧进行解码
                                     | 通过`sws_scale`进行源帧和目标帧的数据转换
                                     |
                                     +----> avcodec_send_frame
                                               |
                                               +---> while avcodec_receive_packet
                                                             |
                                                             |
                                                             +--->av_interleaved_write_frame （写入到输出设备）
```

转码最重要的环节就是在`avcodec_receive_frame`之后的逻辑。 上面说过H.264对pts有要求，因此这里需要对每一帧添加pts值。
```c++
    int64_t now = av_gettime();
    const AVRational codecTimebase = outStream->time_base;
    oframe->pts = av_rescale_q(now, (AVRational) {1, 1000000}, codecTimebase);
```

在最后写入到输出设备之前，仍需要修改pts值:
```c++
    if (opacket->pts != AV_NOPTS_VALUE)
        opacket->pts = av_rescale_q(opacket->pts, outCodecContext->time_base, outStream->time_base);
```

至此就完成了对视频进行H.264编码的过程。可以看到和上篇处理过程大致相同，唯一不同的地方就是针对H.264编码格式进行了一些特殊处理，除此之外大致流程完全一致。

