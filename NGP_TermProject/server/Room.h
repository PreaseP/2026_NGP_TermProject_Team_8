#pragma once
#include "../common/Common.h"
#include "PlayerState.h"
#include "Snowball.h"
#include <windows.h>

constexpr int   RESULT_DURATION_MS = 5000;
constexpr int   MAP_WIDTH = 10, MAP_DEPTH = 15;
constexpr float BLOCK_SIZE = 1.0f;
constexpr float GROUND_TOP = 0.5f;
constexpr float WALL_STEP = 0.5f;
constexpr float WALL_MAX_HEIGHT = 3.0f;
constexpr float WALL_BLOCK_HEIGHT = 1.0f;
constexpr float BACK_WALL_TOP = 5.5f;
constexpr float THIN_SNOW_SPEED_RATE = 0.5f;
constexpr float GRAVITY = -9.8f;
constexpr float SNOWBALL_RADIUS = 0.15f;
constexpr float MIN_THROW_SPEED = 2.0f;
constexpr float MAX_THROW_SPEED = 15.0f;
constexpr int   MAX_CHARGE_MS = 1000;
constexpr float SNOWBALL_LIFETIME = 5.0f;
constexpr int   MAX_SNOWBALLS = 32;
constexpr int   SNOWBALL_DAMAGE = 20;
constexpr int   THROW_ANIM_MS = 300;
constexpr float THROW_START_FORWARD = 1.0f;
constexpr float THROW_START_UP = 0.8f;
constexpr float THROW_DIR_UP = 0.3f;

struct CharacterStats { float moveSpeed; float throwPower; glm::vec3 bbox; };
const CharacterStats CHARACTER_STATS[2] = {
    { 5.3f,  1.3f, {0.6f, 1.8f, 0.6f} },
    { 6.25f, 1.0f, {0.6f, 1.8f, 0.6f} },
};
const glm::vec3 SPAWN_POS[MAX_ROOM_PLAYERS] = {
    {5.0f, GROUND_TOP, 2.0f}, {5.0f, GROUND_TOP, 13.0f}
};
const float FORWARD_Z[MAX_ROOM_PLAYERS] = { +1.0f, -1.0f };
constexpr float PLAY_MIN_X = -0.5f;
constexpr float PLAY_MAX_X = 9.5f;
constexpr float PLAY_MIN_Z[MAX_ROOM_PLAYERS] = { -0.5f, 9.5f };
constexpr float PLAY_MAX_Z[MAX_ROOM_PLAYERS] = { 4.5f, 14.5f };
constexpr float MAX_DT = 0.1f;

class GameServer;

class Room {
public:
    unsigned char  roomId = 0;
    ROOM_STATE     state = ROOM_WAITING;
    int            playerSessionIds[MAX_ROOM_PLAYERS] = { INVALID_ID, INVALID_ID };
    CHARACTER_TYPE selectedCharacters[MAX_ROOM_PLAYERS] = { CHAR_NONE, CHAR_NONE };
    ULONGLONG      resultEndMS = 0;
    PlayerState    players[MAX_ROOM_PLAYERS];
    Snowball       snowballs[MAX_SNOWBALLS];
    int            nextSnowballID = OBJ_ID_SNOWBALL_START;
    float          wallHeight[MAP_WIDTH][MAP_DEPTH] = {};
    ULONGLONG      nextItemMs = 0;
    GameServer*    server = nullptr;

    void Init(int roomId, GameServer* server); // 빈 대기실 초기화
    int GetPlayerCount() const; // 방 인원수 확인
    void SendRoomInfo() const; // 방 정보 전송
    bool StartGame(ULONGLONG now); // 시작 조건 확인 후 경기 시작
    void EndGame(int winnerSlot, END_REASON reason, ULONGLONG now);
    void ResetWorld(); // 눈덩이와 눈 벽 초기화
    void Broadcast(const void* packet) const; // 방 참가자 전원에게 패킷 전송
    void SendStatusChange(int objectId, STATUS_TYPE type, float value) const; // 오브젝트 상태 변경 전송
    void SendMoveObject(int objectId, const glm::vec3& pos) const; // 오브젝트 이동 좌표 전송
    static bool WorldToGrid(float x, float z, int* gx, int* gz); // 월드 좌표를 맵 칸으로 변환
    bool IsBlockedByWall(const glm::vec3& pos, const glm::vec3& bbox) const; // 눈 벽에 막히는지 확인
    void Update(ULONGLONG now, float dt); // 경기 갱신 및 대기실 복귀
    void ReturnToWaiting();
    void UpdateCharacters(ULONGLONG now, float dt); // 캐릭터 위치와 팔 자세 갱신
    void StartCharge(int slot, ULONGLONG now); // 눈덩이 차징 시작
    void ThrowSnowball(int slot, ULONGLONG now); // 차징에 따라 눈덩이 던지기
    int AllocSnowballId();
    void UpdateSnowballs(ULONGLONG now, float dt); // 눈덩이 이동·충돌·만료 처리
    static bool CheckSnowballHit(const Snowball& snowball, const PlayerState& player); // 눈덩이 피격 판정
    void ApplyHit(int target, bool powered, ULONGLONG now); // 방어막과 피해 처리
    void RemoveSnowball(Snowball& snowball);
    void AddSnowWall(int gx, int gz);
    void GiveRandomItem(int slot);
    void UseItem(int slot, ITEM_TYPE item);
};
