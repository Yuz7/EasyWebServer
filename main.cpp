// @Author Yuz
#include <getopt.h>
#include <string>
#include "eventloop.h"
#include "Server.h"

int main(int argc, char *argv[]) {
    int threadNum = 4;
    int port = 80;

    EventLoop mainLoop;
    Server httpserver(&mainLoop, threadNum, port);
    httpserver.start();
    mainLoop.loop();
    return 0;
}
