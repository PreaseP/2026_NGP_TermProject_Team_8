#include "Common.h"

#pragma comment(lib, "ws2_32.lib")

bool NetUtil::ConfigureSocket(SOCKET sock, bool noDelay)
{
	u_long nonBlocking = 1;
	if (ioctlsocket(sock, FIONBIO, &nonBlocking) != 0) {
		return false;
	}

	if (!noDelay) {
		return true;
	}

	const int enabled = 1;
	return setsockopt(sock, IPPROTO_TCP,TCP_NODELAY,
		reinterpret_cast<const char*>(&enabled), sizeof(enabled)) == 0;
}