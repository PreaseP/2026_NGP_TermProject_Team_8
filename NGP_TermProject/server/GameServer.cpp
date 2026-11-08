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
