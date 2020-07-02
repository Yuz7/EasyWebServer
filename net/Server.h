//@Author Yuz
#pragma once
#include <memory>
#include "Channel.h"
#include "eventloop.h"
#include "EventLoopThreadPool.h"

class Server
{
    public:
        Server(EventLoop *loop, int threadnum, int port);
        ~Server();

        EventLoop *getLoop() const { return loop_; }
        void start();
        void handleRead();

    private:
        EventLoop *loop_;
        int threadnum;
        std::unique_ptr<EventLoopThreadPool> threadpool_;
        int port_;
        int listenfd_;
        bool started_;
        std::unique_ptr<Channel> acceptor_;
        static const int MAXFDS = 100000;
}
