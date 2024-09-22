#pragma once
#include <mysql/mysql.h>
#include <myrpc/MyRpcChannel.h>
#include <google/protobuf/service.h>
#include <string>
#include "UserService.h"
#include "User.h"
#include "AccountService.pb.h"
#include "OfflineMsg.h"
class AccountService : public AccountService_::AccountServiceRpc
{
public:
    void Login(::google::protobuf::RpcController *controller,
               const ::AccountService_::LoginRequest *request,
               ::AccountService_::LoginResponse *response,
               ::google::protobuf::Closure *done);
    void Register(::google::protobuf::RpcController *controller,
                const ::AccountService_::RegisterRequest *request,
                ::AccountService_::RegisterResponse *response,
                ::google::protobuf::Closure *done);
    // void Logout(::google::protobuf::RpcController *controller,
    //               const ::AccountService_::LogoutRequest *request,
    //               ::AccountService_::LogoutRespons *response,
    //               ::google::protobuf::Closure *done);

private:
    // 数据操作类对象
    UserService _userService;
    // 离线消息对象
    OfflineMsg _offlineMsg;
};