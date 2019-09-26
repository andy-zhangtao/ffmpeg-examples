# IFade
> 可以在同一个视频中实现多次淡入效果的AVFilter

前面几篇文章聊了聊FFmpeg的基础知识，我也是接触FFmpeg不久，除了时间处理之外，很多高深(滤镜)操作都没接触到。在学习时间处理的时候，都是通过在ffmpeg目前提供的avfilter基础上面修修补补(补充各种debug log)来验证想法。 而这次我将尝试新创建一个avfilter，来实现一个新滤镜。

因为我是新手，所以本着先易后难的原则(其实是不会其它高深API的操作)，从fade滤镜入手来仿制一个new fade(就起名叫做ifade)。

### 目标

`fade`是一个淡入淡出的滤镜，可以通过参数设置fade type(in表示淡入, out表示淡出)，在视频的头部和尾部添加淡入淡出效果。 在使用过程中，fade有一些使用限制。

+ 淡入只能从片头开始设置(00:00:00.0位置起)
+ 淡出只能从片尾开始设置
+ 一次只能设置一个类型

如果想在一个视频中间设置多次淡入淡出效果，那么只能先分割视频，分别应该fade之后在合并(可能还有其它方式，可我没找到)。如果想一次实现多个fade效果，那么就要通过-filter-complex来组合多个fade，并合理安排调用顺序，稍显麻烦。

这次，ifade就尝试支持在同一个视频中实现多次fade效果。ifade计划完成的目标是:

+ 一次支持设置一个类型(淡入/淡出)
+ 一次支持设置多个fade时间点
+ 支持fade时长

### 分析

先看看原版`fade`是如何实现的。

```c
     1	static int filter_frame(AVFilterLink *inlink, AVFrame *frame)
     2	{
     3	    AVFilterContext *ctx = inlink->dst;
     4	    FadeContext *s       = ctx->priv;
     5	    double frame_timestamp = frame->pts == AV_NOPTS_VALUE ? -1 : frame->pts * av_q2d(inlink->time_base);
     6
     7	    // Calculate Fade assuming this is a Fade In
     8	    if (s->fade_state == VF_FADE_WAITING) {
     9	        s->factor=0;
    10	        if (frame_timestamp >= s->start_time/(double)AV_TIME_BASE
    11	            && inlink->frame_count_out >= s->start_frame) {
    12	            // Time to start fading
    13	            s->fade_state = VF_FADE_FADING;
    14
    15	            // Save start time in case we are starting based on frames and fading based on time
    16	            if (s->start_time == 0 && s->start_frame != 0) {
    17	                s->start_time = frame_timestamp*(double)AV_TIME_BASE;
    18	            }
    19
    20	            // Save start frame in case we are starting based on time and fading based on frames
    21	            if (s->start_time != 0 && s->start_frame == 0) {
    22	                s->start_frame = inlink->frame_count_out;
    23	            }
    24	        }
    25	    }
    26	    if (s->fade_state == VF_FADE_FADING) {
    27	        if (s->duration == 0) {
    28	            // Fading based on frame count
    29	            s->factor = (inlink->frame_count_out - s->start_frame) * s->fade_per_frame;
    30	            if (inlink->frame_count_out > s->start_frame + s->nb_frames) {
    31	                s->fade_state = VF_FADE_DONE;
    32	            }
    33
    34	        } else {
    35	            // Fading based on duration
    36	            s->factor = (frame_timestamp - s->start_time/(double)AV_TIME_BASE)
    37	                            * (float) UINT16_MAX / (s->duration/(double)AV_TIME_BASE);
    38	            if (frame_timestamp > s->start_time/(double)AV_TIME_BASE
    39	                                  + s->duration/(double)AV_TIME_BASE) {
    40	                s->fade_state = VF_FADE_DONE;
    41	            }
    42	        }
    43	    }
    44	    if (s->fade_state == VF_FADE_DONE) {
    45	        s->factor=UINT16_MAX;
    46	    }
    47
    48	    s->factor = av_clip_uint16(s->factor);
    49
    50	    // Invert fade_factor if Fading Out
    51	    if (s->type == FADE_OUT) {
    52	        s->factor=UINT16_MAX-s->factor;
    53	    }
    54
    55	    if (s->factor < UINT16_MAX) {
    56	        if (s->alpha) {
    57	            ctx->internal->execute(ctx, filter_slice_alpha, frame, NULL,
    58	                                FFMIN(frame->height, ff_filter_get_nb_threads(ctx)));
    59	        } else if (s->is_packed_rgb && !s->black_fade) {
    60	            ctx->internal->execute(ctx, filter_slice_rgb, frame, NULL,
    61	                                   FFMIN(frame->height, ff_filter_get_nb_threads(ctx)));
    62	        } else {
    63	            /* luma, or rgb plane in case of black */
    64	            ctx->internal->execute(ctx, filter_slice_luma, frame, NULL,
    65	                                FFMIN(frame->height, ff_filter_get_nb_threads(ctx)));
    66
    67	            if (frame->data[1] && frame->data[2]) {
    68	                /* chroma planes */
    69	                ctx->internal->execute(ctx, filter_slice_chroma, frame, NULL,
    70	                                    FFMIN(frame->height, ff_filter_get_nb_threads(ctx)));
    71	            }
    72	        }
    73	    }
    74
    75	    return ff_filter_frame(inlink->dst->outputs[0], frame);
    76	}
```

