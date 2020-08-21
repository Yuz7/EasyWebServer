//@Author Yuz
#pragma once
#include <vector>
#include <functional>
#include <atomic>
#include "base/CurrentThread.h"
#include "base/Mutex.h"
#include <assert.h>
#include <memory>

class Channel;
class Epoll;

typedef std::vector<Channel*> ChannelList;
    
class EventLoop{
  public:
    typedef std::function<void()> Functor;

    EventLoop();
    ~EventLoop();

    void loop();
    void quit();

    bool isInLoopThread() const { return threadId_ == CurrentThread::tid();}
    void assertInLoopThread() { assert(isInLoopThread()); }

    void runInLoop(Functor&& cb);
    void queueInLoop(Functor&& cb);

    void updateChannel(Channel* channel);
    void removeChannel(Channel* channel);
  private:
    const pid_t threadId_;
    std::atomic<bool> quit_;
    bool looping_;
    bool eventHandling_;
    mutable MutexLock mutex_;

    std::unique_ptr<Epoll> Epoller_;

    void handleRead();

    int iteration_;
    ChannelList activeChannels_;
    Channel* currentActiveChannel_;
    std::vector<Functor> pendingFunctors_;

    bool callingPendingFunctors_;
    int wakeupFd_;
    std::unique_ptr<Channel> wakeupChannel_;

    void wakeup();
};
