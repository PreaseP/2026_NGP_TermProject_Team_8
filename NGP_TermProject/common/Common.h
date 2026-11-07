#pragma once

#include <winsock2.h>
#include <vector>

// 상수(SERVER_PORT, MAX_*, BUF_SIZE, OBJ_ID_* 등)와 패킷 정의는 protocol.h에 있다.
#include "protocol.h"

constexpr int TICK_MS = 33;
constexpr unsigned char MOVE_KEY_MASK = KEY_FORWARD | KEY_BACK | KEY_LEFT | KEY_RIGHT;

enum NET_RESULT
{
	NET_OK,
	NET_WOULD_BLOCK,
	NET_CLOSED,
	NET_ERROR
};
using PacketBytes = std::vector<unsigned char>;

constexpr int INVALID_ID = -1;

class NetUtil {
public:
	static bool SetNonBlocking(SOCKET sock);
	static bool SetNoDelay(SOCKET sock);
	static bool ConfigureSocket(SOCKET sock, bool noDelay); // 넌블로킹과 TCP_NODELAY 설정
};

class Inventory {
public:
	ITEM_TYPE items[MAX_ITEMS] = {};
	int       itemCount = 0;
	bool Add(ITEM_TYPE item); // 아이템 지급
	bool Remove(ITEM_TYPE item); // 아이템 제거
	bool Has(ITEM_TYPE item) const; // item에 해당하는 아이템을 가지고 있는지 확인
	void Clear(); // 인벤토리 초기화
};
