// @Author Yuz
#include "TcpConn.h"
#include "Channel.h"
#include "EventLoop.h"
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <iostream>
#include "Socket.h"
#include "time.h"

const int DEFAULT_EXPIRED_TIME = 2000;

TcpConn::TcpConn(EventLoop *loop, int fd):
    loop_(loop),
    fd_(fd),
    error_(false),
    channel_(new Channel(loop_, fd_)),
    connState_(kConnecting),
    method_(METHOD_GET),
    httpversion_(HTTP1_1),
    procstate_(STATE_PARSE_URI),
    parsestate_(H_START),
    nowReadPos_(0)
{
    channel_->setReadCallback(std::bind(&TcpConn::handleRead,this));
    channel_->setWriteCallback(std::bind(&TcpConn::handleWrite,this));
    //channel_->setErrorCallback(std::bind(&TcpConn::handleError,this));
}

void TcpConn::ConnEstablish()
{
    loop_->assertInLoopThread();
    assert(state_ == kConnecting);
    setState(kConnected);

    channel_->enableReading();
}
/*
void TcpConn::handleClose() {
    connectionState_ = kDisConnected;
    shared_ptr<TcpConn> guard(shared_from_this());

}
*/
void TcpConn::reset(){
    fileName_.clear();
    path_.clear();
    nowReadPos_ = 0;
    procstate_ = STATE_PARSE_URI;
    parsestate_ = H_START;
    headers_.clear();
}

void TcpConn::seperateTimer() {
    if(timer.lock) {
        shared_ptr<TimerNode> crtimer(timer_.lock());
        crtimer->clearReq();
        timer_.reset();
    }
}

void TcpConn::handleRead()
{
    __uint32_t &events_ = channel_->getEvents();
    //int read_num = readfd(fd_,); TODO
    if(connState_ == kDisConnection)
    {
        //
        break;
    }

    if(read_num < 0) {
        perror("1");
        error_ = true;
        //handleError();
        break;
    }
    else if(read_num == 0)
    {
        error_ = true;
        break;
    }

    if(procstate_ == STATE_PARSE_URI)
    {
        URIState flag = this->parseURI();
        if(flag = PARSE_URI_ERROR) break;
        else if(flag == PARSE_URI_ERROR)
        {
            perror("2");
            error_ = true;
            //log
            //
            //handleError();
            break;
        }
        else procstate_ = STATE_PARSE_HEADERS;
    }
    if(procstate_ == STATE_PARSE_HEADERS)
    {
        HeaderState flag = this->parseHeaders();
        if(flag == PARSE_HEADER_AGAIN) break;
        else if (flag == PARSE_HEADER_ERROR)
        {
            //handleerror();
            error_ = true;
            break;
        }
        if(method_ == METHOD_POST)
        {
            procstate_ = STATE_RECV_BODY;
        } else{
            procstate_ = STATE_ANALYSIS;
        }
    }
    if(procstate_ == STATE_RECV_BODY){

    }
    if(procstate_ == STATE_ANALYSIS)
    {
        AnalysisState flag = this->analysisRequest();
        if(flag == ANALYSIS_SUCCESS)
        {
            procstate_ = STATE_FINISH;
            break;
        }else{
            error_ = true;
            break;
        }
    }
    if(!error_) {
        if(outBuffer_.size() > 0){
            handleWrite();
        }
        if(!error_ && procstate_ == STATE_FINISH)
        {
            this->reset();
            if(inBuffer_.size() > 0)
            {
                if(connState_ != kDisConnecting) handleRead();
            }
        } else if (!error_ && connState_ != kConnected) events_ |= EPOLLIN;
    }
}

void TcpConn::handleWrite()
{
    if(!error_&&connState_!=kDisConnected)
    {
        __uint32_t &events_ = channel_->getEvents();
        if(writen(fd_,)<0)
        {
            perror("written");
            events_ = 0;
            error_ = true;
        }
        if(outBuffer_.size() > 0) events_ |= EPOLLOUT;
    }
}

