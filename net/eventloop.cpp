//@Author Yuz

#include "eventloop.h"
#include "Channel.h"
#include "Epoll.h"
#include "base/CurrentThread.h"

#include <algorithm>

#include <signal.h>
#include <sys/eventfd.h>
#include <unistd.h>
#include "Socket.h"

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


EventLoop::EventLoop():
	threadId_(CurrentThread::tid()),
	    	quit_(true),
            looping_(false),
            eventHandling_(false),
            Epoller_(new Epoll()),
	    iteration_(0),
            currentActiveChannel_(NULL),
            wakeupFd_(createEventfd()),
            wakeupChannel_(new Channel(this,wakeupFd_))
{
    //log eventloop created;
    if(t_loopInThisThread)
    {
        //log eventloop existed;
    }
    else
    {
        t_loopInThisThread = this;
    }
    wakeupChannel_->setReadCallback(std::bind(&EventLoop::handleRead,this));
    wakeupChannel_->enableReading();
}

EventLoop::~EventLoop()
{
    //log destructs;
    //wakeupChannel_->disableAll();
    //wakeupChannel_->remove();
    ::close(wakeupFd_);
    t_loopInThisThread = NULL;
}

void EventLoop::loop()
{
    assert(!looping_);
    assertInLoopThread();
    looping_ = true;
    quit_ = false;
    //log start looping

    while(!quit_){
        activeChannels_.clear();
        ++iteration_;
        eventHandling_ = true;
        Epoller_->poll(kPollTimeMs, &activeChannels_);
        for(Channel* channel : activeChannels_)
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

void EventLoop::removeChannel(Channel* channel)
{
    Epoller_->removeChannel(channel);
}

void EventLoop::wakeup()
{
    int one = 1;
    ssize_t n = writen(wakeupFd_, (char*)(&one), sizeof(one));
    if(n != sizeof(one))
    {
        //log
    }
}

void EventLoop::runInLoop(Functor&& cb)
{
    if(isInLoopThread())
        cb();
    else
        queueInLoop(std::move(cb));
}

void EventLoop::queueInLoop(Functor&& cb)
{
    {
        MutexLockGuard lock(mutex_);
        pendingFunctors_.emplace_back(std::move(cb));
    }

    if( !isInLoopThread() || callingPendingFunctors_) wakeup();
}
