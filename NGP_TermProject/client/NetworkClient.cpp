#include "NetworkClient.h"
#include <iostream>
#include <algorithm>
#include <limits>

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

// [11.3] 패킷을 저장만 한다. 실제 send()는 다음에 만들 FlushSendBuffer가 담당한다.
// 호출자는 첫 바이트(size)만큼 유효한 protocol.h 패킷을 전달한다.
bool NetworkClient::QueuePacket(const void* packet)
{
    if (!IsConnected() || packet == nullptr) return false;

    // 바이트 단위로 읽기 위해 포인터를 변환한다. 첫 바이트가 헤더 포함 전체 길이다.
    const unsigned char* bytes = static_cast<const unsigned char*>(packet);
    const unsigned char packetSize = bytes[0];
    if (packetSize < sizeof(PacketHeader)) return false;
    // packetSize가 1바이트이므로 최대 255까지 표현된다.

    sendBuffer.insert(sendBuffer.end(), bytes, bytes + packetSize);
    return true;
}

// [11.3] QueuePacket에 저장되어있는 패킷들(sendBuffer)를 send()에 넘긴다.
NET_RESULT NetworkClient::FlushSendBuffer()
{
    if (!IsConnected()) return NET_CLOSED;

    while (!sendBuffer.empty()) {
        //목적은 send()가 요구하는 int 타입으로 바이트 수를 맞추는 것
        const std::size_t maxSendSize = std::numeric_limits<int>::max(); // 32비트 int의 최댓값은 2,147,483,647
        const int sendSize = static_cast<int>(std::min(sendBuffer.size(), maxSendSize));
        const int sent = send(sock, reinterpret_cast<const char*>(sendBuffer.data()), sendSize, 0);

        if (sent == SOCKET_ERROR) {
            const int error = WSAGetLastError();
            //송신할 여유가 부족하면 send()가 SOCKET_ERROR를 반환하고 오류 코드가 WSAEWOULDBLOCK이 될 수 있다.
            // 지금 보낼 수 없으면 남은 데이터를 그대로 두고 다음 호출에서 다시 시도한다.
            if (error == WSAEWOULDBLOCK) return NET_WOULD_BLOCK;

            std::cerr << "send failed: " << error << std::endl;
            return NET_ERROR;
        }
        if (sent == 0) return NET_CLOSED;

        // 일부만 처리됐다면 그 부분만 지운다. 나머지는 반복해서 전송을 시도한다.
        sendBuffer.erase(sendBuffer.begin(), sendBuffer.begin() + sent); // 처음부터 sent 직전까지
    }

    return NET_OK;
}

// [11.2] 서버 연결만 끊는다. Winsock은 유지하므로 다시 연결할 수 있다.
void NetworkClient::Disconnect()
{
    if (sock != INVALID_SOCKET) {
        closesocket(sock);
        sock = INVALID_SOCKET;
    }
    sendBuffer.clear(); // [11.3] 이전 연결의 미전송 요청을 다음 연결에 남기지 않는다.
    recvBuffer.clear(); // [11.3] 이전 연결의 수신 바이트를 다음 연결에 남기지 않는다.
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

// [11.3] 서버에서 도착한 데이터를 읽어서, 나중에 사용할 수 있도록 recvBuffer에 모아 두는 함수
NET_RESULT NetworkClient::ReceiveData()
{
    if (!IsConnected()) return NET_CLOSED;

    unsigned char buffer[4096];
    bool receivedAny = false; //이번 호출에서 데이터를 받았는지 기록한다.

    while (true) { //지금 받을 데이터가 없을 때까지 반복한다.
        const int received = recv(sock, reinterpret_cast<char*>(buffer), static_cast<int>(sizeof(buffer)), 0);

        if (received == SOCKET_ERROR) {
            const int error = WSAGetLastError();
            //도착한 데이터가 없으면 기다리지 않고 돌아간다.
            if (error == WSAEWOULDBLOCK) {
                if (receivedAny) {
                    return NET_OK;
                }
                return NET_WOULD_BLOCK;
            }

            std::cerr << "recv failed: " << error << std::endl;
            return NET_ERROR;
        }
        if (received == 0) return NET_CLOSED; //서버의 정상적인 연결 종료

        //실제로 받은 바이트만 기존 수신 데이터 뒤에 붙인다.
        recvBuffer.insert(recvBuffer.end(), buffer, buffer + received);
        receivedAny = true;
    }
}
