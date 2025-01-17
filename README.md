# FFPlayRunner - 一个简单的Windows投屏接收端工具

# FFPlayRunner - A Simple Windows Screen Mirroring Receiver Tool

FFPlayRunner 是一个用于Windows的简单工具，用于接收来自其他设备的UDP音视频流，并将其通过FFplay播放。它特别适合用于将电脑屏幕投屏到电视或其他显示设备。

FFPlayRunner is a simple tool for Windows to receive UDP audio and video streams from other devices and play them via FFplay. It is particularly suitable for mirroring your computer screen to a TV or other display devices.

## 功能特点

## Features

- **自动开机启动**：程序会自动添加到Windows的开机启动项，无需手动启动。
- **UDP流接收**：通过UDP协议接收音视频流，支持H.264编码。
- **HTTP控制接口**：提供简单的HTTP接口，用于远程控制FFplay的启动和停止。
- **隐藏控制台窗口**：主程序运行时隐藏控制台窗口，适合作为后台服务运行。

- **Auto Startup**: The program automatically adds itself to Windows startup items, no need to manually start it.
- **UDP Stream Reception**: Receives audio and video streams via UDP protocol, supports H.264 encoding.
- **HTTP Control Interface**: Provides a simple HTTP interface for remotely controlling the start and stop of FFplay.
- **Hidden Console Window**: The main program hides the console window when running, suitable for running as a background service.

## 使用方法

## Usage

### 1. 下载并编译

### 1. Download and Compile

首先，确保你已经安装了Visual Studio和FFmpeg。将源码下载到本地，并使用Visual Studio编译。

First, make sure you have installed Visual Studio and FFmpeg. Download the source code locally and compile it using Visual Studio.

```bash
git clone https://github.com/yourusername/FFPlayRunner.git
cd FFPlayRunner
```

### 2. 运行程序

### 2. Run the Program

编译完成后，运行生成的`FFPlayRunner.exe`。程序会自动启动FFplay并监听UDP流。

After compiling, run the generated `FFPlayRunner.exe`. The program will automatically start FFplay and listen for UDP streams.

```bash
FFPlayRunner.exe
```

### 3. 获取推流信息

### 3. Get Streaming Information

程序启动后，会弹出一个消息框，显示本机的IP地址和UDP端口号。请将视频推流到该地址。

After the program starts, a message box will pop up displaying the local IP address and UDP port number. Please stream the video to this address.

```
请将视频推流到 udp://192.168.1.100:12345
按下 OK 退出
发送端连接结束请发送 curl -X POST -d "stop" http://192.168.1.100:12346
```

### 4. 推流

### 4. Stream Video

使用OBS或其他推流工具，将视频推流到显示的UDP地址。

Use OBS or other streaming tools to stream video to the displayed UDP address.

![OBS options](recv/obs.jpg)


### 5. 停止推流

### 5. Stop Streaming

当推流结束后，可以通过发送HTTP POST请求到`http://<IP>:<Port+1>`来停止FFplay。例如：

When the streaming ends, you can stop FFplay by sending an HTTP POST request to `http://<IP>:<Port+1>`. For example:

```bash
curl -X POST -d "stop" http://192.168.1.100:12346
```

## 命令行参数

## Command Line Arguments

- `-p <port>`: 指定UDP端口号，默认为12345。
- `-p <port>`: Specify the UDP port number, default is 12345.

```bash
FFPlayRunner.exe -p 12345
```

## 依赖

## Dependencies

- [FFmpeg](https://ffmpeg.org/) (Just need ffplay.exe)
- [OBS](https://obsproject.com/)

---

希望这个工具能帮助你轻松实现电脑屏幕的投屏！如果有任何问题或建议，欢迎在GitHub上提交Issue。

Hope this tool helps you easily achieve screen mirroring! If you have any questions or suggestions, please feel free to submit an Issue on GitHub.
