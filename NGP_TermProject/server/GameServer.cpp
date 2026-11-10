#include "GameServer.h"

DWORD WINAPI GameServer::ThreadEntry(LPVOID param)
{
    GameServer* gameServer = static_cast<GameServer*>(param);
    if (gameServer == nullptr)
        return 1;

    return gameServer->ThreadMain();
}

bool GameServer::Init(RecvEventQueue* recvQueue, SendRequestQueue* sendQueue, HANDLE stopEvent)
{
    if (recvQueue == nullptr || sendQueue == nullptr || stopEvent == nullptr)
        return false;

    server.recvQueue = recvQueue;
    server.sendQueue = sendQueue;
    server.stopEvent = stopEvent;
    loggedInCount = 0;

    for (GameSession& session : sessions)
        session.Reset();

    for (int i = 0; i < MAX_ROOMS; ++i)
        rooms[i].Init(i, this);

    return true;
}

void GameServer::ProcessRecvEvents()
{
    RecvEvent event;

    // 한 번에 큐 용량만큼만 처리해서 방 갱신이 밀리지 않게 한다.
    for (int i = 0; i < RECV_QUEUE_CAPACITY; ++i) {
        if (!server.recvQueue->TryDequeue(&event))
            break;

        switch (event.kind) {
        case RECV_EVENT_CONNECTED:
            OnConnected(event.sessionId);
            break;
        case RECV_EVENT_PACKET:
            OnPacket(event.sessionId, event.packetData, event.packetSize);
            break;
        case RECV_EVENT_DISCONNECTED:
            OnDisconnected(event.sessionId);
            break;
        }
    }
}

// 패킷 종류별 정상 크기. 클라이언트가 보내는 종류가 아니면 0.
static int ExpectedClientPacketSize(PACKET_TYPE type)
{
    switch (type) {
    case C2S_LOGIN:            return sizeof(C2S_Login);
    case C2S_JOIN_ROOM:        return sizeof(C2S_JoinRoom);
    case C2S_LEAVE_ROOM:       return sizeof(C2S_LeaveRoom);
    case C2S_SELECT_CHARACTER: return sizeof(C2S_SelectCharacter);
    case C2S_GAME_START:       return sizeof(C2S_GameStart);
    case C2S_MOVE:             return sizeof(C2S_Move);
    case C2S_CHARGE_START:     return sizeof(C2S_ChargeStart);
    case C2S_THROW:            return sizeof(C2S_Throw);
    case C2S_USE_ITEM:         return sizeof(C2S_UseItem);
    default:                   return 0;
    }
}

GameSession* GameServer::FindGameSession(int sessionId)
{
    for (GameSession& session : sessions) {
        if (session.inUse && session.sessionId == sessionId)
            return &session;
    }
    return nullptr;
}

void GameServer::OnPacket(int sessionId, const unsigned char* data, int size)
{
    GameSession* session = FindGameSession(sessionId);
    if (session == nullptr || data == nullptr || size < static_cast<int>(sizeof(PacketHeader)))
        return;

    const PacketHeader* header = reinterpret_cast<const PacketHeader*>(data);
    const int expectedSize = ExpectedClientPacketSize(header->type);
    if (expectedSize == 0 || header->size != size || size != expectedSize)
        return;

    // 로그인 전에는 로그인 요청만 받는다.
    if (!session->loggedIn && header->type != C2S_LOGIN)
        return;

    switch (header->type) {
    case C2S_LOGIN:
        HandleLogin(session, *reinterpret_cast<const C2S_Login*>(data));
        break;
    case C2S_JOIN_ROOM:
        HandleJoinRoom(session, *reinterpret_cast<const C2S_JoinRoom*>(data));
        break;
    case C2S_LEAVE_ROOM:
        LeaveRoomInternal(session);
        break;
    default:
        // 캐릭터 선택, 게임 시작, 이동, 차징, 던지기, 아이템 사용은 해당 일정에서 연결한다.
        break;
    }
}
