//@Author
#pragma once
#include <vector>
#include "EventLoopThread.h"
#include "base/noncopyable.h"

class EventLoopThreadPool : noncopyable
{
    public:
        EventLoopThreadPool(EventLoop* baseloop, int numThreads);

        ~EventLoopThreadPool() = default;
        void start();

        EventLoop* getNextLoop();

        bool started() const { return started_; }

    private:
        EventLoop* baseLoop_;
        bool started_;
        int numThreads_;
        int next_;
        std::vector<std::shared_ptr<EventLoopThread>> threads_;
        std::vector<EventLoop*> loops_;
};
