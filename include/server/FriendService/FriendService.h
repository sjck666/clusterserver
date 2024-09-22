#pragma once
#include <mysql/mysql.h>
#include <myrpc/MyRpcChannel.h>
#include <google/protobuf/service.h>
#include <string>
#include "UserService.h"
#include "User.h"
#include "FriendService.pb.h"
#include "OfflineMsg.h"
class FriendService : public FriendService_::FriendServiceRpc
{
public:
    void AddFriend(::google::protobuf::RpcController *controller,
               const ::FriendService_::AddFriendRequest *request,
               ::FriendService_::AddFriendResponse *response,
               ::google::protobuf::Closure *done);
    void GetFriendList(::google::protobuf::RpcController *controller,
                const ::FriendService_::FriendListRequest *request,
                ::FriendService_::FriendListResponse *response,
                ::google::protobuf::Closure *done);

private:
    // 数据操作类对象
    UserService _userService;
    // 离线消息对象
    OfflineMsg _offlineMsg;
};