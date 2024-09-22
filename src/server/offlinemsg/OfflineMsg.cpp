#include "OfflineMsg.h"
#include "Interface.pb.h"
using namespace std;
// 存储用户的离线消息
void OfflineMsg::OfflineMsgInsert(int userid, std::string msg)
{
    
    // 1.组装sql语句
    char sql[1024] = {0};
    sprintf(sql, "insert into offlinemessage values(%d, '%s')", userid, msg.c_str());

    MySQL mysql;
    if (mysql.Connect())
    {
        mysql.Update(sql);
    }
    else
    {
        std::cout << "OfflineMsgInsert mysql connect failed" << std::endl;
    }
}

// 删除用户的离线消息
void OfflineMsg::OfflineMsgRemove(int userid)
{
    // 1.组装sql语句
    char sql[1024] = {0};
    sprintf(sql, "delete from offlinemessage where userid=%d", userid);

    MySQL mysql;
    if (mysql.Connect())
    {
        mysql.Update(sql);
    }
}

// 查询用户的离线消息
std::vector<std::string> OfflineMsg::OfflineMsgQuery(int userid)
{
    // 1.组装sql语句
    char sql[1024] = {0};
    sprintf(sql, "select message from offlinemessage where userid = %d", userid);

    vector<string> vec;
    MySQL mysql;
    if (mysql.Connect())
    {
        MYSQL_RES *res = mysql.Query(sql);
        if (res != nullptr)
        {
            // 把userid用户的所有离线消息放入vec中返回
            MYSQL_ROW row;
            while((row = mysql_fetch_row(res)) != nullptr)
            {
                vec.push_back(row[0]);
            }
            mysql_free_result(res);
            return vec;
        }
    }
    return vec;
}

