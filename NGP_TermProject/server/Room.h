#pragma once
#include "../common/Common.h"
#include "PlayerState.h"
#include "Snowball.h"
#include <windows.h>

constexpr int MAX_SNOWBALLS = 32;

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
    float          wallHeight = 0.0f;
    ULONGLONG      nextItemMs = 0;
    GameServer*    server = nullptr;

    void Init(int roomId, GameServer* server); // 빈 대기실 초기화
};
