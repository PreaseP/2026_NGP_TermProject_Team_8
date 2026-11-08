#pragma once
#include "../common/Common.h"
#include "ThreadQueue.h"
#include "GameSession.h"
#include "Room.h"
#include <windows.h>

struct ServerContext {
    RecvEventQueue*    recvQueue = nullptr;
    SendRequestQueue*  sendQueue = nullptr;
    HANDLE             stopEvent = nullptr;
};

class GameServer {
public:
    ServerContext server;
    GameSession   sessions[MAX_CONNECTIONS];
    Room          rooms[MAX_ROOMS];
    int           loggedInCount = 0;

    static DWORD WINAPI ThreadEntry(LPVOID param); // 게임 스레드에서 방과 이벤트 처리
    bool Init(RecvEventQueue* recvQueue, SendRequestQueue* sendQueue, HANDLE stopEvent); // 게임 서버 초기화
    DWORD ThreadMain();
    void ProcessRecvEvents(); // 수신 큐의 이벤트 처리
    void OnConnected(int sessionId);
    void OnDisconnected(int sessionId);
    void OnPacket(int sessionId, const unsigned char* data, int size); // 클라이언트 패킷 검증 및 처리
};
