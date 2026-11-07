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
