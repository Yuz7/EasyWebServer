//@Author Yuz
//
#pragma once;
#include "Socket.h"
#include <sys/sockets.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

int setNonBlocking(int fd)
{
    int flag = fcntl(fd,F_GETFL,0);
    if (flag == -1) return -1;

    flag |= O_NONBLOCK;
    if (fcntl(fd, F_SETFL, flag) == -1) return -1;
    return 0;
}

void setSocketNodelay(int fd)
{
    int enable = 1;
    setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, (VOID *)&enable, sizeof(enable));
}

void setSocketNoLinger(int fd)
{
    struct linger linger_;
    linger_.l_onoff = 1;
    linger_.l_linger = 30;
    setsockopt(fd, SOL_SOCKET, SO_LINGER, (const char *)&linger_, sizeof(linger_));
}

int socket_bind_listen(int port)
{
    if(port < 0 || port > 65535) return -1;

    int listenfd = 0;
    if((listenfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) return -1;

    int optval = 1;
    if( setsockopt(listenfd, SOL_SOCKET, SOREUSEADDR, &optval, sizeof(optval)) == -1)
    {
        close(listenfd);
        return -1;
    }

    struct sockaddr_in server_addr;
    bzero((char *)&server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons((unsigned short)port);
    if(bind(listenfd, (struct sockaddr *)&server_addr, sizeof(server_addr))==-1)
    {
        close(listenfd);
        return -1;
    }

    if(listenfd == -1)
    {
        close(listenfd);
        return -1;
    }
    return listenfd;
}
