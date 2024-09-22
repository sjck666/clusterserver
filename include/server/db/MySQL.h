#pragma once
#include <mysql/mysql.h>
#include <string>

// 数据库操作类
class MySQL
{
public:
    // 初始化数据库连接
    MySQL();
    // 释放数据库连接资源
    ~MySQL();
    
    // 连接数据库
    bool Connect();
    
    // 更新操作
    bool Update(const std::string& sql);
    
    // 查询操作
    MYSQL_RES* Query(const std::string& sql);
    
    // 获取连接
    MYSQL* GetConnection();

private:
    MYSQL* _conn;
};