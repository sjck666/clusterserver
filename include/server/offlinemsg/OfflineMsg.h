#pragma once
#include <string>
#include <vector>
#include "MySQL.h"
// 提供离线消息表的操作接口方法
class OfflineMsg
{
public:
    // 存储用户的离线消息
    void OfflineMsgInsert(int userid, std::string msg);

    // 删除用户的离线消息
    void OfflineMsgRemove(int userid);

    // 查询用户的离线消息
    std::vector<std::string> OfflineMsgQuery(int userid);
};
