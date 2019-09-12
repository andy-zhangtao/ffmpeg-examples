## Filter
> 通过API实现Filter Graph

FFmpeg提供了很多实用且强大的滤镜,比如:overlay, scale, trim, setpts等等。

通过`-filter-complex`的表达式功能，可以将多个滤镜组装成一个调用图，实现更为复杂的视频剪辑。如何通过代码实现这个功能呢？

首先按照前面几篇的套路，在开发FFmpeg应用时，大致有三板斧：

1. 初始化输入设备(初始化解码器及其应用上下文)
2. 初始化输出设备(初始化编码器及其应用上下文)
3. 编写帧处理逻辑(对符合要求的帧数据做各种运算处理)

本次需要实现的Filter Graph功能稍有不同，在处理帧之前需要先完成`Filter Graph`的处理。 处理流程如下:

```
+------------------------------------------------+
|   +---------+                                  |
|   | Input   | ----------read --------+         |
|   +---------+                        |         |
|                                      |         |
|                                     \|/        |
|                          +-----------+         |
|  +-----------------------|   Input   |         |
|  |                       +-----------|         |
|  |                                   |         |
|  |                                  \|/        |
|  |   +-----------+       +-----------+         |
|  +<--|  Filter N |<-.N.--| Filter 1  |         |
|  |   +-----------+       +-----------+         |
|  |                                             |
|  |       +-------------+                       |
|  +------>|     Output  |                       |
|          +-------------+                       |
+------------------------------------------------+
```

从`Input`读取到视频数据之后，会依次经过`Filter 1`和`Filter N`，每个Filter会依次根据设定好的参数处理流经的帧数据，当所有Filter都处理完毕之后，再经过编码器编码吸入`Output`.

从流程可以看出，视频中的每一帧都被处理了N次，这也是视频在应用滤镜时感觉编解码时间有些长的原因。

本次增加了一部分API:

1. avfilter_get_by_name
2. avfilter_inout_alloc
3. avfilter_graph_alloc
4. avfilter_graph_create_filter
5. avfilter_graph_parse_ptr
6. av_buffersink_get_frame

* 初始化出入设备

和以前的操作一样，这里就不做过多叙述。若有需要可以翻看前几篇文章。这里只增加一个dump函数:
```c++
av_dump_format(inFormatContext, 0, "1", 0);
```

`av_dump_format`可以输出指定FormatContext的数据，方便定位问题。

* 初始化输出设备

同样不做过多描述，若有需要可翻看前几篇文章或者直接看源码。 仅仅提醒一下关于time_base的几个坑。
time_base是用来做基准时间转换的，也就是告诉编码器以何种速度来播放帧(也就是pts)。前几篇代码中所使用的time_base是:

```c++
    outCodecContext->time_base = (AVRational) {1, 25};
```
1是分子，25是分母。 在进行编码时，编码器需要知道每一个关键帧要在哪个时间点进行展示和渲染(对应的就是pts和dts)。 在没有B帧的情况下，PTS=DTS。 而计算pts时，需要建立编码time_base和解码time_base的对应关系.

```
假设，time=5. 那么在1/25(编码time_base)的时间刻度下应该等于1/10000(编码time_base)时间刻度下的(5*1/25)/(1/90000) = 3600*5=18000
```

time_base的详细应用，可以参考`setpts`中的实现。

* 初始化Filter Graph

在`Filter Graph API`中有两个特殊的Filter：`buffer`和`buffersink`：

```
 ----------> |buffer| ---------|Filter ..... Filter N|----------->|buffersink|-------->
```

`buffer`表示Filter Graph的开始，`buffersink`表示Filter Graph的结束。这两中Filter是必须要存在不可缺少。

Filter Graph使用的步骤如下：
1. 初始化`buffer`和`buffersink`。
2. 初始化其它filter
3. 设定Filter Graph的Input和Output。

* 初始化`buffer`和`buffersink`

