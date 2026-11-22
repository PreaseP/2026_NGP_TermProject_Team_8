#include "Room.h"
#include <cmath>
#include <algorithm>
#include <random>
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

void Room::EndGame(int winnerSlot, END_REASON reason, ULONGLONG now)
{
    if (state != ROOM_PLAYING)
        return;

    state = ROOM_RESULT;
    resultEndMS = now + RESULT_DURATION_MS;

    S2C_GameOver result{};
    result.size = sizeof(result);
    result.type = S2C_GAME_OVER;
    result.winner_slot = static_cast<unsigned char>(winnerSlot);
    result.reason = reason;
    Broadcast(&result);
    server->SendRoomList(INVALID_ID);
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
            next.x = std::clamp(player.pos.x + dx, PLAY_MIN_X + halfX, PLAY_MAX_X - halfX);
            if (!IsBlockedByWall(next, stats.bbox))
                player.pos.x = next.x;

            next = player.pos;
            next.z = std::clamp(player.pos.z + dz, PLAY_MIN_Z[slot] + halfZ, PLAY_MAX_Z[slot] - halfZ);
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

void Room::StartCharge(int slot, ULONGLONG now)
{
    if (state != ROOM_PLAYING || slot < 0 || slot >= MAX_ROOM_PLAYERS)
        return;

    PlayerState& player = players[slot];
    if (player.charging || now < player.throwEndMs)
        return;

    player.charging = true;
    player.chargeStartMs = now;
}

int Room::AllocSnowballId()
{
    const int id = nextSnowballID;
    ++nextSnowballID;
    if (nextSnowballID >= OBJ_ID_SNOW_WALL_START)
        nextSnowballID = OBJ_ID_SNOWBALL_START;
    return id;
}

void Room::ThrowSnowball(int slot, ULONGLONG now)
{
    if (state != ROOM_PLAYING || slot < 0 || slot >= MAX_ROOM_PLAYERS)
        return;

    PlayerState& player = players[slot];
    if (!player.charging || player.character == CHAR_NONE)
        return;

    player.charging = false;

    Snowball* snowball = nullptr;
    for (Snowball& candidate : snowballs) {
        if (!candidate.active) {
            snowball = &candidate;
            break;
        }
    }
    if (snowball == nullptr)
        return;

    const ULONGLONG chargeMs = std::min<ULONGLONG>(now - player.chargeStartMs, MAX_CHARGE_MS);
    const float charge = static_cast<float>(chargeMs) / MAX_CHARGE_MS;
    const float speed = (MIN_THROW_SPEED + (MAX_THROW_SPEED - MIN_THROW_SPEED) * charge)
        * CHARACTER_STATS[player.character].throwPower;

    const glm::vec3 direction = glm::normalize(glm::vec3(0.0f, THROW_DIR_UP, FORWARD_Z[slot]));

    snowball->active = true;
    snowball->objectId = AllocSnowballId();
    snowball->ownerSlot = static_cast<unsigned char>(slot);
    snowball->powered = player.powerUp;
    snowball->pos = player.pos + glm::vec3(0.0f, THROW_START_UP, FORWARD_Z[slot] * THROW_START_FORWARD);
    snowball->vel = direction * speed;
    snowball->lifeTime = 0.0f;

    player.throwEndMs = now + THROW_ANIM_MS;
    if (player.powerUp) {
        player.powerUp = false;
        SendStatusChange(slot, STATUS_POWER_UP, 0.0f);
    }

    S2C_AddObject packet{};
    packet.size = sizeof(packet);
    packet.type = S2C_ADD_OBJECT;
    packet.object_id = snowball->objectId;
    packet.obj_type = OBJ_SNOWBALL;
    packet.owner_slot = snowball->ownerSlot;
    packet.x = snowball->pos.x;
    packet.y = snowball->pos.y;
    packet.z = snowball->pos.z;
    packet.height = 0.0f;
    packet.powered = snowball->powered;
    Broadcast(&packet);
}

bool Room::CheckSnowballHit(const Snowball& snowball, const PlayerState& player)
{
    if (player.character == CHAR_NONE)
        return false;

    const glm::vec3& bbox = CHARACTER_STATS[player.character].bbox;
    const glm::vec3 boxMin(player.pos.x - bbox.x * 0.5f, player.pos.y, player.pos.z - bbox.z * 0.5f);
    const glm::vec3 boxMax(player.pos.x + bbox.x * 0.5f, player.pos.y + bbox.y, player.pos.z + bbox.z * 0.5f);

    const glm::vec3 closest = glm::clamp(snowball.pos, boxMin, boxMax);
    const glm::vec3 diff = snowball.pos - closest;
    return glm::dot(diff, diff) <= SNOWBALL_RADIUS * SNOWBALL_RADIUS;
}

void Room::ApplyHit(int target, bool powered, ULONGLONG now)
{
    PlayerState& player = players[target];

    S2C_Hit packet{};
    packet.size = sizeof(packet);
    packet.type = S2C_HIT;
    packet.target_slot = static_cast<unsigned char>(target);

    if (player.shield) {
        player.shield = false;
        SendStatusChange(target, STATUS_SHIELD, 0.0f);
        packet.blocked = true;
    }
    else {
        const int damage = powered ? SNOWBALL_DAMAGE * 2 : SNOWBALL_DAMAGE;
        player.hp = std::max(0, player.hp - damage);
        packet.blocked = false;
    }

    packet.hp = player.hp;
    Broadcast(&packet);

    if (player.hp <= 0)
        EndGame(1 - target, END_HP_ZERO, now);
}

void Room::UpdateSnowballs(ULONGLONG now, float dt)
{
    for (Snowball& snowball : snowballs) {
        if (!snowball.active)
            continue;

        snowball.lifeTime += dt;
        snowball.vel.y += GRAVITY * dt;
        snowball.pos += snowball.vel * dt;

        if (snowball.lifeTime >= SNOWBALL_LIFETIME) {
            RemoveSnowball(snowball);
            continue;
        }

        bool hit = false;
        for (int slot = 0; slot < MAX_ROOM_PLAYERS; ++slot) {
            if (slot == snowball.ownerSlot || playerSessionIds[slot] == INVALID_ID)
                continue;
            if (!CheckSnowballHit(snowball, players[slot]))
                continue;

            const bool powered = snowball.powered;
            RemoveSnowball(snowball);
            ApplyHit(slot, powered, now);
            hit = true;
            break;
        }
        if (hit) {
            if (state != ROOM_PLAYING)
                return;
            continue;
        }

        if (snowball.pos.x < PLAY_MIN_X || snowball.pos.x > PLAY_MAX_X
            || snowball.pos.z < PLAY_MIN_Z[0] || snowball.pos.z > PLAY_MAX_Z[1]) {
            RemoveSnowball(snowball);
            continue;
        }

        int gx, gz;
        if (!WorldToGrid((std::max)(snowball.pos.x, 0.0f), (std::max)(snowball.pos.z, 0.0f), &gx, &gz)) {
            RemoveSnowball(snowball);
            continue;
        }

        const bool hitWall = wallHeight[gx][gz] > 0.0f && snowball.pos.y <= GROUND_TOP + wallHeight[gx][gz];
        const bool onGround = (snowball.pos.z >= PLAY_MIN_Z[0] && snowball.pos.z <= PLAY_MAX_Z[0])
            || (snowball.pos.z >= PLAY_MIN_Z[1] && snowball.pos.z <= PLAY_MAX_Z[1]);
        if (!onGround && snowball.pos.y < 0.0f) {
            RemoveSnowball(snowball);
            continue;
        }

        const bool hitGround = onGround && snowball.pos.y <= GROUND_TOP;
        if (hitWall || hitGround) {
            RemoveSnowball(snowball);
            AddSnowWall(gx, gz);
            continue;
        }

        SendMoveObject(snowball.objectId, snowball.pos);
    }
}

void Room::ReturnToWaiting()
{
    state = ROOM_WAITING;
    resultEndMS = 0;
    nextItemMs = 0;
    ResetWorld();

    for (int i = 0; i < MAX_ROOM_PLAYERS; ++i) {
        players[i] = PlayerState{};
        selectedCharacters[i] = CHAR_NONE;
    }

    SendRoomInfo();
    server->SendRoomList(INVALID_ID);
}

void Room::Update(ULONGLONG now, float dt)
{
    switch (state) {
    case ROOM_PLAYING:
        UpdateCharacters(now, dt);
        UpdateSnowballs(now, dt);
        break;
    case ROOM_RESULT:
        if (now >= resultEndMS)
            ReturnToWaiting();
        break;
    default:
        break;
    }
}

void Room::GiveRandomItem(int slot)
{
    if (state != ROOM_PLAYING || slot < 0 || slot >= MAX_ROOM_PLAYERS)
        return;
    if (playerSessionIds[slot] == INVALID_ID)
        return;

    PlayerState& player = players[slot];
    if (player.itemCount >= MAX_ITEMS)
        return;

    static std::mt19937 randomEngine(std::random_device{}());
    std::uniform_int_distribution<int> itemDistribution(0, 1);
    const ITEM_TYPE item = (itemDistribution(randomEngine) == 0) ? ITEM_SHIELD : ITEM_POWER_UP;
    player.items[player.itemCount++] = item;

    S2C_ItemGet packet{};
    packet.size = sizeof(packet);
    packet.type = S2C_ITEM_GET;
    packet.item = item;
    server->SendPacket(playerSessionIds[slot], &packet);
}

void Room::RemoveSnowball(Snowball& snowball)
{
    if (!snowball.active)
        return;

    snowball.active = false;

    S2C_RemoveObject packet{};
    packet.size = sizeof(packet);
    packet.type = S2C_REMOVE_OBJECT;
    packet.object_id = snowball.objectId;
    Broadcast(&packet);
}

void Room::AddSnowWall(int gx, int gz)
{
    if (gx < 0 || gx >= MAP_WIDTH || gz < 0 || gz >= MAP_DEPTH)
        return;

    float& height = wallHeight[gx][gz];
    if (height >= WALL_MAX_HEIGHT)
        return;

    const bool isNewWall = height == 0.0f;
    height = (std::min)(height + WALL_STEP, WALL_MAX_HEIGHT);
    const int objectId = OBJ_ID_SNOW_WALL_START + gx * 100 + gz;

    if (!isNewWall) {
        SendStatusChange(objectId, STATUS_WALL_HEIGHT, height);
        return;
    }

    S2C_AddObject packet{};
    packet.size = sizeof(packet);
    packet.type = S2C_ADD_OBJECT;
    packet.object_id = objectId;
    packet.obj_type = OBJ_SNOW_WALL;
    packet.owner_slot = 0xFF;
    // [11.22] 클라이언트가 눈 벽을 그리는 좌표에 맞춘다.
    packet.x = gx * BLOCK_SIZE;
    packet.y = GROUND_TOP + height / 2.0f;
    packet.z = gz * BLOCK_SIZE;
    packet.height = height;
    Broadcast(&packet);
}
