#include <mysql/mysql.h>
#include <myrpc/MyRpcChannel.h>
#include <google/protobuf/service.h>
#include <iostream>
#include <thread>
#include <string>
#include <vector>
#include <chrono>
#include <ctime>
#include <unordered_map>
#include <functional>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <semaphore.h>
#include <atomic>
#include "User.h"
#include "Interface.pb.h"
#include "OfflineMsg.h"
// 记录当前系统登录的用户信息
User _currentUser;
// 记录当前登录用户的好友列表信息
std::vector<User> _currentUserFriendList;
// 记录当前登录用户的群组列表信息
std::vector<Interface_::UnifiedMessage::GroupInfo> _currentUserGroupList;

// 控制主菜单页面程序
bool ChatPageFlag = false;

// 用于读写线程之间的通信
sem_t RWsem;
// 记录登录状态
std::atomic_bool isLogin(false);
// 记录注册状态
std::atomic_bool isRegister(false);

// 接收线程
void recvThread(int);
// 获取系统时间（聊天信息需要添加时间信息）
std::string getCurrentTime();
// 主聊天页面程序
void ChatPage(int);
// 显示好友列表
void showFriendList();
// 显示群组列表
void showGroupList();
// 登录成功后的处理
void DoLoginResponse(const Interface_::UnifiedMessage &);

// 聊天客户端程序实现，main线程用作发送线程，子线程用作接收线程
int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        std::cerr << "Usage: ./Client [ip] [port]" << std::endl;
        return -1;
    }

    // 解析命令行参数
    char *ip = argv[1];
    uint16_t port = atoi(argv[2]);

    // 创建socket
    int clientfd = socket(AF_INET, SOCK_STREAM, 0);
    if (clientfd < 0)
    {
        std::cerr << "socket error" << std::endl;
        exit(-1);
    }
    // 填写服务器地址信息
    struct sockaddr_in server;
    // 清空server
    memset(&server, 0, sizeof(server));
    server.sin_family = AF_INET;
    server.sin_port = htons(port);
    server.sin_addr.s_addr = inet_addr(ip);
    // 连接服务器
    if (connect(clientfd, (struct sockaddr *)&server, sizeof(server)) < 0)
    {
        std::cerr << "connect error" << std::endl;
        exit(-1);
    }

    // 初始化读写信号量
    sem_init(&RWsem, 0, 0);

    // 创建接收数据的子线程，用于接收服务器发送的数据
    std::thread recv(recvThread, clientfd);
    recv.detach();

    // 主线程用于接收用户输入，负责发送数据
    while (1)
    {
        // 显示首页面菜单 登录、注册、退出
        std::cout << "========================" << std::endl;
        std::cout << "1. Login" << std::endl;
        std::cout << "2. Register" << std::endl;
        std::cout << "3. Quit" << std::endl;
        std::cout << "========================" << std::endl;
        std::cout << "Please Select: ";
        int select = 0;
        std::cin >> select;

        // 检查 std::cin 是否进入错误状态。这通常会发生在输入的类型不匹配时，比如输入了非整数值。
        if (std::cin.fail())
        {
            std::cin.clear();                                                   // 清除 std::cin 的错误标志，使其可以继续接收输入
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n'); // 丢弃输入流中剩余的字符直到换行符，以清空缓冲区中的多余数据。
            std::cout << "Input Error, Please Input Again!" << std::endl;
            continue;
        }

        // 在读取 select 后调用 std::cin.ignore() 来清空缓冲区中的多余字符（如换行符），以准备接下来的输入操作。
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

        switch (select)
        {
        case 1:
        {
            // 登录
            int id = 0;
            char pwd[50] = {0};
            std::cout << "Please Input Your ID: ";
            std::cin >> id;
            std::cin.get();
            std::cout << "Please Input Your Password: ";
            std::cin.getline(pwd, 50);
            // 封装登录请求
            Interface_::UnifiedMessage request;
            request.set_id(id);
            request.set_password(pwd);
            request.set_type("Login");
            std::string requestStr;
            request.SerializeToString(&requestStr);

            isLogin = false;
            // 发送登录请求
            int ret = send(clientfd, requestStr.c_str(), requestStr.size(), 0);
            if (ret < 0)
            {
                std::cerr << "send login request error" << std::endl;
                break;
            }
            // 等待接收线程处理登录结果
            sem_wait(&RWsem);
            if (isLogin)
            {
                // 登录成功
                ChatPageFlag = true;
                // 进入聊天页面
                ChatPage(clientfd);
            }
            else
            {
                std::cout << "Login Failed, Please Check Your ID and Password!" << std::endl;
            }
            break;
        }
        case 2:
        {
            // 注册
            char name[50] = {0};
            char pwd[50] = {0};
            std::cout << "Please Input Your Name: ";
            std::cin.getline(name, 50);
            std::cout << "Please Input Your Password: ";
            std::cin.getline(pwd, 50);
            // 封装注册请求
            Interface_::UnifiedMessage request;
            request.set_name(name);
            request.set_password(pwd);
            request.set_type("Register");
            std::string requestStr;
            request.SerializeToString(&requestStr);
            // 发送注册请求
            int ret = send(clientfd, requestStr.c_str(), requestStr.size(), 0);
            if (ret < 0)
            {
                std::cerr << "send register request error" << std::endl;
                break;
            }
            // 等待接收线程处理注册结果
            sem_wait(&RWsem);

            break;
        }
        case 3:
        {
            // 退出
            close(clientfd);
            sem_destroy(&RWsem);
            exit(0);
            break;
        }
        default:
        {
            std::cout << "Input Error, Please Input Again!" << std::endl;
            break;
        }
        }
    }
    return 0;
}

