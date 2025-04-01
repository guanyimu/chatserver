#include "chatservice.hpp"
#include "public.hpp"
#include <string>
#include <vector>
#include <map>
#include <iostream>
#include <muduo/base/Logging.h>
using namespace muduo;
using namespace std;
ChatService *ChatService::instance()
{
    static ChatService service;
    return &service;
}

// 注册消息以及对应的回调操作
ChatService::ChatService()
{
    _msgHandlerMap.insert({LOGIN_MSG, std::bind(&ChatService::login, this, _1, _2, _3)});
    _msgHandlerMap.insert({REG_MSG, std::bind(&ChatService::reg, this, _1, _2, _3)});
    _msgHandlerMap.insert({ONE_CHAT_MSG, std::bind(&ChatService::oneChat, this, _1, _2, _3)});
    _msgHandlerMap.insert({ADD_FRIEND_MSG, std::bind(&ChatService::addFriend, this, _1, _2, _3)});

    _msgHandlerMap.insert({CREATE_GROUP_MSG, std::bind(&ChatService::createGroup, this, _1, _2, _3)});
    _msgHandlerMap.insert({ADD_GROUP_MSG, std::bind(&ChatService::addGroup, this, _1, _2, _3)});
    _msgHandlerMap.insert({GROUP_CHAT_MSG, std::bind(&ChatService::groupChat, this, _1, _2, _3)});

    _msgHandlerMap.insert({LOGINOUT_MSG, std::bind(&ChatService::logInOut, this, _1, _2, _3)});

    if (_redis.connect())
    {
        _redis.init_notify_handler(std::bind(&ChatService::handleRedisSubscribeMessage, this, _1, _2));
    }
}

MsgHandler ChatService::getHandler(int msgid)
{
    // 记录错误日志，msgid没有对应的回调函数；
    auto it = _msgHandlerMap.find(msgid);
    if (it == _msgHandlerMap.end())
    {
        return [=](const TcpConnectionPtr &conn, json &js, Timestamp t)
        { LOG_ERROR << "msgid:" << msgid << " can not find handler!"; };
    }
    else
        return _msgHandlerMap[msgid];
}

void ChatService::login(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    // cout << js << endl;
    // LOG_INFO << "do login service!!!";
    int id = js["id"].get<int>();
    string pwd = js["password"];

    User user = _userModel.query(id);

    json response;
    response["msgid"] = LOGIN_MSG_ACK;
    if (user.getId() == id && user.getPwd() == pwd)
    {
        if (user.getState() == "online")
        {
            // 该用户已经登录
            response["errno"] = 2;
            response["errmsg"] = "该账号已经登录,请重新输入新账号";
        }
        else
        {
            {
                lock_guard<mutex> lock(_connMutex);
                // 这个地方可能被多个调用，所以需要考虑线程安全问题
                _userConnMap.insert({id, conn});
            }

            // id用户登录
            _redis.subscribe(id);

            // 登陆成功,更新用户状态信息
            user.setState("online");
            _userModel.updatestate(user);

            response["errno"] = 0;
            response["id"] = user.getId();
            response["name"] = user.getName();

            // 查询该用户是否有离线消息
            vector<string> vec = _offlineMsgModel.query(id);
            if (!vec.empty())
            {
                response["offlinemsg"] = vec;
                _offlineMsgModel.remove(id);
            }

            // 查询该用户的好友信息并返回
            vector<User> userVec = _friendModel.query(id);
            if (!userVec.empty())
            {
                vector<string> friend_vec;
                for (User &user : userVec)
                {
                    json js;
                    js["id"] = user.getId();
                    js["name"] = user.getName();
                    js["state"] = user.getState();
                    friend_vec.push_back(js.dump());
                }
                response["friends"] = friend_vec;
            }
        }
    }
    else
    {
        if (user.getId() == id)
        {
            response["errno"] = 1;
            response["errmsg"] = "密码错误";
        }
        else
        {
            response["errno"] = 3;
            response["errmsg"] = "该账号不存在";
        }
    }

    conn->send(response.dump());
}

