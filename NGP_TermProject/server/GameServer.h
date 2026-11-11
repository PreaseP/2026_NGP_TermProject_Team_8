#pragma once
#include "../common/Common.h"
#include "ThreadQueue.h"
#include "GameSession.h"
#include "Room.h"
#include <windows.h>

constexpr int MAX_LOGIN_USERS = 12;

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
    void HandleLogin(GameSession* session, const C2S_Login& packet); // 로그인 검증 및 결과 전송
    void HandleJoinRoom(GameSession* session, const C2S_JoinRoom& packet); // 방 입장 요청 처리
    void LeaveRoomInternal(GameSession* session); // 방 퇴장 처리
    GameSession* FindGameSession(int sessionId); // 연결 번호로 게임 세션 찾기
    bool IsValidName(const char* name);
    void SendPacket(int sessionId, const void* packet);
    void SendToLobby(const void* packet);
    void SendRoomList(int sessionId);
    void BuildRoomList(S2C_RoomList* out);
    void SendLoginFailAndClose(int sessionId, LOGIN_RESULT result);
};
