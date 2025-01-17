#include <iostream>
#include <string>
#include <thread>
#include <winsock2.h>
#include <windows.h>
#include <iphlpapi.h>
#include <vector>
#include <algorithm>
#include <cstdlib>
#include <cstdio>
#include <sstream>

#pragma comment(lib, "iphlpapi.lib")
#pragma comment(lib, "ws2_32.lib")

// 获取本机IP地址
std::string getLocalIP() {
    std::string ip;
    PIP_ADAPTER_INFO adapterInfo;
    ULONG bufferSize = sizeof(IP_ADAPTER_INFO);
    adapterInfo = (IP_ADAPTER_INFO*)malloc(bufferSize);

    // 获取适配器信息
    if (GetAdaptersInfo(adapterInfo, &bufferSize) == ERROR_BUFFER_OVERFLOW) {
        free(adapterInfo);
        adapterInfo = (IP_ADAPTER_INFO*)malloc(bufferSize);
    }

    if (GetAdaptersInfo(adapterInfo, &bufferSize) == NO_ERROR) {
        PIP_ADAPTER_INFO adapter = adapterInfo;
        while (adapter) {
            if (adapter->Type == MIB_IF_TYPE_ETHERNET) {
                ip = adapter->IpAddressList.IpAddress.String;
                break;
            }
            adapter = adapter->Next;
        }
    }

    free(adapterInfo);
    return ip;
}

// 检测并添加开机启动项
void checkAndAddToStartup(const std::string& programPath) {
    HKEY hKey;
    std::string keyName = "Software\\Microsoft\\Windows\\CurrentVersion\\Run";

    // 打开注册表项
    if (RegOpenKeyExA(HKEY_CURRENT_USER, keyName.c_str(), 0, KEY_READ | KEY_WRITE, &hKey) == ERROR_SUCCESS) {
        char buffer[MAX_PATH];
        DWORD bufferSize = sizeof(buffer);

        // 检查是否已经存在
        if (RegQueryValueExA(hKey, "myPCscreenmirrorReceiver", nullptr, nullptr, (LPBYTE)buffer, &bufferSize) != ERROR_SUCCESS) {
            // 添加开机启动项
            RegSetValueExA(hKey, "myPCscreenmirrorReceiver", 0, REG_SZ, (const BYTE*)programPath.c_str(), programPath.size() + 1);
        }

        RegCloseKey(hKey);
    }
}

// 显示推流信息
void displayStreamInfo(const std::string& ip, int port) {
    std::stringstream ss;
    ss << "请将视频推流到 udp://" << ip << ":" << port;
    ss << "\n按下 OK 退出\n";
    ss << "发送端连接结束请发送 curl -X POST -d \"stop\" "<<"http://" << ip << ":" << port + 1 ;
    std::string message = ss.str();

    // 使用MessageBox显示消息框
    MessageBoxA(nullptr, message.c_str(), "推流信息", MB_OK | MB_ICONINFORMATION);
}

// 运行FFplay并返回进程句柄
HANDLE runFFplay(int port) {
    STARTUPINFOA si = { sizeof(si) };
    PROCESS_INFORMATION pi;
    std::stringstream command;
    command << "ffplay -fs -vcodec h264_qsv udp://0.0.0.0:" << port;

    // 创建进程，隐藏控制台窗口
    if (CreateProcessA(
            nullptr,                   // 不指定模块名（使用命令行）
            const_cast<LPSTR>(command.str().c_str()), // 命令行
            nullptr,                   // 进程句柄不可继承
            nullptr,                   // 线程句柄不可继承
            FALSE,                     // 不继承句柄
            CREATE_NO_WINDOW,          // 隐藏控制台窗口
            nullptr,                   // 使用父进程环境变量
            nullptr,                   // 使用父进程工作目录
            &si,                       // 启动信息
            &pi                        // 进程信息
        )) {
        // 关闭线程句柄（不需要）
        CloseHandle(pi.hThread);
        return pi.hProcess; // 返回进程句柄
    } else {
        std::cerr << "无法启动ffplay进程！错误代码: " << GetLastError() << std::endl;
        return nullptr;
    }
}

// 简单的HTTP服务器
void startHttpServer(int httpPort, HANDLE& ffplayHandle, int port) {
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "WSAStartup failed." << std::endl;
        return;
    }

    SOCKET serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket == INVALID_SOCKET) {
        std::cerr << "Socket creation failed." << std::endl;
        WSACleanup();
        return;
    }

    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(httpPort);

    if (bind(serverSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        std::cerr << "Bind failed." << std::endl;
        closesocket(serverSocket);
        WSACleanup();
        return;
    }

    if (listen(serverSocket, 5) == SOCKET_ERROR) {
        std::cerr << "Listen failed." << std::endl;
        closesocket(serverSocket);
        WSACleanup();
        return;
    }

    while (true) {
        SOCKET clientSocket = accept(serverSocket, nullptr, nullptr);
        if (clientSocket == INVALID_SOCKET) {
            std::cerr << "Accept failed." << std::endl;
            continue;
        }

        char buffer[1024];
        int bytesReceived = recv(clientSocket, buffer, sizeof(buffer), 0);
        if (bytesReceived > 0) {
            buffer[bytesReceived] = '\0';
            std::string request(buffer);

            if (request.find("POST /") != std::string::npos) {
                std::string body = request.substr(request.find("\r\n\r\n") + 4);
                if (body.find("stop") != std::string::npos) {
                    // 终止FFplay进程
                    if (ffplayHandle) {
                        TerminateProcess(ffplayHandle, 0);
                        CloseHandle(ffplayHandle);
                        ffplayHandle = nullptr;
                    }

                    // 5秒后重启FFplay
                    // std::this_thread::sleep_for(std::chrono::seconds(5));
                    ffplayHandle = runFFplay(port);
                }
            }
        }

        closesocket(clientSocket);
    }

    closesocket(serverSocket);
    WSACleanup();
}

int main(int argc, char* argv[]) {
    // 隐藏主程序的控制台窗口
    ShowWindow(GetConsoleWindow(), SW_HIDE);

    int port = 12345; // 默认端口

    // 解析命令行参数
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "-p" && i + 1 < argc) {
            port = std::atoi(argv[++i]);
        }
    }

    // 动态获取程序路径
    char programPath[MAX_PATH];
    GetModuleFileNameA(nullptr, programPath, MAX_PATH);

    // 检测并添加开机启动项
    checkAndAddToStartup(programPath);

    // 获取本机IP地址
    std::string ip = getLocalIP();

    // 启动FFplay并获取进程句柄
    HANDLE ffplayHandle = runFFplay(port);
    if (!ffplayHandle) {
        MessageBoxA(nullptr, "ffplay启动失败，程序退出。", "错误", MB_OK | MB_ICONERROR);
        return 1;
    }

    // 启动HTTP服务器
    int httpPort = port + 1;
    std::thread httpServerThread(startHttpServer, httpPort, std::ref(ffplayHandle), port);
    httpServerThread.detach();

    // 显示推流信息
    //等待5s
    std::this_thread::sleep_for(std::chrono::seconds(5));
    displayStreamInfo(ip, port);

    // 终止FFplay进程
    if (ffplayHandle) {
        TerminateProcess(ffplayHandle, 0);
        CloseHandle(ffplayHandle);
    }

    return 0;
}