#pragma once
#include "../common/Common.h"

class GameSession {
public:
    bool inUse = false;
    int  sessionId = INVALID_ID;
    bool loggedIn = false;
    char name[MAX_NAME_LEN] = {};
    int  roomId = INVALID_ID;
    int  slot = INVALID_ID;

    void Init(int sessionId);
    void Reset();
};
