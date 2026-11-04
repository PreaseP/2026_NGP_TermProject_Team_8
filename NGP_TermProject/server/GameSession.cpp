#include "GameSession.h"
#include <cstring>

void GameSession::Init(int id)
{
    Reset();
    inUse = true;
    sessionId = id;
}

void GameSession::Reset()
{
    inUse = false;
    sessionId = INVALID_ID;
    loggedIn = false;
    memset(name, 0, sizeof(name));
    roomId = INVALID_ID;
    slot = INVALID_ID;
}
