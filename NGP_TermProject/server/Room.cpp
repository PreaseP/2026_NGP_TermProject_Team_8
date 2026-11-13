#include "Room.h"
#include "GameServer.h"

void Room::Init(int id, GameServer* owner)
{
    roomId = static_cast<unsigned char>(id);
    server = owner;
    state = ROOM_WAITING;
    resultEndMS = 0;
    nextSnowballID = OBJ_ID_SNOWBALL_START;
    wallHeight = 0.0f;
    nextItemMs = 0;

    for (int i = 0; i < MAX_ROOM_PLAYERS; ++i) {
        playerSessionIds[i] = INVALID_ID;
        selectedCharacters[i] = CHAR_NONE;
        players[i].Reset(CHAR_NONE, glm::vec3(0.0f));
    }
    for (Snowball& snowball : snowballs)
        snowball.Deactivate();
}

int Room::GetPlayerCount() const
{
    int count = 0;
    for (int i = 0; i < MAX_ROOM_PLAYERS; ++i) {
        if (playerSessionIds[i] != INVALID_ID)
            ++count;
    }
    return count;
}

void Room::SelectCharacter(int slot, CHARACTER_TYPE character)
{
    if (slot < 0 || slot >= MAX_ROOM_PLAYERS || playerSessionIds[slot] == INVALID_ID)
        return;
    if (state != ROOM_WAITING)
        return;

    selectedCharacters[slot] = character;
}

bool Room::CanStart()
{
    if (state != ROOM_WAITING || GetPlayerCount() != MAX_ROOM_PLAYERS)
        return false;

    for (int i = 0; i < MAX_ROOM_PLAYERS; ++i) {
        if (selectedCharacters[i] == CHAR_NONE)
            return false;
    }
    return true;
}

bool Room::StartGame(ULONGLONG now)
{
    if (!CanStart())
        return false;

    state = ROOM_PLAYING;
    ResetWorld();
    nextItemMs = now + ITEM_INTERVAL_MS;

    S2C_GameStartResult result{};
    result.size = sizeof(result);
    result.type = S2C_GAME_START_RESULT;
    result.success = true;
    for (int i = 0; i < MAX_ROOM_PLAYERS; ++i) {
        players[i].Reset(selectedCharacters[i], SPAWN_POS[i]);
        result.character[i] = selectedCharacters[i];
        result.x[i] = SPAWN_POS[i].x;
        result.y[i] = SPAWN_POS[i].y;
        result.z[i] = SPAWN_POS[i].z;
    }

    Broadcast(&result);
    return true;
}

void Room::ResetWorld()
{
    for (Snowball& snowball : snowballs)
        snowball.Deactivate();

    nextSnowballID = OBJ_ID_SNOWBALL_START;
    wallHeight = 0.0f;
}

void Room::SendTo(int slot, const void* packet) const
{
    if (slot < 0 || slot >= MAX_ROOM_PLAYERS || playerSessionIds[slot] == INVALID_ID)
        return;

    server->SendPacket(playerSessionIds[slot], packet);
}

void Room::Broadcast(const void* packet) const
{
    for (int i = 0; i < MAX_ROOM_PLAYERS; ++i)
        SendTo(i, packet);
}

void Room::BuildRoomInfo(S2C_RoomInfo* out) const
{
    out->size = sizeof(S2C_RoomInfo);
    out->type = S2C_ROOM_INFO;
    out->room_id = roomId;

    for (int i = 0; i < MAX_ROOM_PLAYERS; ++i) {
        RoomPlayerInfo& info = out->players[i];
        info = RoomPlayerInfo{};
        info.character = CHAR_NONE;

        const GameSession* session = (playerSessionIds[i] == INVALID_ID) ? nullptr : server->FindGameSession(playerSessionIds[i]);
        if (session == nullptr)
            continue;

        info.connected = true;
        memcpy(info.name, session->name, MAX_NAME_LEN);
        info.character = selectedCharacters[i];
    }
}

void Room::SendRoomInfo() const
{
    S2C_RoomInfo info{};
    BuildRoomInfo(&info);
    Broadcast(&info);
}
