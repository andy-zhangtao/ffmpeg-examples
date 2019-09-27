# 视频读取处理的简要流程
> 读取/输出视频的简要流程和一些误区

在读取，处理视频文件时，以下四个结构体是非常重要的，所以放在片首提一下。

+ AVFormatContext
> 媒体源的抽象描述,可以理解成视频/音频文件信息描述

+ AVInputFormat / AVOutputFormat
> 容器的抽象描述

+ AVCodecContext / AVCodecParameters
> 编解码的抽象描述,ffmpeg使用率最高的结构体(AVCodecContext被AVCodecParameters所取代)

+ AVStream
> 每个音视频的抽象描述

+ AVCodec
> 编解码器的抽象描述

四个结构体的包含关系大致如下:

```
          |AVFormatContext
                |
                |---------> AVInputFormat / AVOutputFormat
                |
                |---------> AVStream
                                |-------> time_base (解码时不需要设置, 编码时需要用户设置)
                                |
                                |------->   AVCodecParameters|--------> codec id
                                                                            |
                                                                            |
          |AVCodec ------------通过 codec id 关联----------------------------+
              |
              |
              |-------------->AVCodecContext
```

读取/输出视频时，基本就是围绕这五个结构体来进行操作的。 而不同点在于，读取文件时，`ffmpeg`会通过读取容器metadata来完成`AVFormateContext`的初始化。输出文件时，我们需要根据实际情况自行封装`AVFormateContext`里面的数据。封装时的数据来源，一部分来自于实际情况(例如time_base,framerate等等)，另外一部分则来自于数据源。

下面分别来描述读取和输出的差异。

先看读取的大致流程:
```shell
                                                                |------------------------------------------------------loop---------------------------------------------------|
                                                                |                                      |--------------------------------------------------------------------  |
                                                                |                                      |                                                                   |  |
avformat_open_input ---------->  AVFormatContext  ----------> stream   ----avcodec_find_decoder---->  codec  -------avcodec_alloc_context3-----------> codecContent ---avcodec_open2-->
                                                               |                                                                                           |
                                                               |------------------------------------avcodec_parameters_to_context--------------------------|

```
`avformat_open_input`会尝试根据指定文件的metadata完成`AVFormatContext`的部分初始化，如果视频源是包含header的，那么此时的`AVFormatContext`数据基本都齐了。如果是不包含header的容器格式(例如MPEG)，`AVFormatContext`此时就没有`AVStream`的数据，需要单独使用`avformat_find_stream_info`来完成`AVStream`的初始化。

无论怎样，待`AVFormatContext`完成了初始化，就可以通过轮询`AVStream`来单独处理每一个`stream`数据,也就是上面的`loop`。下面单拎一条`stream`来聊。

解码视频只需要`AVCodecContext`就可以了，从包含图可以得知根据`AVCodec`可以生成`AVCodecContext`，而`avcodec_find_decoder`又可以生成对应的`codec`。所以大致的思路就清晰了，首先通过`inStream->codecpar(AVCodecParameters)->codec_id`和`avcodec_find_decoder`生成指定的解码器`AVCodec`， 然后通过`avcodec_alloc_context3`就可以生成可以解码视频源的`AVCodecContext`。

此时产生了第一个误区：生成的`AVCodecContext`就可以直接解码视频！ **这是错误的**

现在的`AVCodecContext`只是一个通用`Codec`描述，没有视频源的特定信息(avcodec_parameters_to_context的代码有些长，我也没搞明白具体是哪些信息)。 所以需要调用`avcodec_parameters_to_context`将`inStream->codecpar`和`AVCodecContext`糅合到一起(俗称merge)。这时的`AVCodecContext`才能打开特定的视频文件。

> 对于没有header的容器。 framerate 和 time_base 仍然需要特别设定。
> fraterate 可以通过av_guess_frame_rate获取。 time_base可以直接使用AVStream的time_base;

最后就是使用`avcodec_open2`打开`AVCodecContext`并处于待机状态。

输出的流程和读取的流程相似，但又有不同。 读取读取参数较多，而输出更多的是封装参数。 下面是输出的大致流程：


```shell
                                                                            |----------------------------------avcodec_parameters_from_context-----------------|
                                                                            |                                                                                  |
                                                                     stream(enc)---avcodec_find_encoder ---> codec(enc)---avcodec_alloc_context3---> codecContent(enc)----avcodec_open2---->
                                                                                                              -----------
                                                                                                                   |
                                                                                                                   |
                                                                                                                   |
avformat_alloc_output_context2  -------> AVFormatContext --------avformat_new_stream--------> stream -------copy dec to enc---
                                                                           |                                       |
                                                                           |---------------------loop--------------|
```

无论是读取还是输出，首要任务都是构建`AVFormateContext`。有所不同的是此时(输出)，我们先构建一个模板，然后往里面填值，因此使用的是`avformat_alloc_output_context2`函数。
>avformat_alloc_output_context2和avformat_open_input 都是用来生成AVFormatContext的。不同的是，一个生成模板往里面填值，另一个生成的是已经完成初始化的。

编码一个视频文件，需要的也只是一个`AVCodecContext`. 但此时离生成`AVCodecContext`还差很多东西。所以需要我们逐一进行准备，按照最上面的包含图，需要先生成一个`AVStream`。因此调用`avformat_new_stream`生成一个空`AVStream`。

有了`AVStream`之后，就需要将这个`Stream`与具体的`Codec`关联起来。 其次，根据需要我们指定`avcodec_find_encoder`生成一个标准的`AVCodec`,而后使用`avcodec_alloc_context3`生成对应的`AVCodecContext`。

第二个误区：生成的`AVCodecContext`就可以直接解码视频！ **这是错误的**

现在生成的`AVCodecContext`不能直接使用，因为还有参数是标准参数没有适配。以下参数是经常容易出错的：

```
    width
    height
    framerate
    time_base
    sample_aspect_ratio
    pix_fmt

    time_base(AVSteam)
```

在对`codecpar`和`AVCodecContext`进行`反向`merge. 反向指的是从`AVCodecContext`读取参数填充到`codecpar`中所以才需要提前设置`AVCodecContext`中的参数。

最后调用`avcodec_open2`处于待输出状态。

上面是读取/输出的流程，下面来补充说一下如何从视频源读数据，再写到目标视频中。

真正读取视频数据涉及到的结构体是:

+ AVPacket
> 可能包含一个或多个 frame。 如果包含多个，则读取第一个

+ AVFrame
> 保存当前帧的数据格式

一个典型的读取处理代码，看起来应该是下面的样子:

```C
    while (1){
        av_read_frame (读取数据)
         ...
        avcodec_decode_video2 (对读到的数据进行解码)
         ...
        avcodec_encode_video2 (对数据进行编码)
         ...
        av_interleaved_write_frame (写到文件中)
    }

    av_write_trailer (写metadata)
```

> avcodec_decode_video2 和 avcodec_encode_video2 是即将废弃的函数，替代函数的使用可参看前几篇文章

在这里也有几个误区:

第三个误区，AVPacket声明后就可用。 **这是错误的**
> AVPacket 声明后需要手动设置{.data = NULL, .size = 0}.

第四个误区，AVPacket time_base直接设置 **经过验证，这也是错误的**
> 直接设置不好使。 还是老老实实通过av_packet_rescale_ts来调整 AVPacket的time base吧。同理，在写文件之前也需要调用av_packet_rescale_ts来修改time base。

以上就是今天学习的结果，希望对以后解析/输出视频能有所帮助。