#pragma once

// [11.2] Winsock2를 Windows/OpenGL 헤더보다 먼저 읽도록 한다.
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <winsock2.h>
#include <ws2tcpip.h>
#include "../common/protocol.h"

// [11.2] 서버 연결을 담당한다. 송수신 기능은 11/3 일정에서 추가한다.
class NetworkClient {
public:
    NetworkClient() = default;
    NetworkClient(const NetworkClient&) = delete;
    NetworkClient& operator=(const NetworkClient&) = delete;

    bool InitNetwork();
    bool ConnectToServer(const char* ip, unsigned short port = SERVER_PORT);
    bool IsConnected() const;
    void Disconnect();
    void Shutdown();

private:
    SOCKET sock = INVALID_SOCKET; // INVALID_SOCKET이면 소유한 연결이 없다.
    bool initialized = false;    // WSAStartup 성공 여부. 성공한 횟수에 맞춰 WSACleanup한다.
};
