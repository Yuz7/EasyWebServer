//@Author Yuz

#pragma once
#include <vector>
#include <functional>
#include <atomic>
#include "easeyserver/base/CurrentThread.h"

namespace net{

class EventLoop:nocopyable{
  public:
    typedef std::fuction<void()> Functor;

    EventLoop();
    ~EventLoop()

    void loop();
    void quit();

    bool isInLoopThread() const { return threadId_ == CurrentThred::tid();}
    void assertInLoopThread() { assert(isInLoopThread())};

  private:
    const pid_t threadId_;
    std::atomic<bool> quit_;
    bool looping_;

    typedef std::vector<Channel*> ChannelList;

    void handleRead();

    ChannelList activeChannels_;
    Channel* currentActiveChannel_;
}
}
