
# Client-Server 项目说明

## 📁 项目结构

```
├─ Client
│  ├─ Client
│  └─ Client.sln
├─ libusac
│  ├─ bin
│  │  ├─ dll_superframe.dll
│  │  ├─ libxaacdec.dll
│  │  └─ usacEnc.dll
│  ├─ include
│  │  ├─ libsuperframe_export.h
│  │  ├─ libxaacd_export.h
│  │  └─ libxaacempeg_export.h
│  └─ lib
│     ├─ dll_superframe.lib
│     ├─ libxaacdec.lib
│     └─ usacEnc.lib
├─ README.md
├─ Release
│  ├─ Client_boxed.exe
│  └─ Server_boxed.exe
└─ Server
   ├─ Server
   └─ Server.sln

```
## 📈 流程结构

![流程图](https://raw.githubusercontent.com/xdzqyyds/draw.io/main/client-server.svg)

## 🚀 使用方法

- 自行调整码率`Bitrate`和采样率`SampleRate`
- 位深度`PCM Bit Width`目前只支持`16`位
- 自行调整立体声开关`stereo`
- `SBR`与`MPS`根据上述配置自动开启，无需选择

## ⚡ 编译说明

### 环境配置

本项目基于 **Visual Studio 2022** 和 **Qt 5.12** 开发，在使用前请确保已正确配置好相关开发环境。
- 编码器封装自：[ISO-IEC-USAC](https://github.com/xdzqyyds/ISO-IEC-USAC)
- 解码器封装自：[libxaac](https://github.com/ittiam-systems/libxaac)
- 参考配置教程：[VS2022配置Qt开发环境](https://blog.csdn.net/MelyLenient/article/details/123854069)

### 编译步骤

1. 使用 Visual Studio 打开 `Client.sln` 或 `Server.sln`。
2. 编译时若提示缺少 `.dll` 文件，请将 `libusac/bin/` 目录下的：
   - `libxaacdec.dll`
   - `dll_superframe.dll`
   - `usacEnc.dll`
   
   复制到生成的可执行文件目录，例如：
   - `Client/x64/Debug/` 或 `Client/x64/Release/`
   - `Server/x64/Debug/` 或 `Server/x64/Release/`
   
3. 编译并运行 `Client` 和 `Server` 项目，即可进行通信测试。

## ⚡ 注意事项

- **发送和接收前必须正确初始化音频模块**，否则程序可能异常退出。
- 播放音频时可能出现不连续的现象。
- **支持的音频码率**范围：
  - 最低：20 kbps
  - 最高：80 kbps
  - 推荐：64 kbps
- **采样率支持情况**：
  - 单声道（Mono）：32000 Hz、44100 Hz、48000 Hz
  - 立体声（Stereo）：32000 Hz、44100 Hz、48000 Hz
- 测试音频采集模式时，建议佩戴耳机，以避免啸叫现象。
- 默认 TCP 通信地址为：`127.0.0.1:12345`，如需更改可在源码中修改。

## 📋 其他说明

- `libusac` 目录包含了基于 USAC（统一音频编解码）标准的音频处理库。
- `release` 目录下包含打包好的可执行文件，便于直接测试运行。

## ▶️ 效果演示

https://github.com/user-attachments/assets/173c1fc2-cd74-4269-9a4d-d7b8b49a5f94
