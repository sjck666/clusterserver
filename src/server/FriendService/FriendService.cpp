#include "FriendService.h"
// 添加好友
void FriendService::AddFriend(::google::protobuf::RpcController *controller,
            const ::FriendService_::AddFriendRequest *request,
            ::FriendService_::AddFriendResponse *response,
            ::google::protobuf::Closure *done)
{
    // 1. 获取请求参数
    int myid = request->myid();
    int friendid = request->friendid();
    // 2. 封装sql语句
    char sql[1024] = {0};
    sprintf(sql, "insert into friend values(%d, %d)", myid, friendid);

    MySQL mysql;
    if (mysql.Connect())
    {
        if (mysql.Update(sql))
        {
            response->set_is_success(true);
        }
        else
        {
            response->set_is_success(false);
            response->set_msg("FriendService -- AddFriend service update mysql failed");
        }
    }
    else
    {
        response->set_is_success(false);
        response->set_msg("FriendService -- AddFriend service connect mysql failed");
    }
    // 3. 返回响应
    done->Run();

}
// 获取好友列表
void FriendService::GetFriendList(::google::protobuf::RpcController *controller,
            const ::FriendService_::FriendListRequest *request,
            ::FriendService_::FriendListResponse *response,
            ::google::protobuf::Closure *done)
{
    // 1. 获取请求参数
    int userid = request->id();
    // 2. 封装sql语句
    char sql[1024] = {0};
    sprintf(sql, "select a.id,a.name,a.state from user a inner join friend b on b.friendid = a.id where b.userid=%d", userid);
    // vector<User> vec;
    MySQL mysql;
    if (mysql.Connect())
    {
        MYSQL_RES *res = mysql.Query(sql);
        if (res != nullptr)
        {
            // 把userid用户的所有离线消息放入vec中返回
            MYSQL_ROW row;
            while((row = mysql_fetch_row(res)) != nullptr)
            {
                FriendService_::FriendInfo *friend_info = response->add_friends();
                friend_info->set_id(atoi(row[0]));
                friend_info->set_name(row[1]);
                friend_info->set_state(row[2]);
            }
            mysql_free_result(res);
            response->set_is_success(true);

        }
    }
    else
    {
        response->set_is_success(false);
        response->set_msg("FriendService -- GetFriendList service connect mysql failed");
    }
    // 3. 返回响应  
    done->Run();
}