#pragma once

#include <winsock2.h>

constexpr int MAX_ITEMS = 2;
constexpr int MAX_NAME_LEN = 16;
constexpr int BUF_SIZE = 256;
constexpr int INVALID_ID = -1;

enum NET_RESULT {
	NET_OK,
	NET_WOULD_BLOCK,
	NET_CLOSED,
	NET_ERROR
};

class NetUtil {
public:
	static bool ConfigureSocket(SOCKET sock, bool noDelay);
};

enum ITEM_TYPE : unsigned char
{
    ITEM_NONE,
    ITEM_SHIELD,
    ITEM_POWER_UP
};
