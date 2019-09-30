# Filter-Complex
> 通过API实现可控的Filter调用链

虽然通过声明`[x][y]avfilter=a=x:b=y;avfilter=xxx`的方式可以创建一个可用的`Filter`调用链，并且在绝大多数场合下这种方式都是靠谱和实用的。 但如果想`精细化`的管理`AVFilter`调用链，例如根据某些条件来动态生成`AVFilter Graph`。这种声明方式就不太灵活(也可以通过if判断来动态组装字符串，如果你非常喜欢这种字符串声明方式，到此为止不在建议你往下阅读了)。

首先快速温习一下，如何创建一个`AVFilter Graph`。

```shell
             +-------+             +---------------------+         +---------------+
             |buffer |             |Filter ..... Filter N|         |   buffersink  |
 ----------> |    |output|------>|input|            |output|---> |input|           |-------->
             +-------+             +---------------------+         +---------------+
```


创建三部曲:

1. 初始化`buffer`和`buffersink`。
2. 初始化其它filter
3. 设定Filter Graph的Input和Output。

其中`buffer`和`buffersink`分别代表`Graph`的起始和结束。

然后快速封装`args`也就是`movie=t.png[wm];[in][wm]overlay=10:20[out]`这样的filter-complex命令。 而且通过`avfilter_graph_parse_ptr`完成中间filter的初始化，

最后指定各个filter的input和output，一个graph就算搞定了。


好，下面来看如何通过API`精细化`生成`AVFilter Graph`。 生成下面的Graph:

```shell
             +-------+             +---------------------+         +---------------+
             |buffer |             |       Filter        |         |   buffersink  |
 ----------> |    |output|------>|input|    Fade      |output|---> |input|           |-------->
             +-------+             +---------------------+         +---------------+
```


首先初始化各个AVFilter。所有的AVFilter的初始化都可以简化为两步操作：

+ 通过`avfilter_get_by_name`查找指定的AVFilter
+ 通过`avfilter_graph_create_filter`初始化AVFilterContext

同`AVcodec`和`AVCodecContext`的关系一样， 所有的AVFilter的执行都依靠对应的AVFilterContext(在ffmpeg开发中，每个组件都会对应一个上下文管理器，由这个上下文管理器封装各种参数然后调用组件执行)。

通过`avfilter_get_by_name`生成AVFilter实例之后，紧跟着就需要调用`avfilter_graph_create_filter`初始化上下文管理器。

按照下面的流程，依次初始化三个AVFilter:
```C
buffer_src = avfilter_get_by_name("buffer");
ret = avfilter_graph_create_filter(&buffersrc_ctx, buffer_src, "in", args, NULL, filter_graph);
```

这里重点聊一下`avfilter_graph_create_filter`。 下面是函数原型:

```c
int avfilter_graph_create_filter(AVFilterContext **filt_ctx, const AVFilter *filt,
                                 const char *name, const char *args, void *opaque,
                                 AVFilterGraph *graph_ctx);
```

filt_ctx表示这个AVFilter的上下文管理器。

name表明的是AVFilter在Graph中的名称，这个名称叫啥不重要但必须唯一。 例如`Fade AVFilter`就可以叫做`fade1`,`fade2`或者`ifade`等等。

args则是这个AVFilter的参数， 注意仅仅是这个AVFilter的参数，不是整个graph的参数。再拿`Fade`举例，args就可以是`t=in:st=3:d=3`。

opaque一般给NULL就可以了。


初始完这三个AVFilter之后，就进入到本次文档的重点: `avfilter_link`.

```C
int avfilter_link(	AVFilterContext * 	src,
                    unsigned 	srcpad,
                    AVFilterContext * 	dst,
                    unsigned 	dstpad
)
```

`avfilter_link`分别用来链接两个AVFilter(传说中的一手托两家)。 `src`和`dst`分别表示源Filter和目标Filter。 `srcpad`表示`src`第N个输出端， `dstpad`表示`dst`第N个输入端。 如果不好理解，直接看下面的图:

```

    +-------------+                  +-------------+
    |  src      srcpad 1 ----->   dstpad 3    dst  |
    |           srcpad 2 ----->   dstpad 2         |
    |           srcpad 3 ----->   dstpad 1         |
    +-------------+                  +-------------+

```

上图假设`src`和`dst`分别有三个输出端和三个输入端(不是所有avfilter都有这么多的输入输出端，像fade只有一个，但overlay就有多个)。

而`srcpad`和`dstpad`表示的就是输出/输入端的序号。假如将`buffer`第一个输出端对应`fade`第一个输入端。 那么就应该这么写:

```
avfilter_link(buffersrc_ctx, 0, ifade_ctx, 0);
```

然后将`fade`的第一个输出端对应`buffersink`的输入端，就这么写:

```
avfilter_link(ifade_ctx, 0, buffersink_ctx, 0);
```

而所谓的`精细化`就是在这里体现的，通过代码的逻辑判断，可以动态的组合不同的`AVFilter`生成不同的`Filter Graph`。并且还可以组合不同的输入/输出端。

本次代码示例可以参考`ifilter`。同时也可以参考 [ffmpeg-go-server](https://github.com/andy-zhangtao/ffmpeg-go-server/blob/master/filters/ifade/copy.c)(一个尝试为ffmpeg提供restful API的web server)。