#pragma once

#include "../common/Common.h"

#include <windows.h>

constexpr int MAX_CONNECTIONS = 16;
constexpr int RECV_QUEUE_CAPACITY = 1024;
constexpr int SEND_QUEUE_CAPACITY = 1024;
constexpr int EVENT_RESERVE = MAX_CONNECTIONS * 2;

enum RECV_EVENT_TYPE {
	RECV_EVENT_CONNECTED,
	RECV_EVENT_PACKET,
	RECV_EVENT_DISCONNECTED
};

struct RecvEvent {
	RECV_EVENT_TYPE kind;
	int				sessionId;
	int				packetSize;
	unsigned char	packetData[BUF_SIZE];
};

struct SendRequest {
	int				targetSessionId;
	int				packetSize;
	unsigned char	packetData[BUF_SIZE];
	bool			closeAfterSend;
};

template<typename T, int Capacity, int Reserve = 0>
class ThreadQueue {
	static_assert(Capacity > 0 && Reserve >= 0 && Reserve < Capacity,
		"Invalid thread queue capacity or reserve");

private:
	T					items[Capacity];
	int					head;
	int					tail;
	int					count;
	CRITICAL_SECTION	lock;

public:
	ThreadQueue() = default;
	ThreadQueue(const ThreadQueue&) = delete;
	ThreadQueue& operator=(const ThreadQueue&) = delete;

	void Init()
	{
		head = 0;
		tail = 0;
		count = 0;
		InitializeCriticalSection(&lock);
	}

	void Destroy()
	{
		DeleteCriticalSection(&lock);
	}

	bool Enqueue(const T& item, bool useReserve = false)
	{
		const int limit = useReserve ? Capacity : Capacity - Reserve;
		EnterCriticalSection(&lock);

		if (count >= limit) {
			LeaveCriticalSection(&lock);
			return false;
		}

		items[tail] = item;
		tail = (tail + 1) % Capacity;
		++count;

		LeaveCriticalSection(&lock);
		return true;
	}

	bool TryDequeue(T* out)
	{
		EnterCriticalSection(&lock);

		if (out == nullptr || count == 0) {
			LeaveCriticalSection(&lock);
			return false;
		}

		*out = items[head];
		head = (head + 1) % Capacity;
		--count;

		LeaveCriticalSection(&lock);
		return true;
	}
};

using RecvEventQueue = ThreadQueue<RecvEvent, RECV_QUEUE_CAPACITY, EVENT_RESERVE>;
using SendRequestQueue = ThreadQueue<SendRequest, SEND_QUEUE_CAPACITY>;