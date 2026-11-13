#pragma once

#include "GameServer.h"
#include "NetworkServer.h"

class ServerApp {
private:
	NetworkServer network;
	GameServer game;
	RecvEventQueue recvQueue;
	SendRequestQueue sendQueue;
	HANDLE stopEvent = nullptr;
	HANDLE gameThreadHandle = nullptr;

public:
	bool Init(unsigned short port);
	int Run();
};
