// @Author Yuz

#pragma once
#include <sys/epoll.h>
#include <memory>
#include <vector>
#include "Channel.h"
#include <map>
#include "eventloop.h"

struct epoll_event;

class Epoll{
    public:
        Epoll();
        ~Epoll();
        void epoll_add();
        void epoll_mod();
        void epoll_del();
 
        void poll(int timeoutms, ChannelList* activeChannels);

	void updateChannel(Channel* channel);
	void removeChannel(Channel* channel);
    private:
        static const int kInitEventListSize = 16;

        typedef std::vector<struct epoll_event> EventList;

        void fillActiveChannels(int numEvents, ChannelList* activeChannels) const;

        void update(int operation, Channel* channel);

        int epollfd_;
        EventList events_;

        typedef std::map<int, Channel*> ChannelMap;
        ChannelMap channels_;
};
