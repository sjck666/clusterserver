#include "InterfaceService.h"
#include "Interface.pb.h"
using namespace std;
InterfaceService& InterfaceService::GetInstance()
{
    static InterfaceService instance;
    return instance;
}

InterfaceService::InterfaceService()
{
    _msgHandlerMap["Register"] = std::bind(&InterfaceService::Register, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
    _msgHandlerMap["Login"] = std::bind(&InterfaceService::Login, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
    _msgHandlerMap["Logout"] = std::bind(&InterfaceService::Logout, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
    _msgHandlerMap["Chat"] = std::bind(&InterfaceService::Chat, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
    _msgHandlerMap["AddFriend"] = std::bind(&InterfaceService::AddFriend, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
    _msgHandlerMap["GetFriendList"] = std::bind(&InterfaceService::GetFriendList, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
    _msgHandlerMap["CreateGroup"] = std::bind(&InterfaceService::CreateGroup, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
    _msgHandlerMap["AddGroup"] = std::bind(&InterfaceService::AddGroup, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
    _msgHandlerMap["GetGroupList"] = std::bind(&InterfaceService::GroupList, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
    _msgHandlerMap["GroupChat"] = std::bind(&InterfaceService::GetGroupUsers, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);

    // 连接redis
    if(_redis.RedisConnect())
    {
        // 设置上报消息的回调函数
        _redis.InitSubscribeCallback(std::bind(&InterfaceService::RedisSubscribeCallback, this, std::placeholders::_1, std::placeholders::_2));
    }
    else
    {
        LOG_ERROR << "Redis connect failed";
    }

}   

//获得消息对应的处理器
MsgHandler InterfaceService::GetHandler(std::string msgType)
{
    auto it = _msgHandlerMap.find(msgType);
    if (it != _msgHandlerMap.end())
    {
        return it->second;
    }
    else 
    {
        // 如果没有对应的处理器，返回一个默认的处理器
        return [=](const muduo::net::TcpConnectionPtr &conn, std::string &recv_str, muduo::Timestamp time) 
        {
            LOG_ERROR << "No handler for msg type: " << msgType;
        };
    }
}

// 处理客户端异常退出
void InterfaceService::ClientCloseException(const muduo::net::TcpConnectionPtr &conn)
{
    
    User user;
    // 1.从在线用户列表中删除
    {
        // 2.保证线程安全
        std::lock_guard<std::mutex> lock(_connMutex);
        for (auto it = _userConnMap.begin(); it != _userConnMap.end(); ++it)
        {
            if (it->second == conn)
            {
                user.SetId(it->first);
                _userConnMap.erase(it);
                break;
            }
        }
    }
    
    if (user.GetId() != -1)
    {
        // 2.取消redis订阅
        _redis.RedisUnsubscribe(user.GetId());

        // 3.更新用户状态为离线
        user.SetState("offline");
        _userService.UpdateState(user);
    }
}

// 服务器异常退出，重置服务器
void InterfaceService::Reset()
{
    // 1.重置用户状态,全部设置为离线
    _userService.ResetState();
}

// 处理注册业务.格式：用户名，密码，包含在recv_str中，要进行反序列化传给负责注册业务的服务器
void InterfaceService::Register(const muduo::net::TcpConnectionPtr &conn, std::string &recv_str, muduo::Timestamp time)
{
    // 1. 检查接收字符串是否为空
    if (recv_str.empty())
    {
        LOG_ERROR << "Received empty message from client.";
        return;
    }

    // 1.1 反序列化recv_str
    Interface_::UnifiedMessage register_request;
    if (!register_request.ParseFromString(recv_str))
    {
        LOG_ERROR << "Failed to parse register request from string: " << recv_str;
        return;
    }

    // 1.2 验证请求内容
    if (register_request.name().empty() || register_request.password().empty())
    {
        LOG_ERROR << "Register request missing name or password.";
        return;
    }

    // 2. 构造rpc请求
    AccountService_::RegisterRequest request;
    request.set_name(register_request.name());
    request.set_password(register_request.password());

    // 3. 调用rpc服务
    AccountService_::RegisterResponse register_response;
    AccountService_::AccountServiceRpc_Stub account_stub(new MyRpcChannel());
    MyRpcController controller;

    account_stub.Register(&controller, &request, &register_response, nullptr);
    if (controller.Failed())
    {
        LOG_ERROR << "Register RPC call failed: " << controller.ErrorText();
        return;
    }

    // 4. 处理返回结果
    Interface_::UnifiedMessage response;
    response.set_id(register_response.id());
    response.set_type("Register");
    response.set_is_success(register_response.is_success());
    response.set_msg(register_response.msg());

    // 序列化
    std::string send_str = response.SerializeAsString();
    if (register_response.is_success())
    {
        LOG_INFO << "Register success";
        LOG_INFO << "id: " << register_response.id();
    }
    else
    {
        LOG_ERROR << "Register failed: " << register_response.msg();
    }

    // 5. 发送响应给客户端
    conn->send(send_str);
}

// 处理登录业务
void InterfaceService::Login(const muduo::net::TcpConnectionPtr &conn, std::string &recv_str, muduo::Timestamp time)
{
    // 1. 检查接收字符串是否为空
    if (recv_str.empty())
    {
        LOG_ERROR << "Received empty message from client.";
        return;
    }

    // 1.1 反序列化recv_str
    Interface_::UnifiedMessage login_request;
    if (!login_request.ParseFromString(recv_str))
    {
        LOG_ERROR << "Failed to parse login request from string: " << recv_str;
        return;
    }

    // 1.2 验证请求内容
    if (login_request.id() == 0 || login_request.password().empty())
    {
        LOG_ERROR << "Login request missing id or password.";
        return;
    }

    // 2. 构造rpc请求
    AccountService_::LoginRequest request;
    request.set_id(login_request.id());
    request.set_password(login_request.password());
    request.set_msg("login");

    // 3. 调用rpc服务
    AccountService_::LoginResponse login_response;
    AccountService_::AccountServiceRpc_Stub account_stub(new MyRpcChannel());
    MyRpcController controller;

    account_stub.Login(&controller, &request, &login_response, nullptr);
    if (controller.Failed())
    {
        LOG_ERROR << "Login RPC call failed: " << controller.ErrorText();
        return;
    }

    // 4. 构建统一的消息响应
    Interface_::UnifiedMessage response;
    response.set_id(login_response.id());
    response.set_name(login_response.name());
    response.set_type("Login");
    response.set_is_success(login_response.is_success());
    response.set_msg(login_response.msg());

    // 5. 序列化
    std::string send_str = response.SerializeAsString();

    // 6. 处理返回结果
    if (login_response.is_success())
    {
        // 登录成功
        LOG_INFO << "Login success for user id: " << login_response.id();

        // 保存用户连接，保证线程安全
        {
            std::lock_guard<std::mutex> lock(_connMutex);
            _userConnMap[login_response.id()] = conn;
        }

        // Redis 订阅用户 id，接收用户的消息
        _redis.RedisSubscribe(login_response.id());
    }
    else
    {
        LOG_ERROR << "Login failed: " << login_response.msg();
    }

    // 发送响应给客户端
    conn->send(send_str);
}

// 注销业务
void InterfaceService::Logout(const muduo::net::TcpConnectionPtr &conn, std::string &recv_str, muduo::Timestamp time)
{
    // 1.反序列化recv_str
    Interface_::UnifiedMessage logout_msg;
    logout_msg.ParseFromString(recv_str);
    int id = logout_msg.id();
    // 1.判断用户是否在线
    {
        std::lock_guard<std::mutex> lock(_connMutex);
        auto it = _userConnMap.find(id);
        if (it != _userConnMap.end())
        {
            // 1.1用户在线，删除用户连接
            _userConnMap.erase(it);
        }
    }
    // 2.取消redis订阅
    _redis.RedisUnsubscribe(id);

    // 2.更新用户状态为离线
    User user;
    user.SetId(id);
    user.SetState("offline");
    _userService.UpdateState(user);
}


void InterfaceService::Chat(const muduo::net::TcpConnectionPtr &conn, std::string &recv_str, muduo::Timestamp time)
{
    // 1.反序列化recv_str
    Interface_::UnifiedMessage chat_msg;
    chat_msg.ParseFromString(recv_str);
    int to_id = chat_msg.to_user_id();
    // 1.判断对方是否在线
    {
        std::lock_guard<std::mutex> lock(_connMutex);
        // 2.先在在线用户列表中查找（同一个服务器上的用户）
        auto it = _userConnMap.find(to_id);
        if (it != _userConnMap.end())
        {
            // 2.1对方在线，直接转发消息
            it->second->send(recv_str);
            return;
        }
    }
    // 3.对方不在在线用户表中，再去数据库中查找（可能是在另一个服务器上的用户）
    User user = _userService.Query(to_id);
    if(user.GetState() == "online") {
        _redis.RedisPublish(to_id, recv_str);
        return;
    }

    // 4.如果对方不在线，存储离线消息
    _offlineMsg.OfflineMsgInsert(to_id, recv_str);
}

// 处理添加好友业务
void InterfaceService::AddFriend(const muduo::net::TcpConnectionPtr &conn, std::string &recv_buf, muduo::Timestamp time)
{
    // 1. 反序列化recv_buf
    Interface_::UnifiedMessage add_request;
    add_request.ParseFromString(recv_buf);
    // 1.1 构造rpc请求
    FriendService_::AddFriendRequest request;
    request.set_myid(add_request.id());
    request.set_friendid(add_request.friendid());

    // 2. 调用rpc服务
    FriendService_::AddFriendResponse add_response;
    FriendService_::FriendServiceRpc_Stub friend_stub(new MyRpcChannel());
    MyRpcController controller;
    friend_stub.AddFriend(&controller, &request, &add_response, nullptr);
    if (controller.Failed())
    {
        LOG_ERROR << "add friend rpc call failed: " << controller.ErrorText();
        return;
    }
    // 3. 序列化并发送响应给客户端
    Interface_::UnifiedMessage response;
    response.set_friendid(add_response.friendid());
    response.set_type("AddFriend");
    response.set_is_success(add_response.is_success());
    response.set_msg(add_response.msg());
    string send_str = response.SerializeAsString();

    if (add_response.is_success())
    {
        LOG_INFO << "Add friend success";
        conn->send(send_str);
    }
    else
    {
        LOG_ERROR << "Add friend failed: " << add_response.msg();
        conn->send(send_str);
    }
}

// 处理获取好友列表业务
void InterfaceService::GetFriendList(const muduo::net::TcpConnectionPtr &conn, std::string &recv_buf, muduo::Timestamp time)
{
    // 1. 反序列化recv_buf
    Interface_::UnifiedMessage friend_request;
    friend_request.ParseFromString(recv_buf);
    // 1.1 构造rpc请求
    FriendService_::FriendListRequest request;
    request.set_id(friend_request.id());

    // 2. 调用rpc服务
    FriendService_::FriendListResponse friend_response;
    FriendService_::FriendServiceRpc_Stub friend_stub(new MyRpcChannel());
    MyRpcController controller;
    friend_stub.GetFriendList(&controller, &request, &friend_response, nullptr);
    if (controller.Failed())
    {
        LOG_ERROR << "get friend list rpc call failed: " << controller.ErrorText();
        return;
    }
    // 3. 序列化并发送响应给客户端
    Interface_::UnifiedMessage response;
    response.set_is_success(friend_response.is_success());
    response.set_type("GetFriendList");
    response.set_msg(friend_response.msg());
    if (friend_response.friends_size() > 0)
    {
        for (int i = 0; i < friend_response.friends_size(); ++i)
        {
            Interface_::UnifiedMessage::FriendInfo *friend_info = response.add_friends_info();
            friend_info->set_id(friend_response.friends(i).id());
            friend_info->set_name(friend_response.friends(i).name());
            friend_info->set_state(friend_response.friends(i).state());
        }
        string send_str = response.SerializeAsString();
        LOG_INFO << "Get friend list success";
        conn->send(send_str);

    }
    else
    {
        string send_str = response.SerializeAsString();
        LOG_ERROR << "Get friend list failed";
        conn->send(send_str);
    }
}

// 处理创建群组业务
void InterfaceService::CreateGroup(const muduo::net::TcpConnectionPtr &conn, std::string &recv_buf, muduo::Timestamp time)
{
    // 1. 反序列化recv_buf
    Interface_::UnifiedMessage create_request;
    create_request.ParseFromString(recv_buf);
    // 1.1 构造rpc请求
    GroupService_::CreateGroupRequest request;
    request.set_group_name(create_request.group_name());
    request.set_group_desc(create_request.group_desc());
    request.set_userid(create_request.id());

    // 2. 调用rpc服务
    GroupService_::CreateGroupResponse create_response;
    GroupService_::GroupServiceRpc_Stub group_stub(new MyRpcChannel());
    MyRpcController controller;
    group_stub.CreateGroup(&controller, &request, &create_response, nullptr);
    if (controller.Failed())
    {
        LOG_ERROR << "create group rpc call failed: " << controller.ErrorText();
        return;
    }
    // 3. 序列化并发送响应给客户端
    Interface_::UnifiedMessage response;
    response.set_group_id(create_response.group_id());
    response.set_type("CreateGroup");
    response.set_is_success(create_response.success());
    response.set_msg(create_response.msg());
    string send_str = response.SerializeAsString();
    if (create_response.success())
    {
        LOG_INFO << "Create group success";
        LOG_INFO << "group id: " << create_response.group_id();
        conn->send(send_str);
    }
    else
    {
        LOG_ERROR << "Create group failed: " << create_response.msg();
        conn->send(send_str);
    }

}
// 处理加入群组业务
void InterfaceService::AddGroup(const muduo::net::TcpConnectionPtr &conn, std::string &recv_buf, muduo::Timestamp time)
{
    // 1. 反序列化recv_buf
    Interface_::UnifiedMessage add_request;
    add_request.ParseFromString(recv_buf);
    // 1.1 构造rpc请求
    GroupService_::AddGroupRequest request;
    request.set_userid(add_request.id());
    request.set_group_id(add_request.group_id());
    request.set_role("normal");
    
    // 2. 调用rpc服务
    GroupService_::AddGroupResponse add_response;
    GroupService_::GroupServiceRpc_Stub group_stub(new MyRpcChannel());
    MyRpcController controller;
    group_stub.AddGroup(&controller, &request, &add_response, nullptr);
    if (controller.Failed())
    {
        LOG_ERROR << "add group rpc call failed: " << controller.ErrorText();
        return;
    }
    // 3. 序列化并发送响应给客户端
    Interface_::UnifiedMessage response;
    response.set_group_id(add_response.group_id());
    response.set_type("AddGroup");
    response.set_is_success(add_response.success());
    response.set_msg(add_response.msg());
    string send_str = response.SerializeAsString();
    if (add_response.success())
    {
        LOG_INFO << "Add group success";
        conn->send(send_str);
    }
    else
    {
        LOG_ERROR << "Add group failed: " << add_response.msg();
        conn->send(send_str);
    }

    // 4.序列化并发送响应给客户端
    // string send_str = add_response.SerializeAsString();
    // conn->send(send_str);
}

// 处理获取群组列表业务
void InterfaceService::GroupList(const muduo::net::TcpConnectionPtr &conn, std::string &recv_buf, muduo::Timestamp time)
{
    // 1. 反序列化recv_buf，检查反序列化是否成功
    Interface_::UnifiedMessage group_request;
    if (!group_request.ParseFromString(recv_buf))
    {
        LOG_ERROR << "Failed to parse request buffer";
        Interface_::UnifiedMessage response;
        response.set_type("GetGroupList");
        response.set_is_success(false);
        response.set_msg("Invalid request format");
        conn->send(response.SerializeAsString());
        return;
    }

    // 1.1 构造rpc请求
    GroupService_::GroupListRequest request;
    request.set_userid(group_request.id());

    // 2. 调用rpc服务
    GroupService_::GroupListResponse group_response;
    GroupService_::GroupServiceRpc_Stub group_stub(new MyRpcChannel());
    MyRpcController controller;
    group_stub.GroupList(&controller, &request, &group_response, nullptr);

    // 3. 处理RPC调用失败
    if (controller.Failed())
    {
        LOG_ERROR << "group list rpc call failed: " << controller.ErrorText();
        Interface_::UnifiedMessage response;
        response.set_type("GetGroupList");
        response.set_is_success(false);
        response.set_msg("RPC call failed: " + controller.ErrorText());
        conn->send(response.SerializeAsString());
        return;
    }

    // 4. 构造返回信息
    Interface_::UnifiedMessage response;
    response.set_type("GetGroupList");
    response.set_is_success(group_response.success());
    response.set_msg(group_response.msg().data());  // 确保 msg 类型匹配

    // 4.1 如果RPC调用成功，填充群组信息
    if (group_response.success())
    {
        for (int i = 0; i < group_response.groups_size(); ++i)
        {
            const GroupService_::GroupInfo &group = group_response.groups(i);
            Interface_::UnifiedMessage::GroupInfo *group_info = response.add_groups();
            group_info->set_group_id(group.group_id());
            group_info->set_group_name(group.group_name().data());  // 处理bytes类型为string
            group_info->set_group_desc(group.group_desc().data());

            for (int j = 0; j < group.users_size(); ++j)
            {
                const GroupService_::UserInfo &user = group.users(j);
                Interface_::UnifiedMessage::UserInfo *user_info = group_info->add_users();
                user_info->set_id(user.id());
                user_info->set_name(user.name().data());  // 处理bytes类型为string
                user_info->set_state(user.state().data());
                user_info->set_role(user.role().data());
            }
        }
        LOG_INFO << "Get group list success";
    }
    else
    {
        LOG_ERROR << "Get group list failed: " << group_response.msg();
    }

    // 5. 统一发送响应
    conn->send(response.SerializeAsString());
}

// 处理获取群组用户id列表业务,同时转发消息
void InterfaceService::GetGroupUsers(const muduo::net::TcpConnectionPtr &conn, std::string &recv_buf, muduo::Timestamp time)
{
    // 1. 反序列化recv_buf
    Interface_::UnifiedMessage group_request;
    group_request.ParseFromString(recv_buf);
    // 1.1 构造rpc请求
    GroupService_::GetGroupUsersRequest request;
    request.set_group_id(group_request.group_id());
    request.set_userid(group_request.id());


    // 2. 调用rpc服务
    GroupService_::GetGroupUsersResponse group_response;
    GroupService_::GroupServiceRpc_Stub group_stub(new MyRpcChannel());
    MyRpcController controller;
    group_stub.GetGroupUsers(&controller, &request, &group_response, nullptr);
    if (controller.Failed())
    {
        LOG_ERROR << "group users rpc call failed: " << controller.ErrorText();
        return;
    }
    // 3. 处理返回结果
    if (group_response.success())
    {
        LOG_INFO << "Get group users success";
        lock_guard<mutex> lock(_connMutex);
        for (int i = 0; i < group_response.users_size(); ++i)
        {
            const GroupService_::UserId &user_info = group_response.users(i);
            LOG_INFO << "user id: " << user_info.id();
            auto it = _userConnMap.find(user_info.id());
            if (it != _userConnMap.end())
            {
                // 3.1对方在线，直接转发消息
                it->second->send(recv_buf);
            }
            else
            {
                User user = _userService.Query(user_info.id());
                if(user.GetState() == "online")
                {
                    // 3.2对方在线，但不在本服务器上，通过redis转发消息
                    _redis.RedisPublish(user_info.id(), recv_buf);
                }
                else
                {
                    // 3.3对方不在线，存储离线消息
                    _offlineMsg.OfflineMsgInsert(user_info.id(), group_request.msg());
                }
            }
        }
    }
    else
    {
        LOG_ERROR << "Get group users failed";
    }
}

// redis上报消息的回调函数
void InterfaceService::RedisSubscribeCallback(int channel, std::string message)
{
    // 查询用户是否在线
    lock_guard<mutex> lock(_connMutex);
    auto it = _userConnMap.find(channel);
    if (it != _userConnMap.end())
    {
        // 用户在线，直接转发消息
        it->second->send(message);
    }
    else
    {
        // 用户不在线，存储离线消息(可能在发送订阅消息过来的时候，用户下线了)
        _offlineMsg.OfflineMsgInsert(channel, message);
    }   
}