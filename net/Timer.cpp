// @Author Yuz
#include "Timer.h"
#include <sys/time.h>
#include <unistd.h>
#include <queue>

TimerNode::TimerNode(std::shared_ptr<TcpConn> reqConn,int timeout)
    : deleted_(false), TcpConnPtr(reqConn) {
    struct timeval now;
    gettimeofday(&now,NULL);
    expiredTime_ =
        (((now.tv_sec % 10000) * 1000) + (now.tv_usec / 1000)) + timeout;
}

TimerNode::~TimerNode() {
    if(TcpConnPtr) TcpConnPtr->handleClose();
}

TimerNode::TimerNode(TimerNode &rhs) : expiredTime_(0),TcpConnPtr(rhs.TcpConnPtr) {}

void TimerNode::update(int timeout) {
    struct timeval now;
    gettimeofday(&now, NULL);
    expiredTime_ =
        (((now.tv_sec % 10000) * 1000) + (now.tv_usec / 1000)) + timeout;
}

bool TimerNode::isValid(){
    struct timeval now;
    gettimeofday(&now,NULL);
    size_t temp = (((now.tv_sec % 10000) * 1000) + (now.tv_usec / 1000));
    if(temp < expiredTime_)
        return true;
    else {
        this->setDeleted();
        return false;
    }
}

void TimerNode::clearReq() {
    TcpConnPtr.reset();
    this->setDeleted();
}

void TimerManager::addTimer(std::shared_ptr<TcpConn> TcpConnPtr, int timeout) {
    TimerNodePtr node(new TimerNode(TcpConnPtr,timeout));
    TimerNodeQ.push(node);
    TcpConnPtr->linkTimer(node);
}

void TimerManager::handleExpiredEvent() {
    while(!TimerNodeQ.empty()) {
        TimerNodePtr topTimer = TimerNodeQ.top();
        if (topTimer->isDeleted())
            TimerNodeQ.pop();
        else if(topTimer->isValid() == false)
            TimerNodeQ.pop();
        else
            break;
    }
}
