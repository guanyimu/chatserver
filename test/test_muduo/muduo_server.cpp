#include <muduo/net/TcpServer.h>
#include <muduo/net/EventLoop.h>
#include <iostream>
#include <string>
#include <functional>
using namespace std;
using namespace muduo;
using namespace muduo::net;
using namespace placeholders;

class ChatServer
{
public:
    // 第三步 明确TcpServer需要什么构造函数的参数，整ChatServer的构造函数
    ChatServer(EventLoop *loop,               // 时间循环
               const InetAddress &listenAddr, // IP + port
               const string &nameArg)         // 服务器的名字
        : _server(loop, listenAddr, nameArg), _loop(loop)
    {
        // 第四步 在当前服务器类的构造函数中，注册两个回调函数
        // 给服务器注册用户链接的创建断开回调
        _server.setConnectionCallback(std::bind(&ChatServer::onConnection, this, _1));
        // 给服务器注册用户读写事件回调
        _server.setMessageCallback(std::bind(&ChatServer::onMessage, this, _1, _2, _3));

        // 第五步 设置合适的服务器端的线程数量,muduo库会自己分配IO线程与工作线程
        _server.setThreadNum(4); // 1个IO线程,3个工作线程
    }

    void start()
    {
        // 开启事件循环
        _server.start();
    }

    void quit()
    {
        // 开启事件循环
        _loop->quit();
    }

private:
    // 专门处理用户的连接与断开
    void onConnection(const TcpConnectionPtr &conn)
    {
        if (conn->connected())
            cout << conn->peerAddress().toIpPort() << " -> " << conn->localAddress().toIpPort() << "state online" << endl;
        else
        {
            cout << conn->peerAddress().toIpPort() << " -> " << conn->localAddress().toIpPort() << "state offline" << endl;
            conn->shutdown();
        }
    }

    // 专门处理用户的读写事件的
    void onMessage(const TcpConnectionPtr &conn, // 链接
                   Buffer *buffer,               // 缓冲区
                   Timestamp time)               // 时间信息
    {
        string buf = buffer->retrieveAllAsString();
        cout << "recv data:" << buf << "time:" << time.toString() << endl;
        conn->send(buf);
    }

    // 第一步 组合tcp server对象
    TcpServer _server;

    // 第二步 创建EventLoop事件循环对象的指针
    EventLoop *_loop;
};

int main()
{
    EventLoop loop;
    InetAddress addr("127.0.0.1", 6000);
    ChatServer server(&loop, addr, "ChatServer");

    server.start();
    loop.loop(); // epoll_wait，以阻塞方式等待新用户链接，以链接用户的读写事件
    server.quit();
    return 0;
}