void recvThread(int clientfd)
{
    while (true)
    {
        char buffer[1024] = {0};
        int ret = recv(clientfd, buffer, sizeof(buffer), 0);

        if (ret <= 0)
        {
            close(clientfd);
            std::cerr << "recv error" << std::endl;
            return; // 退出线程而不是调用 exit(-1)
        }

        // 注意：需要根据实际协议处理数据长度和拼接
        std::string data(buffer, ret); // 使用接收到的数据创建一个 string 对象
        Interface_::UnifiedMessage response;
        response.ParseFromString(data);
        // 处理不同的消息类型
        if (response.type() == "Login")
        {
            DoLoginResponse(response);
        }
        else if (response.type() == "Register")
        {
            if (response.is_success())
            {
                std::cout << "Register Success! Your ID is: " << response.id() << std::endl;
            }
            else
            {
                std::cout << "Register Failed: " << response.msg() << std::endl;
            }
            isRegister = true;
            sem_post(&RWsem);
        }
        else if (response.type() == "AddFriend")
        {
            if (response.is_success())
            {
                std::cout << "Add Friend Success!" << std::endl;
            }
            else
            {
                std::cout << "Add Friend Failed: " << response.msg() << std::endl;
            }
        }
        else if (response.type() == "GetFriendList")
        {
            // 接收好友列表信息
            _currentUserFriendList.clear();
            for (int i = 0; i < response.friends_info_size(); i++)
            {
                Interface_::UnifiedMessage::FriendInfo friend_info = response.friends_info(i);
                User user;
                user.SetId(friend_info.id());
                user.SetName(friend_info.name());
                user.SetState(friend_info.state());
                _currentUserFriendList.push_back(user);
            }
            showFriendList();
        }
        else if (response.type() == "CreateGroup")
        {
            if (response.is_success())
            {
                std::cout << "Create Group Success! Group ID: " << response.group_id() << std::endl;
            }
            else
            {
                std::cout << "Create Group Failed: " << response.msg() << std::endl;
            }
        }
        else if (response.type() == "AddGroup")
        {
            if (response.is_success())
            {
                std::cout << "Add Group Success!" << std::endl;
            }
            else
            {
                std::cout << "Add Group Failed: " << response.msg() << std::endl;
            }
        }
        else if (response.type() == "GetGroupList")
        {
            // 检查响应是否成功
            if (!response.is_success())
            {
                std::cerr << "Failed to get group list: " << response.msg() << std::endl;
                return;
            }

            // 接收群组列表信息
            std::cout << "Received group list:" << std::endl;
            for (int i = 0; i < response.groups_size(); i++)
            {
                const Interface_::UnifiedMessage::GroupInfo &group_info = response.groups(i);
                int group_id = group_info.group_id();
                std::string group_name(group_info.group_name().data(), group_info.group_name().size());
                std::string group_desc(group_info.group_desc().data(), group_info.group_desc().size());

                std::cout << "Group ID: " << group_id
                          << " Name: " << group_name
                          << " Description: " << group_desc << std::endl;
            }
        }
        else if (response.type() == "GroupChat")
        {
            std::cout << "========================" << std::endl;
            std::cout << "From: " << response.from_user_id() << " " << response.user_name() << std::endl;
            std::cout << "Group ID: " << response.group_id() << std::endl;
            std::cout << "Time: " << response.time() << std::endl;
            std::cout << "Content: " << response.msg() << std::endl;
            std::cout << "========================" << std::endl;
        }
        else if (response.type() == "Chat")
        {
            std::cout << "========================" << std::endl;
            std::cout << "From: " << response.from_user_id() << std::endl;
            std::cout << "To: " << response.to_user_id() << std::endl;
            std::cout << "Time: " << response.time() << std::endl;
            std::cout << "Content: " << response.msg() << std::endl;
            std::cout << "========================" << std::endl;
        }
        else
        {
            std::cerr << "Unknown message format" << std::endl;
        }
    }
}

