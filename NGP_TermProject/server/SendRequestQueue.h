#pragma once
#include <winsock2.h>
#include <windows.h>
#include "../common/Common.h"

constexpr int SEND_QUEUE_CAPACITY = 1024;

struct SendRequest {
    int           targetSessionId;
    int           packetSize;
    unsigned char packetData[BUF_SIZE];
    bool          closeAfterSend;
};

class SendRequestQueue {
public:
    SendRequest      items[SEND_QUEUE_CAPACITY];
    int              head = 0;
    int              tail = 0;
    int              count = 0;
    CRITICAL_SECTION lock;

    void Init();
    void Destroy();
    bool Enqueue(const SendRequest& request);
    bool TryDequeue(SendRequest* out);
};
