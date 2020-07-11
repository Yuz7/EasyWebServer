// @Author Yuz
#pragma once
#include "Channel.h"
#include "eventloop.h"
#include <sys/epoll.h>
#include <map>
#include <memory>
#include <unistd.h>

enum URIState
{
    PARSE_URI_AGAIN = 1,
    PARSE_URI_ERROR,
    PARSE_URI_SUCCESS
};

enum HeaderState
{
    PARSE_HEADER_SUCCESS = 1,
    PARSE_HEADER_AGAIN,
    PARSE_HEADER_ERROR
};

enum ProcessState
{
    STATE_PARSE_URI = 1,
    STATE_PARSE_HEADERS,
    STATE_RECV_BODY,
    STATE_ANALYSIS,
    STATE_FINISH
};

enum AnalysisState { ANALYSIS_SUCCESS = 1, ANALYSIS_ERROR };

enum ConnState { kConnecting = 0, kConnected, kDisConnecting, kDisConnected };

enum ParseState
{
    H_START = 0,
    H_KEY,
    H_COLON,
    H_SPACES_AFTER_COLON,
    H_VALUE,
    H_CR,
    H_LF,
    H_END_CR,
    H_END_LF
};

enum HTTPMethod { METHOD_POST = 1, METHOD_GET, METHOD_HEAD };

enum HTTPVersion { HTTP1_0 = 1, HTTP1_1 };

class TcpConn
{
    public:
        TcpConn(EventLoop*loop,int fd);
        ~TcpConn();

        void reset();
        void seperateTimer();
        void linkTimer(std::shared_ptr<TimerNode> ctimer) { timer_ = ctimer; }

        void connEstabilsh();
        void handleClose();

    private:
        EventLoop* loop_;
        int fd_;
        bool error_;
        std::shared_ptr<Channel> channel_;
        ConnState connState_;

        HTTPMethod method_;
        HTTPVersion httpversion_;
        ProcesssState procstate_;
        ParseState parsestate_;

        std::string fileName_;
        std::string path_;
        int nowReadPos_;
        std::map<std::string, std::string> headers_;
        std::weak_ptr<TimerNode> timer_;

        void setState(ConnState connState) { connState_ = connState; }

        void handleRead();
        void handleWrite();
        void handleError(int fd, int err_num, std::string short_msg);
        URIState parseURI();
        HeaderState parseHeaders();
        AalysisState analysisRequest();
};
