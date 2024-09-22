#include <myrpc/MyRpcApplication.h>
#include <myrpc/MyRpcConf.h>
#include "Interface.h"
#include "InterfaceService.h"
void ResetHandler(int signum) {
    InterfaceService::GetInstance().Reset();
    exit(0);
}

int main(int argc, char **argv)
{
    std::cout << "Starting application..." << std::endl;

    MyRpcApplication::Instance()->Init(argc, argv);
    std::cout << "Application initialized." << std::endl;

    MyRpcConf configure = MyRpcApplication::Instance()->GetConf();
    
    std::string ip = configure.Load("rpcserverip");
    std::cout << "Loaded IP: " << ip << std::endl;

    std::string portStr = configure.Load("rpcserverport");
    std::cout << "Loaded port: " << portStr << std::endl;
    int port = atoi(portStr.c_str());
    
    if (port == 0) {
        std::cerr << "Error: Invalid port number." << std::endl;
        return 1;
    }

    signal(SIGINT, ResetHandler);

    muduo::net::EventLoop loop;
    muduo::net::InetAddress addr(ip, port);
    
    Interface server(&loop, addr, "Interface");
    std::cout << "Server created with address: " << ip << ":" << port << std::endl;

    server.start();
    std::cout << "Server started." << std::endl;

    loop.loop();
    std::cout << "Event loop started." << std::endl;

    return 0;
}