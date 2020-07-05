// @Author Yuz
#include "EventLoopThreadPool.h"
#include <assert.h>

EventLoopThreadPool::EventLoopThreadPool(EventLoop* baseLoop, int numThreads):
    baseLoop_(baseLoop),
    started_(false),
    numThreads_(numThreads),
    next_(0)
{
    assert(numThreads_ >= 0);
}
void EventLoopThreadPool::start()
{
    assert(!started_);
    baseLoop_->assertInLoopThread();
    started_ = true;
    for( int i = 0; i < numThreads_; ++i )
    {
        std::unique_ptr<EventLoopThread> t(new EventLoopThread());
        threds_.push_back(t);
        loops_.push_back(t->startLoop());
    }
}

EventLoop *EventLoopThreadPool::getNextLoop()
{
    baseLoop_->assertInLoopThread();
    assert(started_);
    EventLoop *loop = baseloop_;
    if(!loops_empty())
    {
        loop = loops_[next_];
        next_ = (next_ + 1) % numThreads_;
    }
    return loop;
}
