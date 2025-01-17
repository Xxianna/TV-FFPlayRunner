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

// ��ȡ����IP��ַ
std::string getLocalIP() {
    std::string ip;
    PIP_ADAPTER_INFO adapterInfo;
    ULONG bufferSize = sizeof(IP_ADAPTER_INFO);
    adapterInfo = (IP_ADAPTER_INFO*)malloc(bufferSize);

    // ��ȡ��������Ϣ
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

// ��Ⲣ��ӿ���������
void checkAndAddToStartup(const std::string& programPath) {
    HKEY hKey;
    std::string keyName = "Software\\Microsoft\\Windows\\CurrentVersion\\Run";

    // ��ע�����
    if (RegOpenKeyExA(HKEY_CURRENT_USER, keyName.c_str(), 0, KEY_READ | KEY_WRITE, &hKey) == ERROR_SUCCESS) {
        char buffer[MAX_PATH];
        DWORD bufferSize = sizeof(buffer);

        // ����Ƿ��Ѿ�����
        if (RegQueryValueExA(hKey, "myPCscreenmirrorReceiver", nullptr, nullptr, (LPBYTE)buffer, &bufferSize) != ERROR_SUCCESS) {
            // ��ӿ���������
            RegSetValueExA(hKey, "myPCscreenmirrorReceiver", 0, REG_SZ, (const BYTE*)programPath.c_str(), programPath.size() + 1);
        }

        RegCloseKey(hKey);
    }
}

// ��ʾ������Ϣ
void displayStreamInfo(const std::string& ip, int port) {
    std::stringstream ss;
    ss << "�뽫��Ƶ������ udp://" << ip << ":" << port;
    ss << "\n���� OK �˳�\n";
    ss << "���Ͷ����ӽ����뷢�� curl -X POST -d \"stop\" "<<"http://" << ip << ":" << port + 1 ;
    std::string message = ss.str();

    // ʹ��MessageBox��ʾ��Ϣ��
    MessageBoxA(nullptr, message.c_str(), "������Ϣ", MB_OK | MB_ICONINFORMATION);
}

// ����FFplay�����ؽ��̾��
HANDLE runFFplay(int port) {
    STARTUPINFOA si = { sizeof(si) };
    PROCESS_INFORMATION pi;
    std::stringstream command;
    command << "ffplay -fs -vcodec h264_qsv udp://0.0.0.0:" << port;

    // �������̣����ؿ���̨����
    if (CreateProcessA(
            nullptr,                   // ��ָ��ģ������ʹ�������У�
            const_cast<LPSTR>(command.str().c_str()), // ������
            nullptr,                   // ���̾�����ɼ̳�
            nullptr,                   // �߳̾�����ɼ̳�
            FALSE,                     // ���̳о��
            CREATE_NO_WINDOW,          // ���ؿ���̨����
            nullptr,                   // ʹ�ø����̻�������
            nullptr,                   // ʹ�ø����̹���Ŀ¼
            &si,                       // ������Ϣ
            &pi                        // ������Ϣ
        )) {
        // �ر��߳̾��������Ҫ��
        CloseHandle(pi.hThread);
        return pi.hProcess; // ���ؽ��̾��
    } else {
        std::cerr << "�޷�����ffplay���̣��������: " << GetLastError() << std::endl;
        return nullptr;
    }
}

// �򵥵�HTTP������
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
                    // ��ֹFFplay����
                    if (ffplayHandle) {
                        TerminateProcess(ffplayHandle, 0);
                        CloseHandle(ffplayHandle);
                        ffplayHandle = nullptr;
                    }

                    // 5�������FFplay
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
    // ����������Ŀ���̨����
    ShowWindow(GetConsoleWindow(), SW_HIDE);

    int port = 12345; // Ĭ�϶˿�

    // ���������в���
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "-p" && i + 1 < argc) {
            port = std::atoi(argv[++i]);
        }
    }

    // ��̬��ȡ����·��
    char programPath[MAX_PATH];
    GetModuleFileNameA(nullptr, programPath, MAX_PATH);

    // ��Ⲣ��ӿ���������
    checkAndAddToStartup(programPath);

    // ��ȡ����IP��ַ
    std::string ip = getLocalIP();

    // ����FFplay����ȡ���̾��
    HANDLE ffplayHandle = runFFplay(port);
    if (!ffplayHandle) {
        MessageBoxA(nullptr, "ffplay����ʧ�ܣ������˳���", "����", MB_OK | MB_ICONERROR);
        return 1;
    }

    // ����HTTP������
    int httpPort = port + 1;
    std::thread httpServerThread(startHttpServer, httpPort, std::ref(ffplayHandle), port);
    httpServerThread.detach();

    // ��ʾ������Ϣ
    //�ȴ�5s
    std::this_thread::sleep_for(std::chrono::seconds(5));
    displayStreamInfo(ip, port);

    // ��ֹFFplay����
    if (ffplayHandle) {
        TerminateProcess(ffplayHandle, 0);
        CloseHandle(ffplayHandle);
    }

    return 0;
}