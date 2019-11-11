# Overlay 坐标点的计算

这张图片是从别处抄来的，但可以很形象的来描述如何计算Overlay坐标点.

![](https://tva1.sinaimg.cn/large/006y8mN6ly1g8u5lce74cj30jm0evq6g.jpg)

根据`ffmpeg`官网描述, `overlay`只能用来叠加两个输入源。 第一个视频源(这个视频源将会被放到后面，充当背景)称之为`main`，第二个视频源(放在第一个视频前面，充当画中画的角色)称之为`overlaid`。 `overlay`的坐标系如上图所描述，最上角为(0,0)，X轴向右递增，Y轴向下递增。

Overlay坐标点的计算，重点算的是`overlaid`原点在`main`中的位置，也就是上图中，input2的原点位置在input1中的坐标点。

在计算坐标之前，先来看一下Overlay两种坐标表示方式：

+ x=*NNN*:y=*NNN*
> NNN表示坐标点， 这种方式是通过X和Y指定坐标，例如: x=23:y=100  (input2原点在input1的 （23，100）位置)

+ *NNN*:*NNN*
> NNN表示坐标点， 但不使用X和Y来表示。 同样23:100也表示input2原点在input1的 （23，100）位置

第二种是第一种的简化方式。

下面来看坐标如何计算。在Overlay Filter中，每个视频源都有两个属性： 高(Height)和宽(Weight). 因此对`main`和`overlaid`来说，其高和宽分别用`main_w`,`main_h`和`overlay_w`和`overlay_h`来表示，如果不想写这么长，也可以用`W`，`H`和`w`，`h`来简写替代。

了解表示方法之后，我们先来计算比较简单的四个位置： 左上角，左下角，右上角和右下角。

+ input2在左上角的情况

当我们期望input2出现在左上角时，此时input2的原点(左上角)坐标和input1的原点坐标是重合的，所以此时此刻input2的坐标是(0,0)，如下图

![](https://tva1.sinaimg.cn/large/006y8mN6ly1g8u6cnknexj30jt0eyadn.jpg)

+ input2在右上角的情况

当我们期望input2出现在右上角的时候，参考一下开篇的坐标系，此时，input2的原点坐标中X点坐标不固定，但Y点坐标可以确定为0。 而X点的位置则可以通过 `input1的宽带 - input2的宽度`来确定。也就是 `main_w - overlay_w`。因此input2的原点坐标此时此刻就是(main_w - overlay_w, 0)，如下图：

![](https://tva1.sinaimg.cn/large/006y8mN6ly1g8u6u6w2d4j30jl0f3421.jpg)

如果为了好看，想让input2离右边有一些距离，可以让X轴再向做移动10个像素，此时原点坐标就是(main_w - overlay_w - 10, 0)

+ input2在左下角的情况

有了上面两个坐标计算过程，左下角就容易计算了。 此时input2的原点X坐标为0, Y点坐标为`main_h - overlay_h`，因此原点坐标就是(0, main_h - overlay_h)

+ input2在右下角的情况

右下角和左上角是互补关系， 此时的原点坐标应该是(main_w - overlay_w, main_h - overlay_h )


其它坐标位置的计算和上述四种坐标点的计算原理相同，例如要将input显示在屏幕中央，input原点坐标就是(main_w /2 - overlay_w / 2, main_h / 2 - overlay_h / 2).