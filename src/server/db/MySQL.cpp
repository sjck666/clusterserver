#include "MySQL.h"
#include <muduo/base/Logging.h>
#include <mysql/mysql.h>

// 数据库配置信息
static const std::string server = "127.0.0.1";
static const std::string user = "root";
static const std::string password = "123456";
static const std::string dbname = "chat";

// 初始化数据库连接
MySQL::MySQL() : _conn(mysql_init(nullptr))
{
    if (_conn == nullptr) {
        LOG_ERROR << "mysql_init failed!";
    }
}

// 释放数据库连接资源
MySQL::~MySQL()
{
    if (_conn != nullptr) {
        mysql_close(_conn);
    }
}

// 连接数据库
bool MySQL::Connect()
{
    if (_conn == nullptr) {
        LOG_ERROR << "Connection is null, cannot connect to database.";
        return false;
    }

    MYSQL *p = mysql_real_connect(_conn, server.c_str(), user.c_str(),
                                  password.c_str(), dbname.c_str(), 3306, nullptr, 0);
    if (p == nullptr) {
        LOG_ERROR << "Connect to MySQL failed: " << mysql_error(_conn);
        return false;
    }

    // 设置字符集
    if (mysql_set_character_set(_conn, "utf8") != 0) {
        LOG_ERROR << "Failed to set character set: " << mysql_error(_conn);
        return false;
    }

    return true;
}

// 更新操作
bool MySQL::Update(const std::string& sql)
{
    if (mysql_query(_conn, sql.c_str())) {
        LOG_ERROR << __FILE__ << ":" << __LINE__ << ": "
                  << sql << " 更新失败: " << mysql_error(_conn);
        return false;
    }

    return true;
}

// 查询操作
MYSQL_RES* MySQL::Query(const std::string& sql)
{
    if (mysql_query(_conn, sql.c_str())) {
        LOG_ERROR << __FILE__ << ":" << __LINE__ << ": "
                  << sql << " 查询失败: " << mysql_error(_conn);
        return nullptr;
    }
    
    return mysql_store_result(_conn);
}

// 获取连接
MYSQL* MySQL::GetConnection()
{
    return _conn;
}