#pragma once
#include <muduo/net/TcpConnection.h>
#include <unordered_map>
#include <functional>
#include <mutex>
#include "UserService.h"
#include "OfflineMsg.h"
#include "muduo/base/Logging.h"
#include "AccountService.pb.h"
#include "myrpc/MyRpcChannel.h"
#include "myrpc/MyRpcController.h"
#include "FriendService.pb.h"
#include "GroupService.pb.h"
#include "Redis.h"
using MsgHandler = std::function<void(const muduo::net::TcpConnectionPtr &conn, std::string &recv_buf, muduo::Timestamp time)>;

class InterfaceService
{
public:
    // 获取单例对象
    static InterfaceService &GetInstance();
    InterfaceService &operator=(const InterfaceService &) = delete;
    InterfaceService(const InterfaceService &) = delete;
    // 获取消息处理器
    MsgHandler GetHandler(std::string msgType);

    // 处理登录业务
    void Login(const muduo::net::TcpConnectionPtr &conn, std::string &recv_buf, muduo::Timestamp time);
    // 处理注册业务
    void Register(const muduo::net::TcpConnectionPtr &conn, std::string &recv_buf, muduo::Timestamp time);
    // 处理注销业务
    void Logout(const muduo::net::TcpConnectionPtr &conn, std::string &recv_buf, muduo::Timestamp time);

    // 处理聊天业务
    void Chat(const muduo::net::TcpConnectionPtr &conn, std::string &recv_buf, muduo::Timestamp time);
    // 处理添加好友业务
    void AddFriend(const muduo::net::TcpConnectionPtr &conn, std::string &recv_buf, muduo::Timestamp time);
    // 处理获取好友列表业务
    void GetFriendList(const muduo::net::TcpConnectionPtr &conn, std::string &recv_buf, muduo::Timestamp time);

    // 处理创建群组业务
    void CreateGroup(const muduo::net::TcpConnectionPtr &conn, std::string &recv_buf, muduo::Timestamp time);
    // 处理加入群组业务
    void AddGroup(const muduo::net::TcpConnectionPtr &conn, std::string &recv_buf, muduo::Timestamp time);
    // 处理获取群组列表业务
    void GroupList(const muduo::net::TcpConnectionPtr &conn, std::string &recv_buf, muduo::Timestamp time);
    // 处理获取群组用户id列表业务
    void GetGroupUsers(const muduo::net::TcpConnectionPtr &conn, std::string &recv_buf, muduo::Timestamp time);
    // redis上报消息的回调函数
    void RedisSubscribeCallback(int channel, std::string message);

    //处理客户端异常退出
    void ClientCloseException(const muduo::net::TcpConnectionPtr &conn);
    // 服务器异常退出，重置服务器
    void Reset();
private:
    InterfaceService();
    // 存储消息处理器
    std::unordered_map<std::string, MsgHandler> _msgHandlerMap;

    // 数据操作类对象
    UserService _userService;
    // 离线消息对象
    OfflineMsg _offlineMsg;
    // redis对象
    Redis _redis;

    // 存储在线用户的通信连接
    std::unordered_map<int, muduo::net::TcpConnectionPtr> _userConnMap;
    // 互斥锁,保证_userConnMap线程安全
    std::mutex _connMutex;
};