// 登录成功后的处理
void DoLoginResponse(const Interface_::UnifiedMessage &login_response)
{
    if (login_response.is_success() == true)
    {
        // 登录成功
        std::cout << "Login Success!" << std::endl;
        // 记录当前登录用户信息
        _currentUser.SetId(login_response.id());
        _currentUser.SetName(login_response.name());

        std::cout << "======================login user======================" << std::endl;
        std::cout << "login user => id:" << login_response.id() << " name:" << login_response.name() << std::endl;
        // 打印离线消息
        OfflineMsg offlineMsg;
        std::vector<std::string> msgs = offlineMsg.OfflineMsgQuery(login_response.id());
        for (auto &msg : msgs)
        {
            Interface_::UnifiedMessage offline_msg;
            offline_msg.ParseFromString(msg);
            if (offline_msg.type() == "Chat")
            {
                std::cout << "========================" << std::endl;
                std::cout << "From: " << "ID: " << offline_msg.from_user_id() << "  Name: " << offline_msg.user_name() << std::endl;
                std::cout << "Time: " << offline_msg.time() << std::endl;
                std::cout << "Content: " << offline_msg.msg() << std::endl;
                std::cout << "========================" << std::endl;
            }
            else if (offline_msg.type() == "GroupChat")
            {
                std::cout << "========================" << std::endl;
                std::cout << "From: " << "ID: " << offline_msg.from_user_id() << "  Name: " << offline_msg.user_name() << std::endl;
                std::cout << "Group ID: " << offline_msg.group_id() << std::endl;
                std::cout << "Time: " << offline_msg.time() << std::endl;
                std::cout << "Content: " << offline_msg.msg() << std::endl;
                std::cout << "========================" << std::endl;
            }
        }
        offlineMsg.OfflineMsgRemove(login_response.id());
        isLogin = true;
    }
    else
    {
        isLogin = false;
    }
    sem_post(&RWsem);
}

// show group list
void showGroupList()
{
    std::cout << "======================== Group List ========================" << std::endl;
    for (int i = 0; i < _currentUserGroupList.size(); i++)
    {
        std::cout << _currentUserGroupList[i].group_id() << " " << _currentUserGroupList[i].group_name() << _currentUserGroupList[i].group_desc() << std::endl;
        std::cout << "| User ID" << " |  User Name" << " |  User State" << " |  User Role" << std::endl;
        for (int j = 0; j < _currentUserGroupList[i].users_size(); j++)
        {
            std::cout << "| " << _currentUserGroupList[i].users(j).id() << " | " << _currentUserGroupList[i].users(j).name() << " | " << _currentUserGroupList[i].users(j).state() << " | " << _currentUserGroupList[i].users(j).role() << std::endl;
        }
    }
    std::cout << "======================== end ========================" << std::endl;
}
// 显示群组列表的函数

// show friend list
void showFriendList()
{
    std::cout << "======================== Friend List ========================" << std::endl;
    for (int i = 0; i < _currentUserFriendList.size(); i++)
    {
        std::cout << "ID: " << _currentUserFriendList[i].GetId() << " Name: " << _currentUserFriendList[i].GetName() << " State: " << _currentUserFriendList[i].GetState() << std::endl;
    }
    std::cout << "======================== end ========================" << std::endl;
}

