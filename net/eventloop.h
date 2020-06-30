//@Author Yuz
#pragma once
#include <vector>
#include <functional>
#include <atomic>
#include "easeyserver/base/CurrentThread.h"

namespace net
{

class Channel;
class Epoller;

class EventLoop:nocopyable{
  public:
    typedef std::fuction<void()> Functor;

    EventLoop();
    ~EventLoop()

    void loop();
    void quit();

    bool isInLoopThread() const { return threadId_ == CurrentThred::tid();}
    void assertInLoopThread() { assert(isInLoopThread())};

    void runInLoop(Functor&& cb);
    void queueInLoop(Functor&& cb);

  private:
    const pid_t threadId_;
    std::atomic<bool> quit_;
    bool looping_;
    bool eventHandling_;
    mutable MutexLock mutex_;

    std::unique_ptr<Epoll> Epoller_;

    typedef std::vector<Channel*> ChannelList;

    void handleRead();

    int iteration_;
    ChannelList activeChannels_;
    Channel* currentActiveChannel_;
    std::vector<Functor> pendingFunctors_;

    bool callingPendingFunctors_;
    int wakeupFd_;
    std::unique_ptr<Channel> wakeupChannel_;

    void wakeup();
}
}

