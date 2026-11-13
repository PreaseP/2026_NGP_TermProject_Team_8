#include "ServerApp.h"

#include <cstdlib>

bool ServerApp::Init(unsigned short port)
{
	if (stopEvent != nullptr) {
		return false;
	}

	WSADATA winsock{};
	if (WSAStartup(MAKEWORD(2, 2), &winsock) != 0) {
		return false;
	}

	stopEvent = CreateEventW(nullptr, TRUE, FALSE, nullptr);
	if (stopEvent == nullptr) {
		WSACleanup();
		return false;
	}

	recvQueue.Init();
	sendQueue.Init();

	if (!network.Init(port, &recvQueue, &sendQueue)
		|| !game.Init(&recvQueue, &sendQueue, stopEvent)) {
		network.Close();
		sendQueue.Destroy();
		recvQueue.Destroy();
		CloseHandle(stopEvent);
		stopEvent = nullptr;
		WSACleanup();
		return false;
	}

	return true;
}

int ServerApp::Run()
{
	if (stopEvent == nullptr || gameThreadHandle != nullptr) {
		return EXIT_FAILURE;
	}

	gameThreadHandle = CreateThread(nullptr, 0, GameServer::ThreadEntry, &game, 0, nullptr);
	if (gameThreadHandle == nullptr) {
		return EXIT_FAILURE;
	}

	network.RunLoop();
	return EXIT_SUCCESS;
}
