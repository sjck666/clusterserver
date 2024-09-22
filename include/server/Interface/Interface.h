#pragma once
#include <string>
#include <muduo/net/Buffer.h>
#include <muduo/net/EventLoop.h>
#include <muduo/net/TcpServer.h>
#include <muduo/net/TcpConnection.h>
#include <muduo/base/Timestamp.h>
class Interface
{
public:
    // 初始化服务器群接口类对象
    Interface(muduo::net::EventLoop* loop, const muduo::net::InetAddress& addr, const std::string& name);

    void start();
private:
    void onMessage(const muduo::net::TcpConnectionPtr& conn, muduo::net::Buffer* buf, muduo::Timestamp time);
    void onConnection(const muduo::net::TcpConnectionPtr& conn);

    muduo::net::EventLoop* loop_;
    muduo::net::TcpServer server_;
};