通过`avfilter_get_by_name`来查找相符的Filter，例如:

```
    const AVFilter *buffersrc = avfilter_get_by_name("buffer");
```

表示获取buffer Filter。然后通过avfilter_graph_create_filter来初始化filter，例如初始化buffer:

```
    snprintf(args, sizeof(args),
             "video_size=%dx%d:pix_fmt=%d:time_base=%d/%d:pixel_aspect=%d/%d",
             inCodecContext->width, inCodecContext->height, inCodecContext->pix_fmt,
             time_base.num, time_base.den,
             inCodecContext->sample_aspect_ratio.num, inCodecContext->sample_aspect_ratio.den);

    av_log(NULL, AV_LOG_ERROR, "%s\n", args);

    ret = avfilter_graph_create_filter(&buffersrc_ctx, buffersrc, "in",
                                       args, NULL, filter_graph);
```

"in"表示buffer在整个Graph中叫做'in'。 名称可以随便叫，只要保证唯一不重复就好。

* 初始化其它filter

通过``使用指定的Filter Graph 语法来初始化剩余的Filter，例如:
```c++
    const char *filter_descr = "movie=t.png[wm];[in][wm]overlay=10:20[out]";

    avfilter_graph_parse_ptr(filter_graph, filter_descr,
                                        &inputs, &outputs, NULL)
```

上面表示使用了两个filter：`movie`和`overlay`。 `inputs`和`outputs`表示Graph的输入输出。

* 设定Filter Graph的Input和Output

这段代码有些不好理解:

```c++
    outputs->name = av_strdup("in");
    outputs->filter_ctx = buffersrc_ctx;
    outputs->pad_idx = 0;
    outputs->next = NULL;

    inputs->name = av_strdup("out");
    inputs->filter_ctx = buffersink_ctx;
    inputs->pad_idx = 0;
    inputs->next = NULL;
```

outputs对应的是`in`(也就是buffer)，`in`是Graph第一个Filter，所以它只有输出端(所以对应到了outputs)。 同理`out`(buffersink)是Graph最后一个Filter，只有输入端，因此对应到了inputs。

```
             +-------+             +---------------------+         +---------------+
             |buffer |             |Filter ..... Filter N|         |   buffersink  |
 ----------> |    |output|------>|input|            |output|---> |input|           |-------->
             +-------+             +---------------------+         +---------------+
```

在下一篇中，我们会通过其它api设定每个Filter的input和output，那个时候应该会更容易理解一点。

在完成Filter Graph初始化之后，一定要通过`avfilter_graph_config`来验证参数配置是否正确。
```c++
    avfilter_graph_config(filter_graph, NULL)
```

* 逻辑处理

在处理帧数据时，就和以前的思路基本保持一致了。 从解码器接受帧，然后发送到`Filter Graph`中进行滤镜处理，最后再发送给编码器写入到输出文件。

唯一有些不同的就是增加了两个函数`av_buffersrc_add_frame_flags`和`av_buffersink_get_frame`. `av_buffersrc_add_frame_flags`表示向Filter Graph加入一帧数据，`av_buffersink_get_frame`表示从Filter Graph取出一帧数据。

因此上一篇中的编码流程增加了一个while循环:
```shell
    while av_read_frame
        |
        +---> avcodec_send_packet
                    |
                    +----> while avcodec_receive_frame
                                     | 对每一数据帧进行解码
                                     | 通过`sws_scale`进行源帧和目标帧的数据转换
                                     |
                                     +---->av_buffersrc_add_frame_flags
                                                    |
                                                    |
                                                    +while av_buffersink_get_frame
                                                            |
                                                            |
                                                            +-->avcodec_send_frame
                                                                    |
                                                                    +---> while avcodec_receive_packet
                                                                                    |
                                                                                    |
                                                                                    |+--->av_interleaved_write_frame （写入到输出设备）
```

至此就完成了通过代码实现`-filter-complex`功能。
