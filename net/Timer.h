// @Author Yuz
#pragma once
#include <unistd.h>
#include <deque>
#include <queue>
#include <memory>
#include "TcpConn.h"
#include "base/MutexLock.h"
#include "base/noncopyable.h"

class TcpConn;

class TimerNode{
    public:
        TimerNode(std::shared_ptr<TcpConn> reqConn, int timeout);
        ~TimerNode();
        TimerNode(TimerNode &rhs);
        void update(int timeout);
        bool isValid();
        void clearReq();
        void setDeleted() { deleted_ = true; }
        bool isDeleted() const { return deleted_; }
        size_t getExpTime() const { return expiredTime_; }

    private:
        bool deleted_;
        size_t expiredTime_;
        std:shared_ptr<TcpConn> TcpConnPtr;
};

struct TimerCmp{
    bool operator()(std::shared_ptr<TimerNode> &a,
            std::shared_ptr<TimerNode> &b) const{
        return a->getExpTime() > b->getExpTime();
    }
};

class TimerManager {
    public:
        TimerManager() = default;
        ~TimerManager() = default;
        void addTimer(std::shared_ptr<TcpConn> TcpConnptr, int timeout);
        void handleExpiredEvent();

    private:
        typedef std::shared_ptr<TimerNode> TimerNodePtr;
        std::priority_queue<TimerNodePtr, std::deque<SPTimerNode>, TimerCmp>
            TimerNodeQ;
};