不想贴代码，但发现不贴代码好像很难表述清楚。-_-！

`fade`在处理fame时最关键的是三种状态和一个变量因子。

三种状态:
+ VF_FADE_WAITING 待渲染, 初始状态
+ VF_FADE_FADING 渲染中
+ VF_FADE_DO 渲染结束

变量因子:
+ factor 控制效果强度

假设现在设置的是淡入效果(如果是淡出效果，52行会实现一个反转)): `s->fade_state`初始化状态是`VF_FADE_WAITING`，滤镜工作时就会进入第八行的判断，此时将`s->factor`设置为0。如果我们假设淡入的背景颜色是黑色(默认色)，当`s->factor==0`时，渲染强度最大，此时渲染出的就是一个纯黑的画面。

第八行的if判断是一个全局初始化，一旦进入之后，`s->fade_status`就会被修改为`VF_FADE_FADING`状态。

而26到43行的判断，是为了找到渲染结束时间点。通过不停的判断每帧的frame_timestamp和start_time+duration之间的关系(通过start_frame同理)，来决定是否结束渲染。`start_time`是由`fade st=xxx`来设定的，当到达结束时间点后，将`s->fade_status`变更为VF_FADE_DO，即可结束渲染(其实是将`s->factor`置为UINT16-MAX，这样就不会进入到第55行的渲染逻辑)。

fade大致的处理流程如下:

```shell
     +------------------------------------------------------------------------------------------------------------- +
     |                                                                                                              |
     |       |----------------------------------------------------------|------------------|-------------------->   |
     |time   0                                                          st             st+duration                  |
     |                                                                                                              |
     |status VF_FADE_WAITING                                                                                        |
     |                               VF_FADE_FADING                                                                 |
     |                                                              VF_FADE_DO                                      |
     |factor 0       0        0         0              0        0       100  500 4000 ...  65535  65535  65535 65535|
     |                                                                                                              |
     +--------------------------------------------------------------------------------------------------------------+
```

在`0->st`这段时间内，status一直是VF_FADE_FADING状态，factor是0。 这段时间内渲染出来的全是黑色。到达st点后，开始逐步调整factor的值(不能一次性的调整到UINT16-MAX，要不就没有逐渐明亮的效果了)，直到`st+duration`这个时间后，在将`factor`调整为UINT16-MAX。以后流经fade的帧就原样流转到`ff_filter_frame`了。

### 改造

分析完`fade`的处理逻辑之后，如果要实现`ifade`的效果，那么应该是下面的流程图：

```shell
     +------------------------------------------------------------------------------------------------------------------+
     |                                     A                  B                C                  D                     |
     |       |-----------------------------|------------------|----------------|------------------|-------------------->|
     |time   0                            st1               st2-duration      st2            st2+duration               |
     |                                                                                                                  |
     |status    VF_FADE_FADING                                                                                          |
     |                                VF_FADE_DO                                                                        |
     |                                                                                                                  |
     |                                                  VF_FADE_FADING                                                  |
     |                                                                                          VF_FADE_DO              |
     |factor 0       0        0           65535     65535    0  0 0  0 0 0 0 0 100  500 4000 ... 65535                  |
     |                                                                                                                  |
     +------------------------------------------------------------------------------------------------------------------+
```

从`0-A点`仍然是`fade`原始逻辑。到达A点之后，将`s->fade_status`改完`VF_FADE_DO`表示关闭渲染。 当到达B点时(距离st2还有duration的时间点)，开始将`s->factor`调整为0. 这是为了模拟出画面从暗到亮的效果。同时`s->fade_status`再次置为`VF_FADE_FADING`状态，到达C点是开始重新计算`s->factor`的值，将画面逐渐变亮。

可以看出`ifade`就是利用`s->fade_status`重复利用现有的处理逻辑来实现多次淡入的效果。

### 实现

上面分析完之后，就可以动手写代码了。 具体代码就不贴出来了，可以直接看源码。 下面就说几个在ffmpeg 4.x中需要注意的地方：

+ 添加新avfilter

    - 在`libavfilter/Makefile`中添加新filter名称。 `OBJS-$(CONFIG_IFADE_FILTER)                   += vf_ifade.o`
    - 在`libavfilter/allfilter.c`中添加新filter. `extern AVFilter ff_vf_ifade`

+ 重新生成makefile

    - 重新根据实际情况执行`configure`，生成最新的makefile脚本

然后就是漫长的等待。

在编写filter时，ffmpeg提供了`AVFILTER_DEFINE_CLASS`这个宏来生成默认的`avclass`和`options`，所以一定要注意class名称和options名称要和宏定义中的名字保持一致，否则会导致编译失败。