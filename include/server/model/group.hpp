#ifndef GROUP_H
#define GROUP_H

#include "user.hpp"
#include <string>
#include <vector>
using namespace std;

class GroupUser : public User
{
    string role;

public:
    GroupUser(int id = -1, string name = "", string pwd = "", string state = "offline", string role = "") : User(id, name, pwd, state), role(role)
    {
    }
    void setRole(string role) { this->role = role; }
    string getRole() { return role; }
};

class Group
{
    int id;
    string name;
    // 组功能描述
    string desc;

    // 用来保存某个群组里有多少群员
    vector<GroupUser> users;

public:
    Group(int id = -1, string name = "", string desc = "") : id(id), name(name), desc(desc) {}

    void setId(int id) { this->id = id; }
    void setName(string name) { this->name = name; }
    void setDesc(string desc) { this->desc = desc; }

    int getId() { return id; }
    string getName() { return name; }
    string getDesc() { return desc; }
    vector<GroupUser> &getUsers() { return users; }
};

#endif