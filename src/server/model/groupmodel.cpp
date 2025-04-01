#include "groupmodel.hpp"
#include "db.hpp"

// 创建群组
bool GroupModel::createGroup(Group &group)
{
    char sql[1024] = {0};
    MySQL mysql;

    sprintf(sql, "insert into allgroup(groupname, groupdesc) values(%s, %s)", group.getName().c_str(), group.getDesc().c_str());
    if (mysql.connect())
    {
        if (mysql.update(sql))
        {
            group.setId(mysql_insert_id(mysql.getConnection()));
            return true;
        }
    }
    return false;
}

// 加入群组
void GroupModel::addGroup(int userid, int groupid, string role)
{
    char sql[1024] = {0};
    MySQL mysql;

    sprintf(sql, "insert into groupuser values(%d, %d, %s)", groupid, userid, role.c_str());
    if (mysql.connect())
    {
        mysql.update(sql);
    }
}

// 查询用户所在群组信息
vector<Group> GroupModel::queryGroups(int userid)
{
    char sql[1024] = {0};
    sprintf(sql, "select a.id, a.groupname, a.groupdesc from allgroup a right join groupuser b on a.id=b.groupid where b.userid=%d;", userid);

    vector<Group> vec;
    MySQL mysql;
    if (mysql.connect())
    {
        MYSQL_RES *res = mysql.query(sql);
        if (res != nullptr)
        {
            MYSQL_ROW row;
            while ((row = mysql_fetch_row(res)) != nullptr)
            {
                vec.emplace_back(atoi(row[0]), row[1], row[2]);
            }

            mysql_free_result(res);
        }
    }

    for (auto &g : vec)
    {
        char sql1[1024] = {0};
        sprintf(sql1, "select a.id, a.name, a.state, b.grouprole from user a inner join groupuser b on a.id=b.userid where b.groupid=%d;", g.getId());

        if (mysql.connect())
        {
            MYSQL_RES *res = mysql.query(sql1);
            if (res != nullptr)
            {
                MYSQL_ROW row;
                while ((row = mysql_fetch_row(res)) != nullptr)
                {
                    g.getUsers().emplace_back(atoi(row[0]), row[1], row[2], row[3]);
                }

                mysql_free_result(res);
            }
        }
    }

    return vec;
}

// 根据指定的groupid查询群组用户id列表，要排除userid自己，用于给除了自己以外的群组成员发消息
vector<int> GroupModel::queryGroupUsers(int userid, int groupid)
{
    char sql[1024] = {0};
    sprintf(sql, "select userid from groupuser where groupid=%d and userid!=%d;", groupid, userid);

    vector<int> vec;
    MySQL mysql;
    if (mysql.connect())
    {
        MYSQL_RES *res = mysql.query(sql);
        if (res != nullptr)
        {
            MYSQL_ROW row;
            while ((row = mysql_fetch_row(res)) != nullptr)
            {
                vec.emplace_back(atoi(row[0]));
            }

            mysql_free_result(res);
        }
    }

    return vec;
}