// "help" command handler
void help(int fd = 0, std::string str = "");
// "chat" command handler
void chat(int, std::string);
// "addfriend" command handler
void addfriend(int, std::string);
// "creategroup" command handler
void creategroup(int, std::string);
// "addgroup" command handler
void addgroup(int, std::string);
// "groupchat" command handler
void groupchat(int, std::string);
// "loginout" command handler
void logout(int, std::string);

void getfriendlist(int, std::string);
void getgrouplist(int, std::string);

// 系统支持的客户端命令列表
std::unordered_map<std::string, std::string> commandMap = {
    {"======================", "=========================="},
    {"help", "  显示所有可以使用的命令 格式-- help"},
    {"chat", "  一对一聊天  格式-- chat:friendid:message"},
    {"addfriend", " 添加好友    格式-- addfriend:friendid"},
    {"creategroup", "   创建群组    格式-- creategroup:groupname:groupdesc"},
    {"addgroup", "  加入群组    格式-- addgroup:groupid"},
    {"groupchat", " 群聊        格式-- groupchat:groupid:message"},
    {"logout", "  注销        格式-- logout"},
    {"getfriendlist", " 获取好友列表  格式-- getfriendlist"},
    {"getgrouplist", " 获取群组列表  格式-- getgrouplist"},
    {"======================", "=========================="}};

// 注册系统支持的客户端命令处理函数,(int, std::string)分别表示clientfd和命令参数
std::unordered_map<std::string, std::function<void(int, std::string)>> commandHandlerMap = {
    {"help", help},
    {"chat", chat},
    {"addfriend", addfriend},
    {"creategroup", creategroup},
    {"addgroup", addgroup},
    {"groupchat", groupchat},
    {"logout", logout},
    {"getfriendlist", getfriendlist},
    {"getgrouplist", getgrouplist}};

void ChatPage(int clientfd)
{
    help();
    char buffer[1024] = {0};
    while (ChatPageFlag)
    {
        std::cin.getline(buffer, 1024);
        std::string commandbuf(buffer);
        std::string command; // 存储命令
        // 解析命令
        int idx = commandbuf.find(":");
        if (idx == -1)
        {
            command = commandbuf;
        }
        else
        {
            command = commandbuf.substr(0, idx);
        }
        auto it = commandHandlerMap.find(command);
        if (it != commandHandlerMap.end())
        {
            // 执行命令处理函数
            it->second(clientfd, commandbuf.substr(idx + 1, commandbuf.size() - idx));
        }
        else
        {
            std::cerr << "Invalid Command, Please Input Again!" << std::endl;
        }
    }
}

// "help" command handler
void help(int, std::string)
{
    std::cout << "show command list >>> " << std::endl;
    for (auto &p : commandMap)
    {
        std::cout << p.first << " : " << p.second << std::endl;
    }
    std::cout << std::endl;
}

