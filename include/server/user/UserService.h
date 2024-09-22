#pragma once
#include "User.h"
#include "MySQL.h"
// User表的数据操作类
class UserService {
public:
    // User表的增加方法
    bool Insert(User &user);

    // 根据用户号码查询用户信息
    User Query(int id);

    // 更新用户的状态信息
    bool UpdateState(User user);

    // 重置用户的状态信息
    void ResetState();
};
