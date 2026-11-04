#include "Common.h"

#pragma comment(lib, "ws2_32.lib")

bool NetUtil::SetNonBlocking(SOCKET sock) {
	u_long mode = 1;
	return ioctlsocket(sock, FIONBIO, &mode) == 0;
}

bool NetUtil::SetNoDelay(SOCKET sock)
{
	const int enabled = 1;
	return setsockopt(sock, IPPROTO_TCP, TCP_NODELAY,
		reinterpret_cast<const char*>(&enabled), sizeof(enabled)) == 0;
}