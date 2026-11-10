#include "NetSession.h"

#include "../common/protocol.h"

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

bool NetSession::QueueSendData(const unsigned char* data, int size)
{
	if (!inUse || data == nullptr || size <= 0) {
		return false;
	}

	const int pendingSize = sendSize - sendOffset;
	if (size > SEND_BUFFER_SIZE - pendingSize) {
		return false;
	}

	if (size > SEND_BUFFER_SIZE - sendSize) {
		std::memmove(sendBuffer, sendBuffer + sendOffset, pendingSize);
		sendSize = pendingSize;
		sendOffset = 0;
	}

	std::memcpy(sendBuffer + sendSize, data, size);
	sendSize += size;
	return true;
}

NET_RESULT NetSession::FlushSendBuffer()
{
	if (!inUse) {
		return NET_ERROR;
	}

	const int pendingSize = sendSize - sendOffset;
	if (pendingSize > 0) {
		const int sent = send(socket, reinterpret_cast<const char*>(sendBuffer + sendOffset),
			pendingSize, 0);
		if (sent == SOCKET_ERROR) {
			return WSAGetLastError() == WSAEWOULDBLOCK ? NET_WOULD_BLOCK : NET_ERROR;
		}

		sendOffset += sent;
		if (sendOffset < sendSize) {
			return NET_WOULD_BLOCK;
		}
	}

	sendSize = 0;
	sendOffset = 0;
	return closeAfterSend ? NET_CLOSED : NET_OK;
}