#include "NetworkServer.h"

#include <cstdio>

namespace {
	constexpr int POLL_TIMEOUT_MS = 5;
}

bool NetworkServer::Init(unsigned short port, RecvEventQueue* recvQueue, SendRequestQueue* sendQueue)
{
	if (listenSocket != INVALID_SOCKET || recvQueue == nullptr || sendQueue == nullptr) {
		WSASetLastError(WSAEINVAL);
		return false;
	}

	const SOCKET newListenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (newListenSocket == INVALID_SOCKET) {
		return false;
	}

	sockaddr_in address{};
	address.sin_family = AF_INET;
	address.sin_addr.s_addr = htonl(INADDR_ANY);
	address.sin_port = htons(port);

	const int reuseAddress = 1;
	if (setsockopt(newListenSocket, SOL_SOCKET, SO_REUSEADDR,
			reinterpret_cast<const char*>(&reuseAddress), sizeof(reuseAddress)) == SOCKET_ERROR
		|| bind(newListenSocket, reinterpret_cast<const sockaddr*>(&address), sizeof(address)) == SOCKET_ERROR
		|| listen(newListenSocket, MAX_CONNECTIONS) == SOCKET_ERROR
		|| !NetUtil::ConfigureSocket(newListenSocket, false)) {
		const int error = WSAGetLastError();
		closesocket(newListenSocket);
		WSASetLastError(error);
		return false;
	}

	listenSocket = newListenSocket;
	this->recvQueue = recvQueue;
	this->sendQueue = sendQueue;
	nextSessionId = 1;
	running.store(true);
	return true;
}

void NetworkServer::RunLoop()
{
	while (running.load()) {
		WSAPOLLFD pollSockets[MAX_CONNECTIONS + 1]{};
		pollSockets[0] = { listenSocket, POLLRDNORM, 0 };
		ULONG socketCount = 1;

		for (const NetSession& session : sessions) {
			if (session.inUse) {
				pollSockets[socketCount++] = { session.socket, session.GetPollEvents(), 0 };
			}
		}

		const int readyCount = WSAPoll(pollSockets, socketCount, POLL_TIMEOUT_MS);
		if (readyCount == SOCKET_ERROR) {
			std::fprintf(stderr, "WSAPoll failed: %d\n", WSAGetLastError());
			Stop();
			break;
		}

		if (readyCount > 0 && running.load()) {
			// 11/15 이벤트 분기 연결 전까지 같은 이벤트를 반복 감시할 때 CPU 사용을 제한.
			Sleep(POLL_TIMEOUT_MS);
		}
	}
}

void NetworkServer::Stop()
{
	running.store(false);
}

void NetworkServer::DisconnectClient(NetSession* session)
{
	if (session == nullptr || !session->inUse) {
		return;
	}

	RecvEvent event{};
	event.kind = RECV_EVENT_DISCONNECTED;
	event.sessionId = session->sessionId;
	if (!recvQueue->Enqueue(event, true)) {
		std::fprintf(stderr, "Failed to queue disconnect for session %d.\n", event.sessionId);
	}

	closesocket(session->socket);
	*session = NetSession{};
}

void NetworkServer::Close()
{
	Stop();

	for (NetSession& session : sessions) {
		if (session.socket != INVALID_SOCKET) {
			closesocket(session.socket);
		}
		session = NetSession{};
	}

	if (listenSocket != INVALID_SOCKET) {
		closesocket(listenSocket);
		listenSocket = INVALID_SOCKET;
	}

	recvQueue = nullptr;
	sendQueue = nullptr;
}
