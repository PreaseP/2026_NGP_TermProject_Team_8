#include "NetworkServer.h"

#include <climits>
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
		ProcessSendQueue();

		for (NetSession& session : sessions) {
			if (session.inUse && session.recvStalled) {
				const NET_RESULT result = session.ReceivePackets(recvQueue, false);
				if (result == NET_ERROR || result == NET_CLOSED) {
					DisconnectClient(&session);
				}
			}
		}

		WSAPOLLFD pollSockets[MAX_CONNECTIONS + 1]{};
		NetSession* polledSessions[MAX_CONNECTIONS + 1]{};
		pollSockets[0] = { listenSocket, POLLRDNORM, 0 };
		ULONG socketCount = 1;

		for (NetSession& session : sessions) {
			const short events = session.GetPollEvents();
			if (session.inUse && events != 0) {
				pollSockets[socketCount] = { session.socket, events, 0 };
				polledSessions[socketCount] = &session;
				++socketCount;
			}
		}

		const int readyCount = WSAPoll(pollSockets, socketCount, POLL_TIMEOUT_MS);
		if (readyCount == SOCKET_ERROR) {
			std::fprintf(stderr, "WSAPoll failed: %d\n", WSAGetLastError());
			Stop();
			break;
		}

		if (!running.load()) {
			break;
		}
		if (readyCount == 0) {
			continue;
		}

		const short listenEvents = pollSockets[0].revents;
		constexpr short socketErrors = POLLERR | POLLNVAL;
		if (listenEvents & (socketErrors | POLLHUP)) {
			std::fprintf(stderr, "Listening socket failed (poll flags: %d).\n", listenEvents);
			Stop();
			break;
		}

		if ((listenEvents & POLLRDNORM) && running.load()) {
			AcceptClients();
		}

		for (ULONG i = 1; i < socketCount && running.load(); ++i) {
			const short events = pollSockets[i].revents;
			NetSession* session = polledSessions[i];

			if (events & socketErrors) {
				DisconnectClient(session);
				continue;
			}

			if (events & POLLRDNORM) {
				const NET_RESULT result = session->ReceivePackets(recvQueue, true);
				if (result == NET_CLOSED || result == NET_ERROR) {
					DisconnectClient(session);
					continue;
				}
			}
			else if ((events & POLLHUP) && !session->recvStalled) {
				DisconnectClient(session);
				continue;
			}

			if (events & POLLWRNORM) {
				const NET_RESULT result = session->FlushSendBuffer();
				if (result == NET_CLOSED || result == NET_ERROR) {
					DisconnectClient(session);
				}
			}
		}
	}
}

void NetworkServer::ProcessSendQueue()
{
	SendRequest request{};
	for (int i = 0; i < SEND_QUEUE_CAPACITY && running.load(); ++i) {
		if (!sendQueue->TryDequeue(&request)) {
			break;
		}

		NetSession* target = nullptr;
		for (NetSession& session : sessions) {
			if (session.inUse && session.sessionId == request.targetSessionId) {
				target = &session;
				break;
			}
		}
		if (target == nullptr || target->closeAfterSend) {
			continue;
		}

		if (request.packetSize < 0 || request.packetSize > BUF_SIZE) {
			std::fprintf(stderr, "Invalid send request size for session %d.\n", request.targetSessionId);
			continue;
		}
		if (request.packetSize > 0 && !target->QueueSendData(request.packetData, request.packetSize)) {
			DisconnectClient(target);
			continue;
		}

		if (request.closeAfterSend) {
			target->closeAfterSend = true;
			if (target->sendOffset == target->sendSize) {
				DisconnectClient(target);
			}
		}
	}
}

void NetworkServer::AcceptClients()
{
	while (running.load()) {
		const SOCKET clientSocket = accept(listenSocket, nullptr, nullptr);
		if (clientSocket == INVALID_SOCKET) {
			const int error = WSAGetLastError();
			if (error != WSAEWOULDBLOCK) {
				std::fprintf(stderr, "accept failed: %d\n", error);
			}
			return;
		}

		NetSession* freeSlot = nullptr;
		for (NetSession& session : sessions) {
			if (!session.inUse) {
				freeSlot = &session;
				break;
			}
		}

		if (freeSlot == nullptr || nextSessionId == INT_MAX) {
			closesocket(clientSocket);
			continue;
		}

		const int keepAlive = 1;
		if (!NetUtil::ConfigureSocket(clientSocket, true)
			|| setsockopt(clientSocket, SOL_SOCKET, SO_KEEPALIVE,
				reinterpret_cast<const char*>(&keepAlive), sizeof(keepAlive)) == SOCKET_ERROR) {
			std::fprintf(stderr, "Client socket setup failed: %d\n", WSAGetLastError());
			closesocket(clientSocket);
			continue;
		}

		freeSlot->Init(clientSocket, nextSessionId);

		RecvEvent event{};
		event.kind = RECV_EVENT_CONNECTED;
		event.sessionId = nextSessionId;
		if (!recvQueue->Enqueue(event, true)) {
			std::fprintf(stderr, "Failed to queue connection for session %d.\n", event.sessionId);
			closesocket(clientSocket);
			*freeSlot = NetSession{};
			return;
		}

		++nextSessionId;
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
