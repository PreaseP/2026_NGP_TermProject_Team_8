#include "GameServer.h"

DWORD WINAPI GameServer::ThreadEntry(LPVOID param)
{
    GameServer* gameServer = static_cast<GameServer*>(param);
    if (gameServer == nullptr)
        return 1;

    ULONGLONG lastTick = GetTickCount64();

    while (WaitForSingleObject(gameServer->server.stopEvent, 1) == WAIT_TIMEOUT) {
        gameServer->ProcessRecvEvents();

        const ULONGLONG now = GetTickCount64();
        if (now - lastTick < TICK_MS)
            continue;

        const float dt = static_cast<float>(now - lastTick) / 1000.0f;
        lastTick = now;

        for (Room& room : gameServer->rooms) {
            if (room.state == ROOM_PLAYING)
                room.UpdateCharacters(now, dt);
        }
    }
    return 0;
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
        session = GameSession{};

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
            for (GameSession& session : sessions) {
                if (!session.inUse) {
                    session = GameSession{};
                    session.inUse = true;
                    session.sessionId = event.sessionId;
                    break;
                }
            }
            break;
        case RECV_EVENT_PACKET:
            OnPacket(event.sessionId, event.packetData, event.packetSize);
            break;
        case RECV_EVENT_DISCONNECTED: {
            GameSession* session = FindGameSession(event.sessionId);
            if (session == nullptr)
                break;

            LeaveRoomInternal(session);
            if (session->loggedIn)
                --loggedInCount;
            *session = GameSession{};
            break;
        }
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
    case C2S_SELECT_CHARACTER: {
        const C2S_SelectCharacter& packet = *reinterpret_cast<const C2S_SelectCharacter*>(data);
        if (session->roomId == INVALID_ID)
            break;
        if (packet.character != CHAR_STEVE && packet.character != CHAR_ALEX)
            break;

        Room& room = rooms[session->roomId];
        if (room.state != ROOM_WAITING)
            break;

        room.selectedCharacters[session->slot] = packet.character;
        room.SendRoomInfo();
        break;
    }
    case C2S_MOVE: {
        const C2S_Move& packet = *reinterpret_cast<const C2S_Move*>(data);
        if (session->roomId == INVALID_ID)
            break;

        Room& room = rooms[session->roomId];
        if (room.state != ROOM_PLAYING)
            break;

        room.players[session->slot].moveKeys = packet.move_keys & MOVE_KEY_MASK;
        break;
    }
    case C2S_CHARGE_START:
        if (session->roomId != INVALID_ID)
            rooms[session->roomId].StartCharge(session->slot, GetTickCount64());
        break;
    case C2S_THROW:
        if (session->roomId != INVALID_ID)
            rooms[session->roomId].ThrowSnowball(session->slot, GetTickCount64());
        break;
    case C2S_GAME_START: {
        if (session->roomId == INVALID_ID)
            break;

        Room& room = rooms[session->roomId];
        if (room.StartGame(GetTickCount64()))
            break;

        S2C_GameStartResult fail{};
        fail.size = sizeof(fail);
        fail.type = S2C_GAME_START_RESULT;
        fail.success = false;
        for (int i = 0; i < MAX_ROOM_PLAYERS; ++i)
            fail.character[i] = CHAR_NONE;
        SendPacket(session->sessionId, &fail);
        break;
    }
    default:
        break;
    }
}

void GameServer::HandleLogin(GameSession* session, const C2S_Login& packet)
{
    if (session->loggedIn)
        return;

    S2C_LoginResult result{};
    result.size = sizeof(result);
    result.type = S2C_LOGIN_RESULT;
    result.session_id = session->sessionId;

    // 정원이 차면 실패 응답 후 연결을 끊는다.
    if (loggedInCount >= MAX_LOGIN_USERS) {
        result.result = LOGIN_SERVER_FULL;
        SendPacket(session->sessionId, &result, true);
        return;
    }

    // 클라이언트가 보낸 이름은 널 문자로 끝나지 않을 수 있어 복사해서 끝을 막는다.
    char name[MAX_NAME_LEN];
    memcpy(name, packet.name, MAX_NAME_LEN);
    name[MAX_NAME_LEN - 1] = '\0';

    int length = 0;
    bool validName = true;
    while (name[length] != '\0') {
        const char c = name[length];
        const bool isAlnum = (c >= '0' && c <= '9') || (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
        if (!isAlnum)
            validName = false;
        ++length;
    }

    // 이름이 잘못되면 연결은 유지해서 다시 입력할 수 있게 한다.
    if (length == 0 || !validName) {
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
            for (int i = 0; i < MAX_ROOM_PLAYERS; ++i) {
                if (room.playerSessionIds[i] == INVALID_ID) {
                    slot = i;
                    break;
                }
            }

            if (slot == INVALID_ID) {
                result.result = JOIN_ROOM_FULL;
            }
            else {
                room.playerSessionIds[slot] = session->sessionId;
                room.selectedCharacters[slot] = CHAR_NONE;
                result.result = JOIN_OK;
            }
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
    SendRoomList(INVALID_ID);
}

void GameServer::LeaveRoomInternal(GameSession* session)
{
    if (session->roomId == INVALID_ID)
        return;

    const int roomId = session->roomId;
    Room& room = rooms[roomId];
    room.playerSessionIds[session->slot] = INVALID_ID;
    room.selectedCharacters[session->slot] = CHAR_NONE;

    session->roomId = INVALID_ID;
    session->slot = INVALID_ID;

    room.SendRoomInfo();
    SendRoomList(INVALID_ID);
}

void GameServer::SendPacket(int sessionId, const void* packet, bool closeAfterSend)
{
    SendRequest request{};
    request.targetSessionId = sessionId;
    request.closeAfterSend = closeAfterSend;

    if (packet != nullptr) {
        const PacketHeader* header = static_cast<const PacketHeader*>(packet);
        if (header->size == 0 || header->size > BUF_SIZE)
            return;

        request.packetSize = header->size;
        memcpy(request.packetData, packet, header->size);
    }
    else if (!closeAfterSend) {
        return;
    }

    server.sendQueue->Enqueue(request, closeAfterSend);
}

void GameServer::SendRoomList(int sessionId)
{
    S2C_RoomList roomList{};
    roomList.size = sizeof(S2C_RoomList);
    roomList.type = S2C_ROOM_LIST;
    roomList.room_count = MAX_ROOMS;
    for (int i = 0; i < MAX_ROOMS; ++i) {
        roomList.rooms[i].room_id = rooms[i].roomId;
        roomList.rooms[i].player_count = static_cast<unsigned char>(rooms[i].GetPlayerCount());
        roomList.rooms[i].state = rooms[i].state;
    }

    if (sessionId != INVALID_ID) {
        SendPacket(sessionId, &roomList);
        return;
    }

    for (GameSession& session : sessions) {
        if (session.inUse && session.loggedIn && session.roomId == INVALID_ID)
            SendPacket(session.sessionId, &roomList);
    }
}
