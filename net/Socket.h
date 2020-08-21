// @Author Yuz
#pragma once
#include <cstdlib>
#include <string>
int setNonBlocking(int fd);
int socket_bind_listen(int port);
void setSocketNodelay(int fd);
void setSocketNoLinger(int fd);


ssize_t readn(int fd, void *buff, size_t n);
ssize_t readn(int fd, std::string &inBuffer, bool &zero);
ssize_t writen(int fd, std::string &outBuffer);
ssize_t writen(int fd, void *buff, size_t n);
