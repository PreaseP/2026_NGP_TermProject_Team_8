#include "ServerApp.h"

#include <cstdio>
#include <cstdlib>

namespace {
	SRWLOCK consoleHandlerLock = SRWLOCK_INIT;
	ServerApp* activeApp = nullptr;
}

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

	// [11.21] 콘솔에서 종료 요청을 받으면 네트워크 루프를 멈춘다.
	AcquireSRWLockExclusive(&consoleHandlerLock);
	const bool registered = activeApp == nullptr
		&& SetConsoleCtrlHandler(ConsoleHandler, TRUE);
	if (registered) {
		activeApp = this;
	}
	ReleaseSRWLockExclusive(&consoleHandlerLock);
	if (!registered) {
		return EXIT_FAILURE;
	}

	std::puts("Server started. Press Ctrl+C to stop.");
	network.RunLoop();
	return EXIT_SUCCESS;
}

BOOL WINAPI ServerApp::ConsoleHandler(DWORD ctrlType)
{
	if (ctrlType != CTRL_C_EVENT && ctrlType != CTRL_BREAK_EVENT) {
		return FALSE;
	}

	AcquireSRWLockShared(&consoleHandlerLock);
	const bool handled = activeApp != nullptr;
	if (handled) {
		activeApp->network.Stop();
	}
	ReleaseSRWLockShared(&consoleHandlerLock);
	return handled ? TRUE : FALSE;
}

void ServerApp::Shutdown()
{
	if (stopEvent == nullptr) {
		return;
	}

	// [11.22] 게임 스레드가 큐 사용을 끝낸 뒤 서버 자원을 닫는다.
	network.Stop();
	SetEvent(stopEvent);
	if (gameThreadHandle != nullptr) {
		WaitForSingleObject(gameThreadHandle, INFINITE);
		CloseHandle(gameThreadHandle);
		gameThreadHandle = nullptr;
	}

	network.Close();
	sendQueue.Destroy();
	recvQueue.Destroy();
	CloseHandle(stopEvent);
	stopEvent = nullptr;
	WSACleanup();

	AcquireSRWLockExclusive(&consoleHandlerLock);
	const bool registered = activeApp == this;
	if (registered) {
		activeApp = nullptr;
	}
	ReleaseSRWLockExclusive(&consoleHandlerLock);
	if (registered) {
		SetConsoleCtrlHandler(ConsoleHandler, FALSE);
	}
}
