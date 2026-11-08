#include "NetSession.h"

#include "../common/Protocol.h"

#include <cstring>

void NetSession::Init(SOCKET clientSocket, int newSessionId)
{
	inUse = true;
	sessionId = newSessionId;
	socket = clientSocket;
	recvSize = 0;
	sendSize = 0;
	sendOffset = 0;
	closeAfterSend = false;
	recvStalled = false;
}

NET_RESULT NetSession::ReceivePackets(RecvEventQueue* recvQueue, bool readSocket)
{
	if (!inUse || recvQueue == nullptr) {
		return NET_ERROR;
	}

	constexpr int headerSize = sizeof(PacketHeader);

	while(true) {
		int consumed = 0;
		NET_RESULT result = NET_OK;

		while (consumed < recvSize) {
			const int packetSize = recvBuffer[consumed];
			if (packetSize < headerSize) {
				result = NET_ERROR;
				break;
			}

			if (packetSize > recvSize - consumed) {
				break;
			}

			RecvEvent event{};
			event.kind = RECV_EVENT_PACKET;
			event.sessionId = sessionId;
			event.packetSize = packetSize;
			std::memcpy(event.packetData, recvBuffer + consumed, packetSize);

			if (!recvQueue->Enqueue(event)) {
				result = NET_WOULD_BLOCK;
				break;
			}

			consumed += packetSize;
		}

		recvSize -= consumed;
		if (consumed > 0 && recvSize > 0) {
			std::memmove(recvBuffer, recvBuffer + consumed, recvSize);
		}

		recvStalled = result == NET_WOULD_BLOCK;
		if (result != NET_OK || !readSocket) {
			return result;
		}

		const int received = recv(socket, reinterpret_cast<char*>(recvBuffer + recvSize),
			RECV_BUFFER_SIZE - recvSize, 0);
		if (received == 0) {
			return NET_CLOSED;
		}
		if (received == SOCKET_ERROR) {
			return WSAGetLastError() == WSAEWOULDBLOCK ? NET_WOULD_BLOCK : NET_ERROR;
		}

		recvSize += received;
		readSocket = false;
	}
}
