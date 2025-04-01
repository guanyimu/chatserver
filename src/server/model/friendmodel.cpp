#include "friendmodel.hpp"

void FriendModel::insert(int userid, int friendid)
{
    char sql1[1024] = {0};
    char sql2[1024] = {0};
    MySQL mysql;

    sprintf(sql1, "insert into friend values(%d, %d)", userid, friendid);
    sprintf(sql2, "insert into friend values(%d, %d)", friendid, userid);
    if (mysql.connect())
    {
        mysql.update(sql1);
        mysql.update(sql2);
    }
}

vector<User> FriendModel::query(int userid)
{
    char sql[1024] = {0};
    sprintf(sql, "select a.id, a.name, a.state from user a inner join friend b on b.friendid=a.id where b.userid=%d;", userid);

    vector<User> vec;
    MySQL mysql;
    if (mysql.connect())
    {
        MYSQL_RES *res = mysql.query(sql);
        if (res != nullptr)
        {
            MYSQL_ROW row;
            while ((row = mysql_fetch_row(res)) != nullptr)
            {
                User user;
                user.setId(atoi(row[0]));
                user.setName(row[1]);
                user.setState(row[2]);
                vec.push_back(user);
            }

            mysql_free_result(res);
            return vec;
        }
    }

    return vec;
}