// @Author
#pragma once
#include "eventloop.h"
#include "base/Condition.h"
#include "base/Mutex.h"
#include "base/Thread.h"
#include "base/noncopyable.h"

class EventLoopThread : noncopyable
{
    public:
        EventLoopThread();
        ~EventLoopThread();
        EventLoop* startLoop();

    private:
        void threadFunc();
        EventLoop* loop_;
        bool exiting_;
        Thread thread_;
        MutexLock mutex_;
        Condition cond_;
};
