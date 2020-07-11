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

char favicon[555] = {
    '\x89', 'P',    'N',    'G',    '\xD',  '\xA',  '\x1A', '\xA',  '\x0',
    '\x0',  '\x0',  '\xD',  'I',    'H',    'D',    'R',    '\x0',  '\x0',
    '\x0',  '\x10', '\x0',  '\x0',  '\x0',  '\x10', '\x8',  '\x6',  '\x0',
    '\x0',  '\x0',  '\x1F', '\xF3', '\xFF', 'a',    '\x0',  '\x0',  '\x0',
    '\x19', 't',    'E',    'X',    't',    'S',    'o',    'f',    't',
    'w',    'a',    'r',    'e',    '\x0',  'A',    'd',    'o',    'b',
    'e',    '\x20', 'I',    'm',    'a',    'g',    'e',    'R',    'e',
    'a',    'd',    'y',    'q',    '\xC9', 'e',    '\x3C', '\x0',  '\x0',
    '\x1',  '\xCD', 'I',    'D',    'A',    'T',    'x',    '\xDA', '\x94',
    '\x93', '9',    'H',    '\x3',  'A',    '\x14', '\x86', '\xFF', '\x5D',
    'b',    '\xA7', '\x4',  'R',    '\xC4', 'm',    '\x22', '\x1E', '\xA0',
    'F',    '\x24', '\x8',  '\x16', '\x16', 'v',    '\xA',  '6',    '\xBA',
    'J',    '\x9A', '\x80', '\x8',  'A',    '\xB4', 'q',    '\x85', 'X',
    '\x89', 'G',    '\xB0', 'I',    '\xA9', 'Q',    '\x24', '\xCD', '\xA6',
    '\x8',  '\xA4', 'H',    'c',    '\x91', 'B',    '\xB',  '\xAF', 'V',
    '\xC1', 'F',    '\xB4', '\x15', '\xCF', '\x22', 'X',    '\x98', '\xB',
    'T',    'H',    '\x8A', 'd',    '\x93', '\x8D', '\xFB', 'F',    'g',
    '\xC9', '\x1A', '\x14', '\x7D', '\xF0', 'f',    'v',    'f',    '\xDF',
    '\x7C', '\xEF', '\xE7', 'g',    'F',    '\xA8', '\xD5', 'j',    'H',
    '\x24', '\x12', '\x2A', '\x0',  '\x5',  '\xBF', 'G',    '\xD4', '\xEF',
    '\xF7', '\x2F', '6',    '\xEC', '\x12', '\x20', '\x1E', '\x8F', '\xD7',
    '\xAA', '\xD5', '\xEA', '\xAF', 'I',    '5',    'F',    '\xAA', 'T',
    '\x5F', '\x9F', '\x22', 'A',    '\x2A', '\x95', '\xA',  '\x83', '\xE5',
    'r',    '9',    'd',    '\xB3', 'Y',    '\x96', '\x99', 'L',    '\x6',
    '\xE9', 't',    '\x9A', '\x25', '\x85', '\x2C', '\xCB', 'T',    '\xA7',
    '\xC4', 'b',    '1',    '\xB5', '\x5E', '\x0',  '\x3',  'h',    '\x9A',
    '\xC6', '\x16', '\x82', '\x20', 'X',    'R',    '\x14', 'E',    '6',
    'S',    '\x94', '\xCB', 'e',    'x',    '\xBD', '\x5E', '\xAA', 'U',
    'T',    '\x23', 'L',    '\xC0', '\xE0', '\xE2', '\xC1', '\x8F', '\x0',
    '\x9E', '\xBC', '\x9',  'A',    '\x7C', '\x3E', '\x1F', '\x83', 'D',
    '\x22', '\x11', '\xD5', 'T',    '\x40', '\x3F', '8',    '\x80', 'w',
    '\xE5', '3',    '\x7',  '\xB8', '\x5C', '\x2E', 'H',    '\x92', '\x4',
    '\x87', '\xC3', '\x81', '\x40', '\x20', '\x40', 'g',    '\x98', '\xE9',
    '6',    '\x1A', '\xA6', 'g',    '\x15', '\x4',  '\xE3', '\xD7', '\xC8',
    '\xBD', '\x15', '\xE1', 'i',    '\xB7', 'C',    '\xAB', '\xEA', 'x',
    '\x2F', 'j',    'X',    '\x92', '\xBB', '\x18', '\x20', '\x9F', '\xCF',
    '3',    '\xC3', '\xB8', '\xE9', 'N',    '\xA7', '\xD3', 'l',    'J',
    '\x0',  'i',    '6',    '\x7C', '\x8E', '\xE1', '\xFE', 'V',    '\x84',
    '\xE7', '\x3C', '\x9F', 'r',    '\x2B', '\x3A', 'B',    '\x7B', '7',
    'f',    'w',    '\xAE', '\x8E', '\xE',  '\xF3', '\xBD', 'R',    '\xA9',
    'd',    '\x2',  'B',    '\xAF', '\x85', '2',    'f',    'F',    '\xBA',
    '\xC',  '\xD9', '\x9F', '\x1D', '\x9A', 'l',    '\x22', '\xE6', '\xC7',
    '\x3A', '\x2C', '\x80', '\xEF', '\xC1', '\x15', '\x90', '\x7',  '\x93',
    '\xA2', '\x28', '\xA0', 'S',    'j',    '\xB1', '\xB8', '\xDF', '\x29',
    '5',    'C',    '\xE',  '\x3F', 'X',    '\xFC', '\x98', '\xDA', 'y',
    'j',    'P',    '\x40', '\x0',  '\x87', '\xAE', '\x1B', '\x17', 'B',
    '\xB4', '\x3A', '\x3F', '\xBE', 'y',    '\xC7', '\xA',  '\x26', '\xB6',
    '\xEE', '\xD9', '\x9A', '\x60', '\x14', '\x93', '\xDB', '\x8F', '\xD',
    '\xA',  '\x2E', '\xE9', '\x23', '\x95', '\x29', 'X',    '\x0',  '\x27',
    '\xEB', 'n',    'V',    'p',    '\xBC', '\xD6', '\xCB', '\xD6', 'G',
    '\xAB', '\x3D', 'l',    '\x7D', '\xB8', '\xD2', '\xDD', '\xA0', '\x60',
    '\x83', '\xBA', '\xEF', '\x5F', '\xA4', '\xEA', '\xCC', '\x2',  'N',
    '\xAE', '\x5E', 'p',    '\x1A', '\xEC', '\xB3', '\x40', '9',    '\xAC',
    '\xFE', '\xF2', '\x91', '\x89', 'g',    '\x91', '\x85', '\x21', '\xA8',
    '\x87', '\xB7', 'X',    '\x7E', '\x7E', '\x85', '\xBB', '\xCD', 'N',
    'N',    'b',    't',    '\x40', '\xFA', '\x93', '\x89', '\xEC', '\x1E',
    '\xEC', '\x86', '\x2',  'H',    '\x26', '\x93', '\xD0', 'u',    '\x1D',
    '\x7F', '\x9',  '2',    '\x95', '\xBF', '\x1F', '\xDB', '\xD7', 'c',
    '\x8A', '\x1A', '\xF7', '\x5C', '\xC1', '\xFF', '\x22', 'J',    '\xC3',
    '\x87', '\x0',  '\x3',  '\x0',  'K',    '\xBB', '\xF8', '\xD6', '\x2A',
    'v',    '\x98', 'I',    '\x0',  '\x0',  '\x0',  '\x0',  'I',    'E',
    'N',    'D',    '\xAE', 'B',    '\x60', '\x82',
};

