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
	return setsockopt(sock, IPPROTO_TCP, TCP_NODELAY,
		reinterpret_cast<const char*>(&enabled), sizeof(enabled)) == 0;
}

bool Inventory::Add(ITEM_TYPE item)
{
	if (itemCount >= MAX_ITEMS)
		return false;

	items[itemCount++] = item;
	return true;
}

bool Inventory::Remove(ITEM_TYPE item)
{
	for (int i = 0; i < itemCount; ++i) {
		if (items[i] == item) {
			for (int j = i; j < itemCount - 1; ++j)
				items[j] = items[j + 1];
			--itemCount;
			return true;
		}
	}
	return false;
}

bool Inventory::Has(ITEM_TYPE item) const
{
	for (int i = 0; i < itemCount; ++i) {
		if (items[i] == item)
			return true;
	}
	return false;
}

void Inventory::Clear()
{
	itemCount = 0;
}