// "addfriend" command handler
void addfriend(int clientfd, std::string str)
{
    int friendid = atoi(str.c_str());

    Interface_::UnifiedMessage request;
    request.set_id(_currentUser.GetId());
    request.set_friendid(friendid);
    request.set_type("AddFriend");
    std::string requestStr = request.SerializeAsString();

    int len = send(clientfd, requestStr.c_str(), requestStr.size(), 0);
    if (-1 == len)
    {
        std::cerr << "send addfriend msg error -> " << requestStr << std::endl;
    }
}
// "chat" command handler
void chat(int clientfd, std::string str)
{
    int idx = str.find(":"); // friendid:message
    if (-1 == idx)
    {
        std::cerr << "chat command invalid!" << std::endl;
        return;
    }

    int friendid = atoi(str.substr(0, idx).c_str());
    std::string message = str.substr(idx + 1, str.size() - idx);

    Interface_::UnifiedMessage chat_msg;
    chat_msg.set_from_user_id(_currentUser.GetId());
    chat_msg.set_user_name(_currentUser.GetName());
    chat_msg.set_to_user_id(friendid);
    chat_msg.set_msg(message);
    chat_msg.set_time(getCurrentTime());
    chat_msg.set_type("Chat");
    std::string buffer = chat_msg.SerializeAsString();

    int len = send(clientfd, buffer.c_str(), buffer.size(), 0);
    if (-1 == len)
    {
        std::cerr << "send chat msg error -> " << buffer << std::endl;
    }
}
// "creategroup" command handler  groupname:groupdesc
void creategroup(int clientfd, std::string str)
{
    int idx = str.find(":");
    if (-1 == idx)
    {
        std::cerr << "creategroup command invalid!" << std::endl;
        return;
    }

    std::string groupname = str.substr(0, idx);
    std::string groupdesc = str.substr(idx + 1, str.size() - idx);

    Interface_::UnifiedMessage request;
    request.set_id(_currentUser.GetId());
    request.set_group_name(groupname);
    request.set_group_desc(groupdesc);
    request.set_type("CreateGroup");
    std::string buffer = request.SerializeAsString();
    int len = send(clientfd, buffer.c_str(), buffer.size(), 0);
    if (-1 == len)
    {
        std::cerr << "send creategroup msg error -> " << buffer << std::endl;
    }
}
// "addgroup" command handler
void addgroup(int clientfd, std::string str)
{
    int groupid = atoi(str.c_str());
    std::cout << groupid << std::endl;

    Interface_::UnifiedMessage request;
    request.set_id(_currentUser.GetId());
    request.set_group_id(groupid);
    request.set_type("AddGroup");
    std::string buffer = request.SerializeAsString();

    int len = send(clientfd, buffer.c_str(), buffer.size(), 0);
    if (-1 == len)
    {
        std::cerr << "send addgroup msg error -> " << buffer << std::endl;
    }
}
// "groupchat" command handler   groupid:message
void groupchat(int clientfd, std::string str)
{
    int idx = str.find(":");
    if (-1 == idx)
    {
        std::cerr << "groupchat command invalid!" << std::endl;
        return;
    }

    int groupid = atoi(str.substr(0, idx).c_str());
    std::string message = str.substr(idx + 1, str.size() - idx);

    Interface_::UnifiedMessage group_msg;
    group_msg.set_from_user_id(_currentUser.GetId());
    group_msg.set_user_name(_currentUser.GetName());
    group_msg.set_group_id(groupid);
    group_msg.set_msg(message);
    group_msg.set_time(getCurrentTime());
    group_msg.set_type("GroupChat");
    std::string buffer = group_msg.SerializeAsString();

    int len = send(clientfd, buffer.c_str(), buffer.size(), 0);
    if (-1 == len)
    {
        std::cerr << "send groupchat msg error -> " << buffer << std::endl;
    }
}
// "loginout" command handler
void logout(int clientfd, std::string)
{
    Interface_::UnifiedMessage request;
    request.set_id(_currentUser.GetId());
    request.set_type("Logout");
    std::string buffer = request.SerializeAsString();

    int len = send(clientfd, buffer.c_str(), buffer.size(), 0);
    if (-1 == len)
    {
        std::cerr << "send logout msg error -> " << buffer << std::endl;
    }
    else
    {
        ChatPageFlag = false;
    }
}

void getfriendlist(int clientfd, std::string)
{
    Interface_::UnifiedMessage request;
    request.set_id(_currentUser.GetId());
    request.set_type("GetFriendList");
    std::string buffer = request.SerializeAsString();
    int len = send(clientfd, buffer.c_str(), buffer.size(), 0);
    if (-1 == len)
    {
        std::cerr << "send getfriendlist msg error -> " << buffer << std::endl;
    }
}

void getgrouplist(int clientfd, std::string args)
{
    Interface_::UnifiedMessage request;
    request.set_id(_currentUser.GetId());
    request.set_type("GetGroupList");

    std::string buffer = request.SerializeAsString();
    ssize_t len = send(clientfd, buffer.c_str(), buffer.size(), 0);

    if (len == -1)
    {
        std::cerr << "Error sending GetGroupList message: " << buffer
                  << " | Error: " << strerror(errno) << std::endl;
        return; // 直接返回以避免后续处理
    }

    if (len < static_cast<ssize_t>(buffer.size()))
    {
        std::cerr << "Partial send of GetGroupList message: sent " << len
                  << " bytes out of " << buffer.size() << " bytes." << std::endl;
    }
}

// 获取系统时间（聊天信息需要添加时间信息）
std::string getCurrentTime()
{
    auto tt = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    struct tm *ptm = localtime(&tt);
    char date[60] = {0};
    sprintf(date, "%d-%02d-%02d %02d:%02d:%02d",
            (int)ptm->tm_year + 1900, (int)ptm->tm_mon + 1, (int)ptm->tm_mday,
            (int)ptm->tm_hour, (int)ptm->tm_min, (int)ptm->tm_sec);
    return std::string(date);
}