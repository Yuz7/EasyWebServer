//@Author Yuz

#include "eventloop.h"
#include "Channel.h"
#include "EPoll.h"
#include "EasyWebServer/base/CurrentThread.h"

#include <algorithm>

#include <signal.h>
#include <sys/eventfd.h>
#include <unist.h>

using namespace easyserver;
using namespace easyserver::net;

namespace
{
__thread EventLoop* t_loopInThisThread = 0;

const int kPollTimeMs = 10000;

int createEventfd()
{
  int evtfd = ::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
  if (evtfd <0)
  {
//    LOG_SYSERR << "Failed in eventfd";
    abort();
  }
  return evtfd;
}
}

EventLoop::EventLoop():
            looping_(false),
	    	quit_(true),
	    	threadID_(CurrentThread::ID()){},
            wakeupFd(createEventfd*()),
            wakeupChannel_(new Channel(this,wakeupFd_)),
            currentActiveChannel_(NULL),
            eventHandling_(false),
            iteration_(0)
{
    //log eventloop created;
    if(t_loopInthisThread)
    {
        //log eventloop existed;
    }
    else
    {
        t_loopInthisThread = this;
    }
    wakeupChannel_->setReadCallback(std::bind(&Eventloop::handleRead(),this));
    wakeupChannel_->enableReading();
}

EventLoop::~EventLoop()
{
    //log destructs;
    //wakeupChannel_->disableAll();
    //wakeupChannel_->remove();
    ::close(wakeupFd_);
    t_loopInthisThread = NULL;
}

void EventLoop::loop()
{
    assert(!looping_);
    assertInLoopThread();
    looping_ = true;
    quit_ = false;
    //log start looping

    while(!quit){
        activeChannels_.clear();
        ++iteration_;
        eventHandling_ = true;
        Epoller_->poll(kPollTimeMs, &activeChannels_);
        for(Channel* channel : activechannels_)
        {
            currentActiveChannel_ = channel;
            currentActiveChannel_->handleEvent();
        }
        currentActiveChannel_ = NULL;
        eventHandling_ = false;
        //dopendingfunctors;
    }

    looping_ = false;
}

void EventLoop::updateChannel(Channel* channel)
{
    Epoller_->updateChannel(channel);
}

void EventLoop::wakeup()
{
    uint64_t one = 1;
    ssize_t n = writen(wakeupFd_, (char*)(&one), sizeof(one));
    if(n != sizeof(one))
    {
        //log
    }
}

void EventLoop::runInLoop(Functor&& cb)
{
    if(isInLoopThread)
        cb();
    else
        queueInLoop(std::move(cb));
}

void EventLoop::queueInLoop(Functor&& cb)
{
    {
        MutexLockGuard lock(mutex_);
        pendingFunctors_emplace_back(std::move(cb));
    }

    if( !isInLoopThread() || callingpendingFunctors_) wakeup();
}