// 处理注册业务 name password
void ChatService::reg(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    // LOG_INFO << "do reg service!!!";
    string name = js["name"];
    string pwd = js["password"];
    User user;
    user.setName(name);
    user.setPwd(pwd);

    bool state = _userModel.insert(user);

    // LOG_INFO << "do reg service2!!!";

    json response;
    response["msgid"] = REG_MSG_ACK;
    if (state)
    {
        // 注册成功
        response["id"] = user.getId();
        response["errno"] = 0;
    }
    else
    {
        // 注册失败
        response["errno"] = 1;
    }

    conn->send(response.dump());
}

void ChatService::clientCloseException(const TcpConnectionPtr &conn)
{
    User user;
    lock_guard<mutex> lock(_connMutex);
    for (auto it = _userConnMap.begin(); it != _userConnMap.end();)
    {
        if (it->second == conn)
        {
            user.setId(it->first);
            // 从map表删除用户的连接
            it = _userConnMap.erase(it);
            if (user.getId() != -1)
            {
                user.setState("offline");
                _userModel.updatestate(user);
                _redis.unsubscribe(user.getId());
            }
        }
        else
        {
            ++it;
        }
    }
}

void ChatService::oneChat(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int toid = js["toid"].get<int>();

    // 标识用户是否在线
    {
        lock_guard<mutex> lock(_connMutex);
        auto it = _userConnMap.find(toid);
        if (it != _userConnMap.end())
        {
            // toid在线，转发消息，因为需要_conn，所以得在锁的范围内
            // 服务器主动推送消息给toid用户
            it->second->send(js.dump());
            return;
        }
    }

    // 查询toid是否在线
    User user = _userModel.query(toid);
    if (user.getState() == "online")
    {
        _redis.publish(toid, js.dump());
        return;
    }

    // toid不在线，存储离线消息
    _offlineMsgModel.insert(toid, js.dump());
}

void ChatService::reset()
{
    // 把所有online状态的用户设置为offline
    _userModel.resetState();
}

void ChatService::addFriend(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int userid = js["id"].get<int>();
    int friendid = js["friendid"].get<int>();

    // 存储好友信息
    _friendModel.insert(userid, friendid);
}

void ChatService::createGroup(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int userid = js["id"].get<int>();
    string name = js["groupname"];
    string desc = js["groupdesc"];

    Group group(-1, name, desc);
    if (_groupModel.createGroup(group))
    {
        _groupModel.addGroup(userid, group.getId(), "creator");
    }
}

void ChatService::addGroup(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int userid = js["id"].get<int>();
    int groupid = js["groupid"].get<int>();
    _groupModel.addGroup(userid, groupid, "normal");
}

void ChatService::groupChat(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int userid = js["id"].get<int>();
    int groupid = js["groupid"].get<int>();
    vector<int> useridVec = _groupModel.queryGroupUsers(userid, groupid);

    for (int id : useridVec)
    {
        lock_guard<mutex> lock(_connMutex);
        auto it = _userConnMap.find(id);
        if (it != _userConnMap.end())
        {
            it->second->send(js.dump());
        }
        else
        {
            // 查询toid是否在线
            User user = _userModel.query(id);
            if (user.getState() == "online")
            {
                _redis.publish(id, js.dump());
            }
            else
            {
                _offlineMsgModel.insert(id, js.dump());
            }
        }
    }
}

void ChatService::logInOut(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int userid = js["id"].get<int>();

    User user;
    user.setId(userid);
    user.setState("offline");
    _userModel.updatestate(user);

    {
        lock_guard<mutex> lock(_connMutex);
        _userConnMap.erase(userid);
    }

    // id用户登录
    _redis.unsubscribe(userid);
}

// 从redis消息队列获得订阅的消息
// redis收到消息，通过调用这个回调函数，把消息返回回来
void ChatService::handleRedisSubscribeMessage(int userid, string msg)
{
    json js = json::parse(msg.c_str());

    lock_guard<mutex> lock(_connMutex);
    auto it = _userConnMap.find(userid);
    if (it != _userConnMap.end())
    {
        it->second->send(js.dump());
        return;
    }

    _offlineMsgModel.insert(userid, msg);
}
