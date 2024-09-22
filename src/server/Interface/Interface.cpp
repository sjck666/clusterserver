#include "Interface.h"
#include "Interface.pb.h"
#include "InterfaceService.h"
const size_t MAX_MSG_LENGTH = 1024 * 1024; // 1 MB
// 初始化服务器群接口类对象
Interface::Interface(muduo::net::EventLoop *loop,
                     const muduo::net::InetAddress &addr,
                     const std::string &name)
    : server_(loop, addr, name), loop_(loop)
{
    // 设置连接回调函数
    server_.setConnectionCallback(std::bind(&Interface::onConnection, this, std::placeholders::_1));
    // 设置消息回调函数
    server_.setMessageCallback(std::bind(&Interface::onMessage, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));

    // 设置线程数量
    server_.setThreadNum(4);
}

void Interface::start()
{
    // 启动服务器
    server_.start();
}

void Interface::onConnection(const muduo::net::TcpConnectionPtr &conn)
{
    // 客户端断开链接
    if (!conn->connected())
    {
        InterfaceService::GetInstance().ClientCloseException(conn);
        conn->shutdown();
    }
}

void Interface::onMessage(const muduo::net::TcpConnectionPtr &conn, muduo::net::Buffer *buf, muduo::Timestamp time)
{
    std::string msg = buf->retrieveAllAsString();
    std::cout << msg << std::endl;

    if (msg.empty())
    {
        LOG_WARN << "Received empty message.";
        return;
    }

    if (msg.size() > MAX_MSG_LENGTH)
    {
        LOG_WARN << "Message too long: " << msg.size();
        conn->shutdown();
        return;
    }

    Interface_::UnifiedMessage request;

    if (!request.ParseFromString(msg))
    {
        LOG_ERROR << "Failed to parse message: " << msg;
        conn->shutdown();
        return;
    }
    std::string msg_type = request.type();
    auto msg_handler = InterfaceService::GetInstance().GetHandler(msg_type);
    msg_handler(conn, msg, time);
}
