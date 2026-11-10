#include <winsock2.h>

#include <cstdio>
#include <cstdlib>

int main()
{
	WSADATA winsock{};
	const int startupResult = WSAStartup(MAKEWORD(2, 2), &winsock);
	if (startupResult != 0) {
		std::fprintf(stderr, "WSAStartup failed: %d\n", startupResult);
		return EXIT_FAILURE;
	}

	if (WSACleanup() == SOCKET_ERROR) {
		std::fprintf(stderr, "WSACleanup failed: %d\n", WSAGetLastError());
		return EXIT_FAILURE;
	}

	std::puts("Server startup check passed (Winsock 2.2).");
	return EXIT_SUCCESS;
}