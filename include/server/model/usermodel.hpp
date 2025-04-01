#ifndef USERMODEL_H
#define USERMODEL_H

#include "user.hpp"

// User表的数据操作类
class UserModel
{
public:
    // User表的增加方法
    bool insert(User &user);

    // User表的查询方法
    User query(int id);

    // User表的更新方法
    bool updatestate(User user);

    void resetState();
};

#endif