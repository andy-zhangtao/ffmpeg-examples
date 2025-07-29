# FFmpeg 开发环境搭建指南

本文档详细介绍如何在不同操作系统上搭建 FFmpeg 开发环境，适合完全零基础的新手用户。

## 目录

- [Windows 环境搭建](#windows-环境搭建)
- [macOS 环境搭建](#macos-环境搭建)
- [Linux 环境搭建](#linux-环境搭建)
- [验证安装](#验证安装)
- [常见问题解决](#常见问题解决)
- [IDE 配置](#ide-配置)

## Windows 环境搭建

### 方式一：使用 vcpkg（推荐）

#### 1. 安装 Visual Studio
下载并安装 [Visual Studio 2019/2022 Community](https://visualstudio.microsoft.com/downloads/)
- 勾选 "C++ 桌面开发" 工作负载
- 确保包含 CMake 工具

#### 2. 安装 vcpkg
```powershell
# 在 C:\ 目录下执行
git clone https://github.com/Microsoft/vcpkg.git
cd vcpkg
.\bootstrap-vcpkg.bat
.\vcpkg integrate install
```

#### 3. 安装 FFmpeg
```powershell
# 安装 FFmpeg（这个过程可能需要 30-60 分钟）
.\vcpkg install ffmpeg:x64-windows

# 如果需要更多编解码器支持
.\vcpkg install ffmpeg[avcodec,avdevice,avfilter,avformat,avresample,avutil,swresample,swscale]:x64-windows
```

#### 4. 配置环境变量
添加到系统环境变量：
```
VCPKG_ROOT=C:\vcpkg
CMAKE_TOOLCHAIN_FILE=C:\vcpkg\scripts\buildsystems\vcpkg.cmake
```

### 方式二：手动编译（高级用户）

#### 1. 安装 MSYS2
- 下载 [MSYS2](https://www.msys2.org/)
- 安装后更新系统：
```bash
pacman -Syu
```

#### 2. 安装编译工具
```bash
pacman -S base-devel mingw-w64-x86_64-toolchain
pacman -S mingw-w64-x86_64-cmake
pacman -S mingw-w64-x86_64-pkg-config
```

#### 3. 编译 FFmpeg
```bash
# 下载 FFmpeg 源码
git clone https://git.ffmpeg.org/ffmpeg.git
cd ffmpeg

# 配置编译选项
./configure --prefix=/mingw64 --enable-shared --disable-static

# 编译（需要较长时间）
make -j4
make install
```

## macOS 环境搭建

### 使用 Homebrew（推荐）

#### 1. 安装 Homebrew
```bash
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
```

#### 2. 安装 Xcode Command Line Tools
```bash
xcode-select --install
```

#### 3. 安装 FFmpeg 和 CMake
```bash
# 安装 FFmpeg 开发库
brew install ffmpeg

# 安装 CMake
brew install cmake

# 安装 pkg-config（用于查找库文件）
brew install pkg-config
```

#### 4. 验证安装
```bash
ffmpeg -version
cmake --version
pkg-config --modversion libavcodec
```

### 使用 MacPorts（备选方案）

#### 1. 安装 MacPorts
下载并安装 [MacPorts](https://www.macports.org/install.php)

#### 2. 安装依赖
```bash
sudo port install ffmpeg +universal
sudo port install cmake
```

## Linux 环境搭建

### Ubuntu/Debian 系列

#### 1. 更新包管理器
```bash
sudo apt update && sudo apt upgrade -y
```

#### 2. 安装编译工具
```bash
# 基础编译工具
sudo apt install build-essential cmake git

# FFmpeg 开发库
sudo apt install libavformat-dev libavcodec-dev libavdevice-dev \
                 libavfilter-dev libavutil-dev libswscale-dev \
                 libswresample-dev pkg-config
```

#### 3. 安装额外的编解码器（可选）
```bash
sudo apt install libx264-dev libx265-dev libvpx-dev \
                 libfdk-aac-dev libmp3lame-dev libopus-dev
```

### CentOS/RHEL/Fedora 系列

#### CentOS/RHEL 8+
```bash
# 安装 EPEL 仓库
sudo dnf install epel-release

# 安装 RPM Fusion
sudo dnf install https://download1.rpmfusion.org/free/el/rpmfusion-free-release-8.noarch.rpm

# 安装开发工具
sudo dnf groupinstall "Development Tools"
sudo dnf install cmake git

# 安装 FFmpeg 开发库
sudo dnf install ffmpeg-devel
```

#### Fedora
```bash
# 安装 RPM Fusion
sudo dnf install https://download1.rpmfusion.org/free/fedora/rpmfusion-free-release-$(rpm -E %fedora).noarch.rpm

# 安装依赖
sudo dnf install gcc gcc-c++ cmake git
sudo dnf install ffmpeg-devel
```

### Arch Linux
```bash
# 安装基础工具
sudo pacman -S base-devel cmake git

# 安装 FFmpeg
sudo pacman -S ffmpeg
```

## 验证安装

创建一个简单的测试程序来验证环境是否正确配置：

### 1. 创建测试文件
创建 `test_ffmpeg.cpp`：
```cpp
#include <iostream>
extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/avutil.h>
}

int main() {
    std::cout << "FFmpeg version: " << av_version_info() << std::endl;
    std::cout << "libavformat version: " << avformat_version() << std::endl;
    std::cout << "libavcodec version: " << avcodec_version() << std::endl;
    std::cout << "libavutil version: " << avutil_version() << std::endl;
    
    // 初始化 FFmpeg
    av_register_all();
    avcodec_register_all();
    
    std::cout << "FFmpeg 初始化成功！" << std::endl;
    return 0;
}
```

### 2. 创建 CMakeLists.txt
```cmake
cmake_minimum_required(VERSION 3.10)
project(test_ffmpeg)

set(CMAKE_CXX_STANDARD 11)

# 查找 FFmpeg 库
find_package(PkgConfig REQUIRED)
pkg_check_modules(FFMPEG REQUIRED libavformat libavcodec libavutil)

# 创建可执行文件
add_executable(test_ffmpeg test_ffmpeg.cpp)

# 链接库
target_link_libraries(test_ffmpeg ${FFMPEG_LIBRARIES})
target_include_directories(test_ffmpeg PRIVATE ${FFMPEG_INCLUDE_DIRS})
target_compile_options(test_ffmpeg PRIVATE ${FFMPEG_CFLAGS_OTHER})
```

### 3. 编译并运行
```bash
mkdir build && cd build
cmake ..
make
./test_ffmpeg
```

如果看到版本信息输出，说明环境配置成功！

## 常见问题解决

### Q1: pkg-config 找不到 FFmpeg 库
**Linux 解决方案：**
```bash
# 检查 pkg-config 路径
export PKG_CONFIG_PATH=/usr/local/lib/pkgconfig:$PKG_CONFIG_PATH

# 或者手动指定库路径
sudo ldconfig /usr/local/lib
```

**macOS 解决方案：**
```bash
# 检查 Homebrew 路径
export PKG_CONFIG_PATH="/opt/homebrew/lib/pkgconfig:$PKG_CONFIG_PATH"
```

### Q2: Windows 上找不到头文件
确保在 CMakeLists.txt 中正确使用 vcpkg：
```cmake
set(CMAKE_TOOLCHAIN_FILE "C:/vcpkg/scripts/buildsystems/vcpkg.cmake")
```

### Q3: 链接错误 "undefined reference"
在 CMakeLists.txt 中添加所有必要的库：
```cmake
target_link_libraries(your_target 
    avformat avcodec avutil avdevice avfilter swscale swresample
)
```

### Q4: macOS 上权限问题
屏幕录制需要授权：
1. 系统偏好设置 → 安全性与隐私 → 隐私
2. 选择 "屏幕录制"
3. 添加你的应用程序

### Q5: 编译时间过长
使用多核编译：
```bash
# Linux/macOS
make -j$(nproc)

# Windows (Visual Studio)
msbuild /m YourProject.sln
```

## IDE 配置

### Visual Studio Code

#### 1. 安装扩展
- C/C++
- CMake Tools
- CMake

#### 2. 配置 settings.json
```json
{
    "cmake.configureSettings": {
        "CMAKE_TOOLCHAIN_FILE": "C:/vcpkg/scripts/buildsystems/vcpkg.cmake"
    },
    "C_Cpp.default.includePath": [
        "${workspaceFolder}/**",
        "/usr/local/include/**"
    ]
}
```

### CLion

#### 1. 配置 CMake
- File → Settings → Build, Execution, Deployment → CMake
- 添加 CMake options: `-DCMAKE_TOOLCHAIN_FILE=C:/vcpkg/scripts/buildsystems/vcpkg.cmake`

#### 2. 配置库路径
- File → Settings → Build, Execution, Deployment → CMake
- 在 Environment variables 中添加：
  - `PKG_CONFIG_PATH=/usr/local/lib/pkgconfig`

### Qt Creator

#### 1. 配置 Kit
- Tools → Options → Kits
- 添加新的 Kit，配置正确的 CMake 路径

#### 2. 项目配置
在项目设置中添加 CMake 参数：
```
-DCMAKE_TOOLCHAIN_FILE=C:/vcpkg/scripts/buildsystems/vcpkg.cmake
```

## 进阶配置

### 自定义 FFmpeg 编译

如果需要特定的编解码器支持，可以自定义编译 FFmpeg：

```bash
# 下载源码
git clone https://git.ffmpeg.org/ffmpeg.git
cd ffmpeg

# 查看编译选项
./configure --help

# 自定义配置示例
./configure \
    --prefix=/usr/local \
    --enable-shared \
    --enable-libx264 \
    --enable-libx265 \
    --enable-libvpx \
    --enable-libfdk-aac \
    --enable-gpl \
    --enable-nonfree

# 编译并安装
make -j$(nproc)
sudo make install
sudo ldconfig
```

### 交叉编译配置

#### 为 ARM 平台编译（树莓派）
```bash
# 安装交叉编译工具链
sudo apt install gcc-arm-linux-gnueabihf

# 配置 FFmpeg
./configure \
    --cross-prefix=arm-linux-gnueabihf- \
    --arch=arm \
    --target-os=linux \
    --enable-cross-compile
```

## 下一步

环境搭建完成后，建议：

1. 阅读 [项目主页](../README.md) 了解各个示例模块
2. 从简单的 `pts-timestamp` 示例开始
3. 逐步尝试更复杂的 `ScreenRecord` 和 `Filter` 模块
4. 参考 [FFmpeg 官方文档](https://ffmpeg.org/documentation.html) 深入学习

## 技术支持

如果在环境搭建过程中遇到问题：

1. 检查本文档的常见问题部分
2. 在项目 Issues 中搜索类似问题
3. 提交新的 Issue，包含详细的错误信息和系统环境

---

⚠️ **重要提示**: 第一次搭建环境可能需要较长时间，特别是编译 FFmpeg 的过程。建议预留 1-2 小时，并保持网络连接稳定。