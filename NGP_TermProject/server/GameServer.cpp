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
    case C2S_SELECT_CHARACTER:
        HandleSelectCharacter(session, *reinterpret_cast<const C2S_SelectCharacter*>(data));
        break;
    case C2S_GAME_START:
        HandleGameStart(session, *reinterpret_cast<const C2S_GameStart*>(data));
        break;
    default:
        break;
    }
}

void GameServer::HandleLogin(GameSession* session, const C2S_Login& packet)
{
    if (session->loggedIn)
        return;

    // 정원이 차면 실패 응답 후 연결을 끊는다.
    if (loggedInCount >= MAX_LOGIN_USERS) {
        SendLoginFailAndClose(session->sessionId, LOGIN_SERVER_FULL);
        return;
    }

    // 클라이언트가 보낸 이름은 널 문자로 끝나지 않을 수 있어 복사해서 끝을 막는다.
    char name[MAX_NAME_LEN];
    memcpy(name, packet.name, MAX_NAME_LEN);
    name[MAX_NAME_LEN - 1] = '\0';

    S2C_LoginResult result{};
    result.size = sizeof(result);
    result.type = S2C_LOGIN_RESULT;
    result.session_id = session->sessionId;

    // 이름이 잘못되면 연결은 유지해서 다시 입력할 수 있게 한다.
    if (!IsValidName(name)) {
        result.result = LOGIN_INVALID_NAME;
        SendPacket(session->sessionId, &result);
        return;
    }

    memcpy(session->name, name, MAX_NAME_LEN);
    session->loggedIn = true;
    ++loggedInCount;

    result.result = LOGIN_OK;
    SendPacket(session->sessionId, &result);
    SendRoomList(session->sessionId);
}

void GameServer::HandleJoinRoom(GameSession* session, const C2S_JoinRoom& packet)
{
    // 이미 방에 있으면 새 입장 요청은 무시한다.
    if (session->roomId != INVALID_ID)
        return;

    S2C_JoinRoomResult result{};
    result.size = sizeof(result);
    result.type = S2C_JOIN_ROOM_RESULT;
    result.room_id = packet.room_id;
    result.my_slot = 0;

    int slot = INVALID_ID;
    if (packet.room_id >= MAX_ROOMS) {
        result.result = JOIN_INVALID_ROOM;
    }
    else {
        Room& room = rooms[packet.room_id];
        if (room.state != ROOM_WAITING) {
            result.result = JOIN_ROOM_PLAYING;
        }
        else if (room.GetPlayerCount() >= MAX_ROOM_PLAYERS) {
            result.result = JOIN_ROOM_FULL;
        }
        else {
            slot = room.AddPlayer(session->sessionId);
            result.result = (slot == INVALID_ID) ? JOIN_ROOM_FULL : JOIN_OK;
        }
    }

    if (result.result != JOIN_OK) {
        SendPacket(session->sessionId, &result);
        return;
    }

    session->roomId = packet.room_id;
    session->slot = slot;
    result.my_slot = static_cast<unsigned char>(slot);
    SendPacket(session->sessionId, &result);

    // 방 안 사람에게는 방 정보를, 로비에 남은 사람에게는 바뀐 방 목록을 보낸다.
    rooms[packet.room_id].SendRoomInfo();

    S2C_RoomList roomList{};
    BuildRoomList(&roomList);
    SendToLobby(&roomList);
}

void GameServer::LeaveRoomInternal(GameSession* session)
{
    if (session->roomId == INVALID_ID)
        return;

    const int roomId = session->roomId;
    Room& room = rooms[roomId];
    room.RemovePlayer(session->slot);

    session->roomId = INVALID_ID;
    session->slot = INVALID_ID;

    room.SendRoomInfo();

    S2C_RoomList roomList{};
    BuildRoomList(&roomList);
    SendToLobby(&roomList);
}

void GameServer::HandleSelectCharacter(GameSession* session, const C2S_SelectCharacter& packet)
{
    if (session->roomId == INVALID_ID)
        return;
    if (packet.character != CHAR_STEVE && packet.character != CHAR_ALEX)
        return;

    Room& room = rooms[session->roomId];
    if (room.state != ROOM_WAITING)
        return;

    room.SelectCharacter(session->slot, packet.character);
    room.SendRoomInfo();
}

void GameServer::HandleGameStart(GameSession* session, const C2S_GameStart&)
{
    if (session->roomId == INVALID_ID)
        return;

    Room& room = rooms[session->roomId];
    if (room.StartGame(GetTickCount64()))
        return;

    S2C_GameStartResult fail{};
    fail.size = sizeof(fail);
    fail.type = S2C_GAME_START_RESULT;
    fail.success = false;
    for (int i = 0; i < MAX_ROOM_PLAYERS; ++i)
        fail.character[i] = CHAR_NONE;
    SendPacket(session->sessionId, &fail);
}

void GameServer::SendPacket(int sessionId, const void* packet, bool closeAfterSend)
{
    const PacketHeader* header = static_cast<const PacketHeader*>(packet);
    if (header->size == 0 || header->size > BUF_SIZE)
        return;

    SendRequest request{};
    request.targetSessionId = sessionId;
    request.packetSize = header->size;
    memcpy(request.packetData, packet, header->size);
    request.closeAfterSend = closeAfterSend;

    server.sendQueue->Enqueue(request, closeAfterSend);
}

void GameServer::BuildRoomList(S2C_RoomList* out)
{
    out->size = sizeof(S2C_RoomList);
    out->type = S2C_ROOM_LIST;
    out->room_count = MAX_ROOMS;
    for (int i = 0; i < MAX_ROOMS; ++i) {
        out->rooms[i].room_id = rooms[i].roomId;
        out->rooms[i].player_count = static_cast<unsigned char>(rooms[i].GetPlayerCount());
        out->rooms[i].state = rooms[i].state;
    }
}

void GameServer::SendRoomList(int sessionId)
{
    S2C_RoomList roomList{};
    BuildRoomList(&roomList);
    SendPacket(sessionId, &roomList);
}
