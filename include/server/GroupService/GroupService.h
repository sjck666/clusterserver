#pragma once
#include <mysql/mysql.h>
#include <myrpc/MyRpcChannel.h>
#include <google/protobuf/service.h>
#include <string>
#include "UserService.h"
#include "User.h"
#include "GroupService.pb.h"
#include "OfflineMsg.h"
class GroupService : public GroupService_::GroupServiceRpc
{
public:
    // 加入群组
    void AddGroup(::google::protobuf::RpcController *controller,
               const ::GroupService_::AddGroupRequest *request,
               ::GroupService_::AddGroupResponse *response,
               ::google::protobuf::Closure *done);
    // 创建群组
    void CreateGroup(::google::protobuf::RpcController *controller,
                const ::GroupService_::CreateGroupRequest *request,
                ::GroupService_::CreateGroupResponse *response,
                ::google::protobuf::Closure *done);
    // 获取用户加入的所有群组
    void GroupList(::google::protobuf::RpcController *controller,
                  const ::GroupService_::GroupListRequest *request,
                  ::GroupService_::GroupListResponse *response,
                  ::google::protobuf::Closure *done);
    // 获取群组所有用户id, 用于转发群组消息
    void GetGroupUsers(::google::protobuf::RpcController *controller,
                  const ::GroupService_::GetGroupUsersRequest *request,
                  ::GroupService_::GetGroupUsersResponse *response,
                  ::google::protobuf::Closure *done);

private:
    // 数据操作类对象
    UserService _userService;
    // 离线消息对象
    OfflineMsg _offlineMsg;
};