// @Author Yuz
#pragma once
#include <cstdlib>

int setNonBlocking(int fd);
int socket_bind_listen(int port);
void setSocketNodelay(int fd);
void setSocketNoLinger(int fd);
