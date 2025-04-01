#ifndef GROUPMODEL_HPP
#define GROUPMODEL_HPP

#include "group.hpp"
#include <string>
#include <vector>
using namespace std;

class GroupModel
{
public:
    // 创建群组
    bool createGroup(Group &group);
    // 加入群组
    void addGroup(int userid, int groupid, string role);
    // 查询用户所在群组信息
    vector<Group> queryGroups(int userid);
    // 根据指定的groupid查询群组用户id列表，要排除userid自己，用于给除了自己以外的群组成员发消息
    vector<int> queryGroupUsers(int userid, int groupid);
};

#endif