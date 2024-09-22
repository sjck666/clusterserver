#include <iostream>
#include <string>
#include "myrpc/MyRpcApplication.h"
#include "myrpc/MyRpcProvider.h"
#include "AccountService.h"

int main(int argc, char **argv) {
    MyRpcApplication::Instance()->Init(argc, argv);
    MyRpcProvider rpcProvider;
    // 9 启动rpc服务提供者，发布rpc方法
    rpcProvider.NotifyService(new AccountService());  
    std::cout << "AccountService start" << std::endl;

    rpcProvider.Run();


    return 0;   
}