#include "NetworkServer.h"

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

void NetworkServer::Close()
{
	running.store(false);

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
