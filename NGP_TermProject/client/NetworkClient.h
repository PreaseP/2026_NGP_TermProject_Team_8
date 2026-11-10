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
#include <vector> // [11.3] 보낼 바이트를 저장할 송신 버퍼
#include "../common/protocol.h"

// [11.3] 송수신 결과. 정상 / 지금은 처리 불가 / 연결 없음·종료 / 오류
enum NET_RESULT { NET_OK, NET_WOULD_BLOCK, NET_CLOSED, NET_ERROR };

// [11.2] 서버 연결을 담당한다. 송수신 기능은 11/3 일정에서 추가한다.
class NetworkClient {
public:
    NetworkClient() = default;
    NetworkClient(const NetworkClient&) = delete;
    NetworkClient& operator=(const NetworkClient&) = delete;

    bool InitNetwork();
    bool ConnectToServer(const char* ip, unsigned short port = SERVER_PORT);
    bool IsConnected() const;
    bool QueuePacket(const void* packet); // [11.3] 패킷을 송신 버퍼 뒤에 복사
    NET_RESULT FlushSendBuffer(); // [11.3] 기존 송신 버퍼의 데이터를 실제로 보낸다.
    NET_RESULT ReceiveData(); // [11.3] 받은 바이트를 수신 버퍼 뒤에 모은다.
   
    // NET_OK: 하나 꺼냈음 / NET_WOULD_BLOCK: 데이터가 부족함 / NET_ERROR: out이 없거나 패킷 길이가 잘못됨
    NET_RESULT ExtractPacket(unsigned char* out);
    void Disconnect();
    void Shutdown();

private:
    SOCKET sock = INVALID_SOCKET; // INVALID_SOCKET이면 소유한 연결이 없다.
    bool initialized = false;    // WSAStartup 성공 여부. 성공한 횟수에 맞춰 WSACleanup한다.
    std::vector<unsigned char> sendBuffer; // [11.3] 아직 보내지 않은 바이트
    std::vector<unsigned char> recvBuffer; // [11.3] 패킷 추출 전까지 보관할 수신 바이트
};
