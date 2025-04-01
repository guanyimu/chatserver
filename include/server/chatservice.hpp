#ifndef CHATSERVICE_H
#define CHATSERVICE_H
#include <muduo/net/TcpConnection.h>
#include <unordered_map>
#include <functional>
#include <usermodel.hpp>
#include <mutex>
#include "redis.hpp"
#include "groupmodel.hpp"
#include "json.hpp"
#include "friendmodel.hpp"
#include "offlinemessagemodel.hpp"
using json = nlohmann::json;
using namespace muduo;
using namespace muduo::net;
using namespace std;
using MsgHandler = std::function<void(const TcpConnectionPtr &, json &, Timestamp)>;

// 聊天服务器，业务类
class ChatService
{
public:
    static ChatService *instance();
    // 处理登录业务
    void login(const TcpConnectionPtr &conn, json &js, Timestamp time);

    // 处理注册业务
    void reg(const TcpConnectionPtr &conn, json &js, Timestamp time);

    // 客户端异常退出
    void clientCloseException(const TcpConnectionPtr &conn);

    void oneChat(const TcpConnectionPtr &conn, json &js, Timestamp time);

    void addFriend(const TcpConnectionPtr &conn, json &js, Timestamp time);

    void reset();

    // 获取消息对应的处理器
    MsgHandler getHandler(int msgid);

    void createGroup(const TcpConnectionPtr &conn, json &js, Timestamp time);
    void addGroup(const TcpConnectionPtr &conn, json &js, Timestamp time);
    void groupChat(const TcpConnectionPtr &conn, json &js, Timestamp time);
    void logInOut(const TcpConnectionPtr &conn, json &js, Timestamp time);
    void handleRedisSubscribeMessage(int userid, string msg);

private:
    ChatService();

    UserModel _userModel;
    unordered_map<int, MsgHandler> _msgHandlerMap;

    // 存储在线用户的通信连接
    unordered_map<int, TcpConnectionPtr> _userConnMap;

    // 定义互斥锁，保证登录的线程安全
    mutex _connMutex;

    OfflineMsgModel _offlineMsgModel;

    FriendModel _friendModel;

    GroupModel _groupModel;

    // 负载均衡器会开一堆这个,每一个都有一个redis对象
    Redis _redis;
};
#endif