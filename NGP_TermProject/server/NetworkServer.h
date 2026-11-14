#pragma once

#include "NetSession.h"

#include <atomic>

class NetworkServer {
private:
	SOCKET listenSocket = INVALID_SOCKET;
	NetSession sessions[MAX_CONNECTIONS];
	int nextSessionId = 1;
	RecvEventQueue* recvQueue = nullptr;
	SendRequestQueue* sendQueue = nullptr;
	std::atomic<bool> running{false};

	void DisconnectClient(NetSession* session);

public:
	bool Init(unsigned short port, RecvEventQueue* recvQueue, SendRequestQueue* sendQueue);
	void RunLoop();
	void Stop();
	void Close();
};
