#ifndef CHATSERVER_H
#define CHATSERVER_H

#include <muduo/net/TcpServer.h>
#include <muduo/net/EventLoop.h>
using namespace muduo;
using namespace muduo::net;

class ChatServer
{
public:
    // 初始化聊天服务器对象
    ChatServer(EventLoop *loop,               // 时间循环
               const InetAddress &listenAddr, // IP + port
               const string &nameArg);        // 服务器的名字

    // 启动服务
    void start();

private:
    // 上报链接相关信息的回调函数
    void onConnection(const TcpConnectionPtr &conn);

    // 上报读写相关信息的回调函数
    void onMessage(const TcpConnectionPtr &conn, // 链接
                   Buffer *buffer,               // 缓冲区
                   Timestamp time);              // 时间信息

    TcpServer _server;
    EventLoop *_loop;
};

#endif