### Screen Record
调用FFMPEG Device API完成Mac录屏功能。

调用FFMPEG提供的API来完成录屏功能，大致的思路是:

1. 打开输入设备.
2. 打开输出设备.
3. 从输入设备读取视频流，然后经过解码->编码，写入到输出设备.

```
+--------------------------------------------------------------+
|   +---------+    decode               +------------+         |
|   | Input   | ----------read -------->|  Output    |         |
|   +---------+                 encode  +------------+         |
+--------------------------------------------------------------+

```

因此主要使用的API就是:

1. avformat_open_input
2. avcodec_find_decoder
3. av_read_frame
4. avcodec_send_packet/avcodec_receive_frame
5. avcodec_send_frame/avcodec_receive_packet

* 打开输入设备

如果使用FFmpeg提供的`-list_devices` 命令可以查询到当前支持的设备，其中分为两类:
- AVFoundation video devices
- AVFoundation audio devices

AVFoundation 是Mac特有的基于时间的多媒体处理框架。本次是演示录屏功能，因此忽略掉audio设备，只考虑video设备。在`avfoundation.m`文件中没有发现可以程序化读取设备的API。FFmpeg官方也说明没有程序化读取设备的方式，通用方案是解析日志来获取设备(https://trac.ffmpeg.org/wiki/DirectShow#Howtoprogrammaticallyenumeratedevices)，下一篇再研究如何通过日志获取当前支持的设备，本次就直接写死设备ID。

1. 获取指定格式的输入设备

```c++
    pAVInputFormat = av_find_input_format("avfoundation");
```

通过指定格式名称获取到AVInputFormat结构体。

2. 打开设备

```c++
    value = avformat_open_input(&pAVFormatContext, "1", pAVInputFormat, &options);
    if (value != 0) {
        cout << "\nerror in opening input device";
        exit(1);
    }
```

"1"指代的是设备ID。 options是打开设备时输入参数，

```c++
    // 记录鼠标
    value = av_dict_set(&options, "capture_cursor", "1", 0);
    if (value < 0) {
        cout << "\nerror in setting capture_cursor values";
        exit(1);
    }

    // 记录鼠标点击事件
    value = av_dict_set(&options, "capture_mouse_clicks", "1", 0);
    if (value < 0) {
        cout << "\nerror in setting capture_mouse_clicks values";
        exit(1);
    }

    // 指定像素格式
    value = av_dict_set(&options, "pixel_format", "yuyv422", 0);
    if (value < 0) {
        cout << "\nerror in setting pixel_format values";
        exit(1);
    }
```

通过value值判断设备是否正确打开。 然后获取设备视频流ID(解码数据包时需要判断是否一致)，再获取输入编码器(解码时需要)。

* 打开输出设备

假设需要将从输入设备读取的数据保存成`mp4`格式的文件。

将视频流保存到文件中，只需要一个合适的编码器(用于生成符合MP4容器规范的帧)既可。 获取编码器大致分为两个步骤:

1. 构建编码器上下文(AVFormatContext)
2. 匹配合适的编码器(AVCodec)

构建编码器:
```c++
    // 根据output_file后缀名推测合适的编码器
    avformat_alloc_output_context2(&outAVFormatContext, NULL, NULL, output_file);
    if (!outAVFormatContext) {
        cout << "\nerror in allocating av format output context";
        exit(1);
    }
```

匹配编码器:
```c++
    output_format = av_guess_format(NULL, output_file, NULL);
    if (!output_format) {
        cout << "\nerror in guessing the video format. try with correct format";
        exit(1);
    }

    video_st = avformat_new_stream(outAVFormatContext, NULL);
    if (!video_st) {
        cout << "\nerror in creating a av format new stream";
        exit(1);
    }
```

* 编解码

从输入设备读取的是原生的数据流，也就是经过设备编码之后的数据。 需要先将原生数据进行解码，变成程序`可读`的数据，在编码成输出设备可识别的数据。 所以这一步的流程是：

1. 解码输入设备数据
2. 转码
3. 编码写入输出设备

通过`av_read_frame`从输入设备读取数据:
```c++
while (av_read_frame(pAVFormatContext, pAVPacket) >= 0) {
    ...
}
```

对读取后的数据进行拆包，找到我们所感兴趣的数据
```c++
    // 最开始没有做这种判断，出现不可预期的错误。 在官网example中找到这句判断，但还不是很清楚其意义。应该和packet封装格式有关
    pAVPacket->stream_index == VideoStreamIndx
```
从FFmpeg 4.1开始，有了新的编解码函数。 为了长远考虑，直接使用新API。 使用`avcodec_send_packet`将输入设备的数据发往`解码器`进行解码，然后使用`avcodec_receive_frame`从`解码器`接受解码之后的数据帧。代码大概是下面的样子:
```c++
            value = avcodec_send_packet(pAVCodecContext, pAVPacket);
            if (value < 0) {
                fprintf(stderr, "Error sending a packet for decoding\n");
                exit(1);
            }

            while(1){
                value = avcodec_receive_frame(pAVCodecContext, pAVFrame);
                if (value == AVERROR(EAGAIN) || value == AVERROR_EOF) {
                    break;

                } else if (value < 0) {
                    fprintf(stderr, "Error during decoding\n");
                    exit(1);
                }

                .... do something
            }
```

读取到数据帧后，就可以对每一帧进行`转码`:
```c++
    sws_scale(swsCtx_, pAVFrame->data, pAVFrame->linesize, 0, pAVCodecContext->height, outFrame->data,outFrame->linesize);
```

最后将转码后的帧封装成输出设备可设别的数据包格式。也就是解码的逆动作，使用`avcodec_send_frame`将每帧发往编码器进行编码，通过`avcodec_receive_packet`一直接受编码之后的数据包。处理逻辑大致是:
```c++
                value = avcodec_send_frame(outAVCodecContext, outFrame);
                if (value < 0) {
                    fprintf(stderr, "Error sending a frame for encoding\n");
                    exit(1);
                }

                while (value >= 0) {
                    value = avcodec_receive_packet(outAVCodecContext, &outPacket);
                    if (value == AVERROR(EAGAIN) || value == AVERROR_EOF) {
                        break;
                    } else if (value < 0) {
                        fprintf(stderr, "Error during encoding\n");
                        exit(1);
                    }

                    ... do something;

                    av_packet_unref(&outPacket);
                }
```

以后就按照这种的处理逻辑，不停的从输入设备读取数据，然后经过解码->转码->编码，最后发送到输出设备。 这样就完成了录屏功能。
上面是大致处理思路，完整源代码可以参考 (https://github.com/andy-zhangtao/ffmpeg-examples/tree/master/ScreenRecord) .