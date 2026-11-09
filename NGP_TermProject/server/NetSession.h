#pragma once

#include "ThreadQueue.h"

constexpr int RECV_BUFFER_SIZE = 1024;
constexpr int SEND_BUFFER_SIZE = 8192;

class NetSession {
public:
	bool			inUse = false;
	int				sessionId = INVALID_ID;
	SOCKET			socket = INVALID_SOCKET;
	unsigned char	recvBuffer[RECV_BUFFER_SIZE];
	int				recvSize = 0;
	unsigned char	sendBuffer[SEND_BUFFER_SIZE];
	int				sendSize = 0;
	int				sendOffset = 0;
	bool			closeAfterSend = false;
	bool			recvStalled = false;

	void Init(SOCKET clientSocket, int newSessionId);
	NET_RESULT ReceivePackets(RecvEventQueue* recvQueue, bool readSocket);
	bool QueueSendData(const unsigned char* data, int size);
	NET_RESULT FlushSendBuffer();
};