#include "NetworkClient.h"
#include <iostream>

#pragma comment(lib, "ws2_32.lib")

// [11.2] Windows에서 소켓을 사용할 준비. 연결 자체는 아직 하지 않는다.
bool NetworkClient::InitNetwork()
{
    if (initialized) return true;

    WSADATA wsaData{};
    const int error = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (error != 0) {
        std::cerr << "WSAStartup failed: " << error << std::endl;
        return false;
    }

    initialized = true;
    return true;
}

// [11.2] 숫자 IPv4 주소로 TCP 연결. 예: "127.0.0.1", 기본 포트는 protocol.h의 9000.
bool NetworkClient::ConnectToServer(const char* ip, unsigned short port)
{
    if (!initialized) {
        std::cerr << "Call InitNetwork before ConnectToServer." << std::endl;
        return false;
    }
    if (IsConnected()) {
        std::cerr << "Already connected. Call Disconnect before reconnecting." << std::endl;
        return false;
    }

    sockaddr_in serverAddress{};
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(port);
    if (ip == nullptr || inet_pton(AF_INET, ip, &serverAddress.sin_addr) != 1) {
        std::cerr << "Invalid IPv4 address." << std::endl;
        return false;
    }

    sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock == INVALID_SOCKET) {
        std::cerr << "socket failed: " << WSAGetLastError() << std::endl;
        return false;
    }

    // 설계대로 최초 connect는 연결 성공/실패가 결정될 때까지 기다린다.
    if (connect(sock, reinterpret_cast<const sockaddr*>(&serverAddress), sizeof(serverAddress)) == SOCKET_ERROR) {
        std::cerr << "connect failed: " << WSAGetLastError() << std::endl;
        Disconnect();
        return false;
    }

    // 작은 게임 요청을 모아 두느라 지연하지 않도록 Nagle 알고리즘을 끈다.
    // 공용 NetUtil은 아직 없으므로 이번 단계에서는 해당 Windows API를 직접 호출한다.
    const BOOL noDelay = TRUE;
    if (setsockopt(sock, IPPROTO_TCP, TCP_NODELAY,
        reinterpret_cast<const char*>(&noDelay), sizeof(noDelay)) == SOCKET_ERROR) {
        std::cerr << "TCP_NODELAY failed: " << WSAGetLastError() << std::endl;
        Disconnect();
        return false;
    }

    // 연결 이후에는 받을 데이터가 없어도 기다리지 않는다. 단일 GLUT 스레드를 위한 설정이다.
    u_long nonBlocking = 1;
    if (ioctlsocket(sock, FIONBIO, &nonBlocking) == SOCKET_ERROR) {
        std::cerr << "FIONBIO failed: " << WSAGetLastError() << std::endl;
        Disconnect();
        return false;
    }

    return true;
}

// [11.2] 현재 클라이언트가 연결 소켓을 소유하는지 확인한다.
// 상대가 끊었는지는 다음 단계의 recv 결과(0 또는 오류)를 통해 판단한다.
bool NetworkClient::IsConnected() const
{
    return sock != INVALID_SOCKET;
}

// [11.2] 서버 연결만 끊는다. Winsock은 유지하므로 다시 연결할 수 있다.
void NetworkClient::Disconnect()
{
    if (sock == INVALID_SOCKET) return;
    closesocket(sock);
    sock = INVALID_SOCKET;
}

// [11.2] 연결을 끊고 Windows 소켓 사용을 마무리한다.
// 이후 GameClient의 종료 흐름에서 호출한다. 현재 게임 Main/OnExit에는 아직 연결하지 않는다.
void NetworkClient::Shutdown()
{
    Disconnect();
    if (initialized) {
        WSACleanup();
        initialized = false;
    }
}
