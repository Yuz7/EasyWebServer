// @Author

#include "Epoll.h"
#include <assert.h>
#include "Channel.h"
#include <sys/epoll.h>
#include <errno.h>

using namespace easyserver;
using namespace easyserver::net;

namespace{
const int kNew = -1;
const int kAdded = 1;
const int kDeleted = 2;
}

Epoll::Epoll(EventLoop* loop):
    epollfd_(::epoll_create1(EPOLL_CLOEXEC)),
    events_(kInitEventListSize)
{
    if(epollfd_ < 0)
    {
        // log
    }
}

Epoll::~Epoll()
{
    ::close(epollfd_);
}

void Epoll::poll(int timeoutms,ChannelList* activechannels)
{
    int numEvents = epoll_wait(epolfd_, &*events_.begin(), events_.size(), timeoutms);
    if(numEvents > 0)
    {
        fillActiveChannels(numEvents, activechannels);
        if(implicit_cast<size_t>(numEvents) == events_.size())
        {
            events_.resize(events_.size() * 2);
        }
    }
    else if(numEvents == 0)
    {
        //log
    }
    else
    {
        // log errno;
    }
}

void Epoll::fillActiveChannels(int numEvents, ChannelList* activechannels) const
{
    assert(implicit_cast<size_t>(numEvents) <= events_.size());
    for(int i = 0; i < numEvents; ++i)
    {
        Channel* channel = static_cast<Channel*>(events_[i].data.ptr);
        channel->set_revents(events_[i].events);
        activechannels->push_back(channel);
    }
}

void Epoll::updateChannel(Channel* channel)
{
    const int index = channel->index();
    //log
    if(index == kNew || index == kDeleted)
    {
        int fd = channel->fd();
        if (index == kNew)
        {
            assert(channels_.find(fd) == channels_.end());
            channels_[fd] = channel;
        }
        else //kDeleted
        {
            assert(channels_find(fd) != channels_.end());
            assert(channels_[fd] == channel);
        }

        channel->set_index(kAdded);
        update(EPOLL_CTL_ADD, channel);
    }
    else // kAdded
    {
        int fd = channel->fd();
        assert(channels_.find(fd) != channels_.end());
        assert(channels_[fd] == channel);
        assert(index == kAdded);
        if(channel->isNoneEvent())
        {
            update(EPOLL_CTL_DEL,channel);
            channel->set_index(kDeleted);
        }
        else
        {
            update(EPOLL_CTL_MOD, channel);
        }
    }
}

void Epoll::update(int operation, Channel* channel)
{
    struct epoll_event event;
    memZero(&event, sizeof(event));
    event.events = channel->events();
    event.data.ptr = channel;
    int fd = channel->fd();
    //log
    if(::epoll_ctl(epollfd_, operation, fd, &event) < 0)
    {
        if(operation == EPOLL_CTL_DEL)
        {
            //log syserr
        }
        else
        {
            //log sysfatal
        }
    }
}
