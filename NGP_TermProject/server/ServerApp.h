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

	static BOOL WINAPI ConsoleHandler(DWORD ctrlType);

public:
	bool Init(unsigned short port);
	int Run();
	void Shutdown();
};