URIState TcpConn::parseURI()
{
    string &str = inBuffer_;
    string cop = str;
    size_t pos = str.find('\r', nowReadPos_);
    if(pos < 0)
    {
        return PARSE_URI_AGAGIN;
    }

    string request_line = str.substr(0, pos);
    if(str.size() > pos + 1)
        str = str.substr(pos+1);
    else
        str.clear();
    int posGet = request_line.find("GET");
    int posPost = request_line.find("POST");
    int posHead = request_line.find("HEAD");

    if(posGet >= 0){
        pos = posGet;
        method_ = METHOD_GET;
    } else if(posPost >= 0){
        pos = posPost;
        method_ = METHOD_POST;
    } else if(posHead >= 0){
        pos = posHead;
        method_ = METHOD_HEAD;
    } else {
        return PARSE_URI_ERROR;
    }

    pos = request_line.find("/",pos);
    if(pos < 0){
        fileName_ = "index.html";
        httpversion_ = HTTP1_1;
        return PARSE_URI_SUCCESS;
    } else {
        size_t _pos = request_line.find(' ', pos);
        if(_pos < 0)
            return PARSE_URI_ERROR;
        else{
            if(_pos - pos > 1){
                fileName_ = request_line.substr(pos + 1, _pos - pos - 1);
                size_t __pos = fileName_.find("?");
                if(__pos>=0){
                    fileName_ = fileName_.substr(0,__pos);
                }
            }
            else fileName_ = "index.html";
        }
        pos = _pos;
    }
    pos = request_line.find("/", pos);
    if(pos < 0)
        return PARSE_URI_ERROR;
    else {
        if(request_line.size() - pos <= 3)
            return PARSE_URI_ERROR;
        else {
            string ver = request_line.substr(pos + 1, 3);
            if(ver == "1.0")
                httpversion_ = HTTP1_0;
            else if(ver == "1.1")
                httpversion_ = HTTP1_1;
            else
                return PARSE_URI_ERROR;
        }
    }
    return PARSE_URI_SUCCESS;
}

HeaderState TcpConn::parseHeaders(){
    string &str = inBuffer_;
    int key_start = -1, key_end = -1, value_start = -1, value_end = -1;
    int now_read_line_begin = 0;
    bool notFinish = true;
    size_t i =0;
    for(; i < str.size() && notFinish; ++i){
        switch(parsestate_){
            case H_START: {
                if(str[i]=='\n' || str[i] == '\r') break;
                parsestate_ = H_KEY;
                key_start = i;
                now_read_line_begin = i;
                break;
            }
            case H_KEY: {
                if(str[i]==':'){
                    key_end = i;
                    if(key_end - key_start <= 0) return PARSE_HEADER_ERROR;
                    parsestate_ = H_COLON;
                }else if(str[i] == '\n'|| str[i]=='\r'){
                    return PARSE_HEADER_ERROR;
                }
                break;
            }
            case H_COLON:{
                if(str[i]==' '){
                    parsestate_ = H_SPACES_AFTER_COLON;
                }else
                    return PARSE_HEADER_ERROR;
                break;
            }
            case H_SPACES_AFTER_COLON:{
                parsestate_ = H_VALUE;
                value_start = i;
                break;
            }
            case H_VALUE:{
                if(str[i] == '\r'){
                    parsestate_ = H_CR;
                    value_end = i;
                    if(value_end - value_strat <= 0) return PARSE_HEADER_ERROR;
                }else if(i - value_start > 255)
                    return PARSE_HEDER_ERROR;
                break;
            }
            case H_CR: {
                if(str[i] == '\n'){
                    parsestate_ = H_LF;
                    string key(str.begin()+key_start, str.begin()+key_end);
                    string value(str.begin()+value_start,str.begin()+value_end);
                    headers_[key] = value;
                    now_read_line_begin = i;
                }else
                    return PARSE_HEADER_ERROR;
                break;
            }
            case H_LF:{
                if(str[i]=='\r')
                    parsestate_ = H_END_CR;
                else{
                    key_start = i;
                    parsestate_ = H_KEY;
                }
                break;
            }
            case H_END_CR: {
                if(str[i]=='\n')
                    parsestate_ = H_END_LF;
                else
                    return PARSE_HEADER_ERROR;
                break;
            }
            case H_END_LF: {
                notFinish = false;
                key_start = i;
                now_read_line_begin = i;
                break;
            }
        }
        if(parsestate_ == H_END_LF){
            str = str.substr(i);
            return PARSE_HEADER_SUCCESS;
        }
        str = str.substr(now_read_line_begin);
        return PARSE_HEADER_AGAGIN;
    }
}
