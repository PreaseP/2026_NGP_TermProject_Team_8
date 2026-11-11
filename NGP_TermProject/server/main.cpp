#include "NetworkServer.h"

#include <winsock2.h>

#include <cstdio>
#include <cstdlib>
#include <memory>

int main()
{
	WSADATA winsock{};
	const int startupResult = WSAStartup(MAKEWORD(2, 2), &winsock);
	if (startupResult != 0) {
		std::fprintf(stderr, "WSAStartup failed: %d\n", startupResult);
		return EXIT_FAILURE;
	}

	auto recvQueue = std::make_unique<RecvEventQueue>();
	auto sendQueue = std::make_unique<SendRequestQueue>();
	recvQueue->Init();
	sendQueue->Init();

	NetworkServer network;
	const bool initialized = network.Init(SERVER_PORT, recvQueue.get(), sendQueue.get());
	if (initialized) {
		network.Close();
	}
	else {
		std::fprintf(stderr, "NetworkServer::Init failed: %d\n", WSAGetLastError());
	}

	sendQueue->Destroy();
	recvQueue->Destroy();

	if (WSACleanup() == SOCKET_ERROR) {
		std::fprintf(stderr, "WSACleanup failed: %d\n", WSAGetLastError());
		return EXIT_FAILURE;
	}

	if (!initialized) {
		return EXIT_FAILURE;
	}

	std::printf("Server startup check passed (Winsock 2.2, TCP port %u).\n",
		static_cast<unsigned int>(SERVER_PORT));
	return EXIT_SUCCESS;
}