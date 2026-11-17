#include "Room.h"
#include <cmath>
#include <algorithm>
#include "GameServer.h"

void Room::Init(int id, GameServer* owner)
{
    roomId = static_cast<unsigned char>(id);
    server = owner;
    state = ROOM_WAITING;
    resultEndMS = 0;
    nextSnowballID = OBJ_ID_SNOWBALL_START;
    memset(wallHeight, 0, sizeof(wallHeight));
    nextItemMs = 0;

    for (int i = 0; i < MAX_ROOM_PLAYERS; ++i) {
        playerSessionIds[i] = INVALID_ID;
        selectedCharacters[i] = CHAR_NONE;
        players[i] = PlayerState{};
    }
    for (Snowball& snowball : snowballs)
        snowball = Snowball{};
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

bool Room::StartGame(ULONGLONG now)
{
    if (state != ROOM_WAITING || GetPlayerCount() != MAX_ROOM_PLAYERS)
        return false;

    for (int i = 0; i < MAX_ROOM_PLAYERS; ++i) {
        if (selectedCharacters[i] == CHAR_NONE)
            return false;
    }

    state = ROOM_PLAYING;
    ResetWorld();
    nextItemMs = now + ITEM_INTERVAL_MS;

    S2C_GameStartResult result{};
    result.size = sizeof(result);
    result.type = S2C_GAME_START_RESULT;
    result.success = true;
    for (int i = 0; i < MAX_ROOM_PLAYERS; ++i) {
        players[i] = PlayerState{};
        players[i].character = selectedCharacters[i];
        players[i].pos = SPAWN_POS[i];
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
        snowball = Snowball{};

    nextSnowballID = OBJ_ID_SNOWBALL_START;
    memset(wallHeight, 0, sizeof(wallHeight));
}

void Room::Broadcast(const void* packet) const
{
    for (int i = 0; i < MAX_ROOM_PLAYERS; ++i) {
        if (playerSessionIds[i] != INVALID_ID)
            server->SendPacket(playerSessionIds[i], packet);
    }
}

void Room::SendRoomInfo() const
{
    S2C_RoomInfo info{};
    info.size = sizeof(S2C_RoomInfo);
    info.type = S2C_ROOM_INFO;
    info.room_id = roomId;

    for (int i = 0; i < MAX_ROOM_PLAYERS; ++i) {
        RoomPlayerInfo& playerInfo = info.players[i];
        playerInfo = RoomPlayerInfo{};
        playerInfo.character = CHAR_NONE;

        const GameSession* session = (playerSessionIds[i] == INVALID_ID) ? nullptr : server->FindGameSession(playerSessionIds[i]);
        if (session == nullptr)
            continue;

        playerInfo.connected = true;
        memcpy(playerInfo.name, session->name, MAX_NAME_LEN);
        playerInfo.character = selectedCharacters[i];
    }

    Broadcast(&info);
}

void Room::SendStatusChange(int objectId, STATUS_TYPE type, float value) const
{
    S2C_StatusChange packet{};
    packet.size = sizeof(packet);
    packet.type = S2C_STATUS_CHANGE;
    packet.object_id = objectId;
    packet.status = type;
    packet.value = value;
    Broadcast(&packet);
}

void Room::SendMoveObject(int objectId, const glm::vec3& pos) const
{
    S2C_MoveObject packet{};
    packet.size = sizeof(packet);
    packet.type = S2C_MOVE_OBJECT;
    packet.object_id = objectId;
    packet.x = pos.x;
    packet.y = pos.y;
    packet.z = pos.z;
    Broadcast(&packet);
}

bool Room::WorldToGrid(float x, float z, int* gx, int* gz)
{
    if (x < 0.0f || z < 0.0f)
        return false;

    const int cellX = static_cast<int>(x / BLOCK_SIZE);
    const int cellZ = static_cast<int>(z / BLOCK_SIZE);
    if (cellX >= MAP_WIDTH || cellZ >= MAP_DEPTH)
        return false;

    *gx = cellX;
    *gz = cellZ;
    return true;
}

bool Room::IsBlockedByWall(const glm::vec3& pos, const glm::vec3& bbox) const
{
    const float halfX = bbox.x * 0.5f;
    const float halfZ = bbox.z * 0.5f;

    int minX, minZ, maxX, maxZ;
    const bool minOk = WorldToGrid(pos.x - halfX, pos.z - halfZ, &minX, &minZ);
    const bool maxOk = WorldToGrid(pos.x + halfX, pos.z + halfZ, &maxX, &maxZ);
    if (!minOk)
        minX = minZ = 0;
    if (!maxOk) {
        maxX = MAP_WIDTH - 1;
        maxZ = MAP_DEPTH - 1;
    }

    for (int gx = minX; gx <= maxX; ++gx) {
        for (int gz = minZ; gz <= maxZ; ++gz) {
            if (wallHeight[gx][gz] >= WALL_BLOCK_HEIGHT)
                return true;
        }
    }
    return false;
}

void Room::UpdateCharacters(ULONGLONG now, float dt)
{
    for (int slot = 0; slot < MAX_ROOM_PLAYERS; ++slot) {
        if (playerSessionIds[slot] == INVALID_ID)
            continue;

        PlayerState& player = players[slot];

        float forward = 0.0f;
        float right = 0.0f;
        if (player.moveKeys & KEY_FORWARD) forward += 1.0f;
        if (player.moveKeys & KEY_BACK)    forward -= 1.0f;
        if (player.moveKeys & KEY_RIGHT)   right += 1.0f;
        if (player.moveKeys & KEY_LEFT)    right -= 1.0f;

        float dx = -FORWARD_Z[slot] * right;
        float dz = FORWARD_Z[slot] * forward;
        const float length = std::sqrt(dx * dx + dz * dz);

        if (player.character != CHAR_NONE && length != 0.0f) {
            const CharacterStats& stats = CHARACTER_STATS[player.character];
            const float step = stats.moveSpeed * dt / length;
            dx *= step;
            dz *= step;

            const float halfX = stats.bbox.x * 0.5f;
            const float halfZ = stats.bbox.z * 0.5f;
            const glm::vec3 before = player.pos;

            glm::vec3 next = player.pos;
            next.x = std::clamp(player.pos.x + dx, halfX, MAP_WIDTH * BLOCK_SIZE - halfX);
            if (!IsBlockedByWall(next, stats.bbox))
                player.pos.x = next.x;

            next = player.pos;
            next.z = std::clamp(player.pos.z + dz, halfZ, MAP_DEPTH * BLOCK_SIZE - halfZ);
            if (!IsBlockedByWall(next, stats.bbox))
                player.pos.z = next.z;

            if (player.pos != before)
                SendMoveObject(slot, player.pos);
        }

        ARM_STATE nextArm = ARM_IDLE;
        if (now < player.throwEndMs)
            nextArm = ARM_THROW;
        else if (player.charging)
            nextArm = ARM_CHARGE;
        else if (player.moveKeys != 0)
            nextArm = ARM_RUN;

        if (nextArm != player.armState) {
            player.armState = nextArm;
            SendStatusChange(slot, STATUS_ARM_STATE, static_cast<float>(nextArm));
        }
    }
}
