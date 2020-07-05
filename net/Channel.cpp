//@Author Yuz

#pragma once
#include "Channel.h"
#include "eventloop.h"


namespace net{
const int Channel::kNoneEvent = 0;
const int Channel::kReadEvent = EPOLLIN |  EPOLLET;
const int Channel::kWriteEvent = EPOLLOUT;

Channel::Channel(EventLoop* loop, int fd__):
    loop_(loop),
    fd_(fd__),
    events_(0),
    revents_(0),
    index_(-1)
{
}

Channel::~Channel()
{
    assert(!eventHandling_);
    if(loop_isInLoopThread())
    {
        //
    }

}

void Channel::update()
{
    loop_->updateChannel(this);
}

void Channel::remove()
{
    assert(isNoneEvent());
    loop_->removeChannel(this);
}

void Channel::handleEvent()
{
    events_ = 0;
    if((revents_ & EPOLLHUP) && !(revents_ &EPOLLIN))
    {
        events_ = 0;
        return;
    }
    if(revents_ & EPOLLERR)
    {
        if(errorCallback_) errorCallback_();
        events_ = 0;
        return;
    }
    if(revents_ & (EPOLLIN | EPOLLPRI | EPOLLRDHUP))
    {
        readCallback();
    }
    if(revents_ & EPOLLOUT)
    {
        writeCallback();
    }
}
}



