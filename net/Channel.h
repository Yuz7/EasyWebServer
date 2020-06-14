//@Author Yuz

#pragma once
#include "EventLoop.h"
#include <fuctional>

namespace easyserver{

namespace net{
class EventLoop;

class Channel: noncopyable{
  public:
    typedef std::fuction<void()> EventCallback;

    Channel(EventLoop*, int fd);

    void handleEvent();
    void setReadCallback(const EventCallback& cb)
    { readCallback_ = cb; }
    void setWriteCallback(const EventCallback& cb)
    { writeCallback_ = cb; }
    void setErrorCallback(const EventCallback& cb)
    { errorCallback_ = cb; }

    int fd() const { return fd_; }
    int events const { return events_; }
    void set_revents(int revt) { revents_ = revt;}

    void enableReading() { events_ |= kReadEvent; update();}
    void enableWriting() { events_ |= kWriteEvent; update();}
    void disableWriting() { events_ &= ~kWriteEvent; update(); }
    void disableAll() { events_ = kNoneEvent; update(); }

    int index() { return index_; }
    void set_index(int idx) { index_ = idx; }
    
  private:
    void update();

    static const int kNoneEvent;
    static const int kReadEvent;
    static const int kWriteEvent;

    EventLoop*	loop_;
    const int	fd_;
    int		events_;
    int		revents_;
    int		index_;

    EventCallback readCallback_;
    EventCallback writeCallback_;
    EventCallback errorCallback_;


}
}




}
