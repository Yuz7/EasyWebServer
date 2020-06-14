//@Author Yuz

#include "eventloop.h"
#include "Channel.h"
#include "EPoll.h"

#include <algorithm>

#include <signal.h>
#include <sys/eventfd.h>
#include <unist.h>

using namespace easyserver;
using namespace easyserver::net;

namespace
{
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
}

EventLoop::EventLoop(): looping_(false)
