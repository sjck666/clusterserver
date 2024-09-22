#include "UserService.h"
// User表的增加方法
bool UserService::Insert(User &user)
{
    // 1.组装sql语句
    char sql[1024] = {0};
    sprintf(sql, "insert into user(name, password, state) values('%s', '%s', '%s')",
            user.GetName().c_str(), user.GetPwd().c_str(), user.GetState().c_str());

    MySQL mysql;
    if (mysql.Connect())
    {
        if (mysql.Update(sql))
        {
            // 获取插入成功的用户数据生成的主键id
            user.SetId(mysql_insert_id(mysql.GetConnection()));
            return true;
        }
    }

    return false;
}


// 根据用户号码查询用户信息
User UserService::Query(int id)
{
    // 1.组装sql语句
    char sql[1024] = {0};
    sprintf(sql, "select * from user where id = %d", id);

    MySQL mysql;
    if (mysql.Connect())
    {
        MYSQL_RES *res = mysql.Query(sql);
        if (res != nullptr)
        {
            MYSQL_ROW row = mysql_fetch_row(res);
            if (row != nullptr)
            {
                User user;
                user.SetId(atoi(row[0]));
                user.SetName(row[1]);
                user.SetPwd(row[2]);
                user.SetState(row[3]);
                mysql_free_result(res);
                return user;
            }
        }
    }

    return User();
}

// 更新用户的状态信息
bool UserService::UpdateState(User user)
{
    // 1.组装sql语句
    char sql[1024] = {0};
    sprintf(sql, "update user set state = '%s' where id = %d", user.GetState().c_str(), user.GetId());

    MySQL mysql;
    if (mysql.Connect())
    {
        if (mysql.Update(sql))
        {
            return true;
        }
    }
    return false;
}

// 重置用户的状态信息
void UserService::ResetState()
{
     // 1.组装sql语句
    char sql[1024] = "update user set state = 'offline' where state = 'online'";

    MySQL mysql;
    if (mysql.Connect())
    {
        mysql.Update(sql);
    }
}

