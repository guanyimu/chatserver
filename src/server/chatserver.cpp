#include "chatserver.hpp"
#include "chatservice.hpp"
#include "json.hpp"
#include <functional>
#include <iostream>
#include <string>
using namespace std;
using namespace placeholders;
using json = nlohmann::json;

ChatServer::ChatServer(EventLoop *loop,               // 时间循环
                       const InetAddress &listenAddr, // IP + port
                       const string &nameArg)         // 服务器的名字
    : _server(loop, listenAddr, nameArg), _loop(loop)
{
    _server.setConnectionCallback(std::bind(&ChatServer::onConnection, this, _1));
    _server.setMessageCallback(std::bind(&ChatServer::onMessage, this, _1, _2, _3));
    _server.setThreadNum(4);
};

void ChatServer::start()
{
    _server.start();
}

void ChatServer::onConnection(const TcpConnectionPtr &conn)
{
    if (conn->connected())
    {
        cout << conn->peerAddress().toIpPort() << " -> " << conn->localAddress().toIpPort() << "state online" << endl;
    }
    else
    {
        ChatService::instance()->clientCloseException(conn);

        // 客户端断开连接
        cout << conn->peerAddress().toIpPort() << " -> " << conn->localAddress().toIpPort() << "state offline" << endl;
        conn->shutdown();
    }
}

// 专门处理用户的读写事件的
void ChatServer::onMessage(const TcpConnectionPtr &conn, // 链接
                           Buffer *buffer,               // 缓冲区
                           Timestamp time)               // 时间信息
{
    string buf = buffer->retrieveAllAsString();
    // 数据的反序列化
    json js = json::parse(buf);

    // 通过js["msgid"]来获取一个业务处理器handler,这个是业务模块写好的,让网络模块跟业务模块解耦合
    auto masgHandler = ChatService::instance()->getHandler(js["msgid"].get<int>());
    // 回调消息绑定好的事件处理器,来执行相应的业务处理,完美解耦合
    masgHandler(conn, js, time);
}
