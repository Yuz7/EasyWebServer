//@Author Yuz
#include "Server.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include "Socket.h"
#include <functional>
#include <arpa/inet.h>
#include <assert.h>
#include "TcpConn.h"
#include <string.h>

Server::Server(EventLoop *loop, int threadNum, int port):
        loop_(loop),
        threadNum_(threadNum),
        threadpool_(new EventLoopThreadPool(loop_, threadNum)),
        port_(port),
        listenfd_(socket_bind_listen(port_)),
        started_(false),
        acceptor_(new Channel(loop_, listenfd_))
{
    acceptor_->setReadCallback(std::bind(&Server::handleRead,this));
    acceptor_->enableReading();
    assert(setNonBlocking(listenfd_) >= 0);
}

void Server::start()
{
    threadpool_->start();
    started_ = true;
}

void Server::handleRead()
{
    struct sockaddr_in client_addr;
    memset(&client_addr, 0, sizeof(struct sockaddr_in));
    socklen_t client_addr_len = sizeof(client_addr);
    int accept_fd = 0;
    while((accept_fd = accept(listenfd_,(struct sockaddr *)&client_addr,&client_addr_len))>0)
    {
        EventLoop *ioloop = threadpool_->getNextLoop();
        if (accept_fd >= MAXFDS)
        {
            close(accept_fd);
            continue;
        }

        if(setNonBlocking(accept_fd) < 0)
        {
            // log
            return;
        }

        setSocketNodelay(accept_fd);

        std::shared_ptr<TcpConn> TcpConnPtr(new TcpConn(ioloop, accept_fd));
        ioloop->runInLoop(std::bind(&TcpConn::ConnEstablish,TcpConnPtr));
    }
    acceptor_->enableReading();
}
