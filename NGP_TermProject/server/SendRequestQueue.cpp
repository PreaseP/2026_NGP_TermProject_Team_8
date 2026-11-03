#include "SendRequestQueue.h"

void SendRequestQueue::Init()
{
    head = 0;
    tail = 0;
    count = 0;
    InitializeCriticalSection(&lock);
}

void SendRequestQueue::Destroy()
{
    DeleteCriticalSection(&lock);
}

bool SendRequestQueue::Enqueue(const SendRequest& request)
{
    EnterCriticalSection(&lock);

    if (count >= SEND_QUEUE_CAPACITY) {
        LeaveCriticalSection(&lock);
        return false;
    }

    items[tail] = request;
    tail = (tail + 1) % SEND_QUEUE_CAPACITY;
    ++count;

    LeaveCriticalSection(&lock);
    return true;
}

bool SendRequestQueue::TryDequeue(SendRequest* out)
{
    EnterCriticalSection(&lock);

    if (count == 0) {
        LeaveCriticalSection(&lock);
        return false;
    }

    *out = items[head];
    head = (head + 1) % SEND_QUEUE_CAPACITY;
    --count;

    LeaveCriticalSection(&lock);
    return true;
}
