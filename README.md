# EasyWebServer
通过学习muduo网络库，实现自己的Linux下c++轻量高性能Web服务器，计划实现：
* 使用epoll(ET)+non-blocking IO+one loop per thread并发模型 √
* 缓冲区初步仅使用string(后续将改为内存池链表) √
* 使用有限状态机来解析Http请求，支持解析get和post请求 √
* 定时器基于小根堆，用其处理超时请求与非活动链接 √
* 采用多线程异步日志来记录服务器运行状态

## 框架
多线程服务器模型使用non-blocking IO + one loop per thread模型，也就是每个IO线程有一个event loop（或称Reactor）来处理读写和定时事件。  
其中有一个MainReactor负责响应客户端请求，建立连接，并使用Round Robin的方式将连接分配给SubReactor。同时使用eventfd来进行异步唤醒阻塞的SubReactor来处理活跃事件。
![并发模型](https://github.com/linyacool/WebServer/blob/master/datum/model.png)

## Buffer //TODO
缓冲区计划将用一个缓冲链表来管理内存，链表通过一个evbuffer管理，以此来实现zero copy。

## 测试

