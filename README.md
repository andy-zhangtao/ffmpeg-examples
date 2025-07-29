# FFmpeg Examples

一个基于 FFmpeg APIs 的综合性示例项目，展示了音视频处理的核心功能实现。

## 项目概述

本项目提供了多个完整的 FFmpeg 使用示例，涵盖屏幕录制、视频滤镜、时间戳处理、视频拼接等常见音视频处理场景。每个示例都包含完整的源代码和详细的实现说明。

## 功能模块

### 📺 独立应用程序
这些是完整的可执行程序，直接调用 FFmpeg API：

- **ScreenRecord**: 基础屏幕录制应用，支持 MPEG4 编码
- **ScreenRecord-H264**: H.264 编码的屏幕录制应用
- **Filter**: 实时视频滤镜处理应用
- **ifilter**: 高级视频滤镜应用
- **PTS TimeStamp**: 视频帧时间戳分析工具
- **itransfer**: 视频格式转换和编码工具

### 🔧 FFmpeg 滤镜源码
这些是 FFmpeg 内置滤镜的源代码实现，需要编译到 FFmpeg 中：

- **iconcat**: 视频拼接滤镜源码（基于 FFmpeg concat 滤镜）
- **ifade**: 视频淡入淡出滤镜源码（基于 FFmpeg fade 滤镜）
- **isetpts**: 时间戳设置滤镜源码（基于 FFmpeg setpts 滤镜）

## 技术特性

- ✅ 跨平台支持（macOS/Linux/Windows）
- ✅ 支持多种编码格式（H.264、MPEG4、YUV420P等）
- ✅ 实时视频处理
- ✅ 自定义滤镜链
- ✅ 高性能音视频处理
- ✅ 完整的错误处理机制

## 系统要求

- **FFmpeg**: 4.0 或更高版本
- **CMake**: 3.10 或更高版本
- **编译器**: 支持 C++11 的编译器（GCC 7+, Clang 5+, MSVC 2017+）

> 📚 **新手指南**: 如果你是第一次搭建 FFmpeg 开发环境，请参考详细的 [环境搭建指南](./docs/environment-setup.md)，包含 Windows、macOS、Linux 的完整配置步骤。

### 快速安装（经验用户）

#### macOS 依赖安装
```bash
brew install ffmpeg cmake
```

#### Ubuntu/Debian 依赖安装
```bash
sudo apt update
sudo apt install libavformat-dev libavcodec-dev libavutil-dev libavdevice-dev libavfilter-dev cmake build-essential
```

## 编译与运行

### 1. 克隆项目
```bash
git clone https://github.com/andy-zhangtao/ffmpeg-examples.git
cd ffmpeg-examples
```

### 2. 编译单个模块
每个模块都有独立的 CMake 配置：

```bash
# 编译屏幕录制模块
cd ScreenRecord
mkdir build && cd build
cmake ..
make
./ScreenRecord

# 编译滤镜模块
cd ../../Filter
mkdir build && cd build
cmake ..
make
./Filter
```

### 3. 运行示例

#### 独立应用程序
```bash
# 屏幕录制
cd ScreenRecord/build
./ScreenRecord

# H.264 屏幕录制
cd ../../SceenRecord-h264/build
./SceenRecord_h264

# 视频滤镜处理
cd ../../Filter/build
./Filter

# 时间戳分析
cd ../../pts-timestamp/build
./pts_timestamp
```

#### FFmpeg 滤镜使用
这些滤镜源码需要先编译到 FFmpeg 中，然后通过 ffmpeg 命令使用：
```bash
# 使用自定义的淡入淡出滤镜（需要重新编译 FFmpeg）
ffmpeg -i input.mp4 -vf "ifade=t=in:d=2.0" output.mp4

# 使用自定义的拼接滤镜
ffmpeg -i video1.mp4 -i video2.mp4 -filter_complex "[0:v][1:v]iconcat=n=2[v]" -map "[v]" output.mp4

# 使用自定义的时间戳滤镜
ffmpeg -i input.mp4 -vf "isetpts=PTS*2" output.mp4
```

## 代码结构

