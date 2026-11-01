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