```
ffmpeg-examples/
├── 📱 独立应用程序
│   ├── ScreenRecord/          # 基础屏幕录制应用
│   ├── SceenRecord-h264/      # H.264 屏幕录制应用
│   ├── Filter/                # 视频滤镜处理应用
│   ├── ifilter/               # 高级滤镜应用
│   ├── pts-timestamp/         # 时间戳分析工具
│   └── itransfer/             # 视频格式转换工具
├── 🔧 FFmpeg 滤镜源码
│   ├── iconcat/               # concat 滤镜源码
│   ├── ifade/                 # fade 滤镜源码
│   └── isetpts/               # setpts 滤镜源码
└── 📚 文档
    └── docs/                  # 详细文档和教程
```

## 核心 API 使用

### 初始化 FFmpeg
```cpp
av_register_all();
avcodec_register_all();
avdevice_register_all();
avfilter_register_all();
```

### 屏幕录制配置
```cpp
// 设置录制参数
av_dict_set(&options, "capture_cursor", "1", 0);
av_dict_set(&options, "framerate", "30", 0);
av_dict_set(&options, "video_size", "1920x1080", 0);
av_dict_set(&options, "pixel_format", "yuyv422", 0);
```

### 滤镜链创建
```cpp
// 创建滤镜图
filter_graph = avfilter_graph_alloc();
avfilter_graph_create_filter(&buffersrc_ctx, buffersrc, "in", args, NULL, filter_graph);
avfilter_graph_create_filter(&buffersink_ctx, buffersink, "out", NULL, NULL, filter_graph);
```

## 滤镜源码集成

### 如何将自定义滤镜编译到 FFmpeg

对于 `iconcat`、`ifade`、`isetpts` 这三个滤镜源码，需要以下步骤：

#### 1. 下载 FFmpeg 源码
```bash
git clone https://git.ffmpeg.org/ffmpeg.git
cd ffmpeg
```

#### 2. 复制滤镜源码
```bash
# 将滤镜源码复制到 FFmpeg 源码目录
cp /path/to/ffmpeg-examples/ifade/ifade.c libavfilter/vf_ifade.c
cp /path/to/ffmpeg-examples/iconcat/avf_concat.c libavfilter/avf_iconcat.c
cp /path/to/ffmpeg-examples/isetpts/setpts.c libavfilter/vf_isetpts.c
```

#### 3. 修改 Makefile
在 `libavfilter/Makefile` 中添加：
```makefile
OBJS-$(CONFIG_IFADE_FILTER)          += vf_ifade.o
OBJS-$(CONFIG_ICONCAT_FILTER)        += avf_iconcat.o
OBJS-$(CONFIG_ISETPTS_FILTER)        += vf_isetpts.o
```

#### 4. 配置并编译
```bash
./configure --enable-filter=ifade --enable-filter=iconcat --enable-filter=isetpts
make -j$(nproc)
sudo make install
```

## 性能优化

- **多线程处理**: 支持并行帧处理
- **内存管理**: 优化的缓冲区分配和释放
- **硬件加速**: 支持GPU编解码（需要硬件支持）
- **格式优化**: 针对不同场景优化编码参数

## 常见问题

### Q: 编译时找不到 FFmpeg 库？
A: 确保已正确安装 FFmpeg 开发包，并设置了正确的库路径。

### Q: 屏幕录制在 macOS 上需要权限？
A: 需要在系统偏好设置中授予屏幕录制权限。

### Q: 如何自定义编码参数？
A: 修改各模块中的编码上下文配置，参考源码中的参数设置。

## 文档资源

- [FFmpeg 官方文档](https://ffmpeg.org/documentation.html)
- [API 参考手册](https://ffmpeg.org/doxygen/trunk/)
- [项目详细文档](./docs/)

## 贡献指南

1. Fork 本项目
2. 创建特性分支 (`git checkout -b feature/new-feature`)
3. 提交更改 (`git commit -am 'Add new feature'`)
4. 推送到分支 (`git push origin feature/new-feature`)
5. 创建 Pull Request

## 许可证

本项目采用 MIT 许可证，详见 [LICENSE](LICENSE) 文件。

## 作者

- **Zhang Tao** - 初始作者和维护者

---

⭐ 如果这个项目对你有帮助，请给个 Star！
