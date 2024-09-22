#include "GroupService.h"
using namespace std;
// 本地函数，用于添加用户到群组
bool addgroup(int groupid, int userid, std::string role)
{
    // 2. 组装sql语句
    char sql[1024] = {0};
    sprintf(sql, "insert into groupuser values(%d, %d, '%s')", groupid, userid, role.c_str());

    MySQL mysql;
    if (mysql.Connect())
    {
        if (mysql.Update(sql))
        {
            return true;
        }
        else
        {
            return false;
        }
    }
    else
    {
        return false;
    }
}

// 加入群组
void GroupService::AddGroup(::google::protobuf::RpcController *controller,
                            const ::GroupService_::AddGroupRequest *request,
                            ::GroupService_::AddGroupResponse *response,
                            ::google::protobuf::Closure *done)
{
    // 1. 获取请求参数
    int userid = request->userid();
    int groupid = request->group_id();
    std::string role = request->role();

    // 2. 添加用户到群组
    if (addgroup(groupid, userid, role))
    {
        response->set_success(true);
    }
    else
    {
        response->set_success(false);
        response->set_msg("GroupService -- AddGroup service addgroup failed");
    }
    done->Run();
}

// 创建群组
void GroupService::CreateGroup(::google::protobuf::RpcController *controller,
                               const ::GroupService_::CreateGroupRequest *request,
                               ::GroupService_::CreateGroupResponse *response,
                               ::google::protobuf::Closure *done)
{
    // 1. 获取请求参数
    std::string groupname = request->group_name();
    std::string groupdesc = request->group_desc();
    int userid = request->userid();

    // 2. 组装sql语句
    char sql[1024] = {0};
    sprintf(sql, "insert into allgroup(groupname, groupdesc) values('%s', '%s')", groupname.c_str(), groupdesc.c_str());

    MySQL mysql;
    if (mysql.Connect())
    {
        if (mysql.Update(sql))
        {
            response->set_success(true);
            // 获取刚刚插入的群组id
            response->set_group_id(mysql_insert_id(mysql.GetConnection()));
            // 将创建者添加到群组中
            addgroup(response->group_id(), userid, "creator");
        }
        else
        {
            response->set_success(false);
            response->set_msg("GroupService -- CreateGroup service update mysql failed");
        }
    }
    else
    {
        response->set_success(false);
        response->set_msg("GroupService -- CreateGroup service connect mysql failed");
    }
    done->Run();
}

void GroupService::GroupList(::google::protobuf::RpcController *controller,
                             const ::GroupService_::GroupListRequest *request,
                             ::GroupService_::GroupListResponse *response,
                             ::google::protobuf::Closure *done)
{
    int userid = request->userid();
    char sql[1024] = {0};
    // 组装SQL语句
    sprintf(sql, "SELECT a.id AS groupid, a.groupname, a.groupdesc FROM allgroup a "
                 "INNER JOIN groupuser b ON a.id = b.groupid WHERE b.userid = %d",
            userid);

    MySQL mysql;
    if (!mysql.Connect())
    {
        response->set_success(false);
        response->set_msg("GroupService -- GroupList service connect mysql failed");
        done->Run();
        return;
    }

    MYSQL_RES *res = mysql.Query(sql);
    if (res == nullptr)
    {
        response->set_success(false);
        response->set_msg("GroupService -- GroupList service query mysql failed");
        done->Run();
        return;
    }

    MYSQL_ROW row;
    while ((row = mysql_fetch_row(res)) != nullptr)
    {
        GroupService_::GroupInfo *group_info = response->add_groups();
        group_info->set_group_id(atoi(row[0]));
        group_info->set_group_name(row[1] ? row[1] : ""); // 处理可能为 nullptr 的情况
        group_info->set_group_desc(row[2] ? row[2] : "");

        // 查询群组中的所有用户信息
        sprintf(sql, "SELECT a.id, a.name, a.state, b.grouprole FROM user a "
                     "INNER JOIN groupuser b ON a.id = b.userid WHERE b.groupid = %d",
                atoi(row[0]));
        MYSQL_RES *res2 = mysql.Query(sql);
        if (res2 != nullptr)
        {
            MYSQL_ROW row2;
            while ((row2 = mysql_fetch_row(res2)) != nullptr)
            {
                GroupService_::UserInfo *group_user_info = group_info->add_users();
                group_user_info->set_id(atoi(row2[0]));
                group_user_info->set_name(row2[1] ? row2[1] : "");
                group_user_info->set_state(row2[2] ? row2[2] : "");
                group_user_info->set_role(row2[3] ? row2[3] : "");
            }
            mysql_free_result(res2);
        }
        else
        {
            response->set_success(false);
            response->set_msg("GroupService -- GroupList service query group_user_info mysql failed");
            mysql_free_result(res);
            done->Run();
            return;
        }
    }
    mysql_free_result(res);
    response->set_success(true);
    done->Run();
}

// 获取群组所有用户的id, 用于转发群组消息
void GroupService::GetGroupUsers(::google::protobuf::RpcController *controller,
                                 const ::GroupService_::GetGroupUsersRequest *request,
                                 ::GroupService_::GetGroupUsersResponse *response,
                                 ::google::protobuf::Closure *done)
{
    // 1. 获取请求参数
    int groupid = request->group_id();
    int userid = request->userid();
    // 2. 组装sql语句
    char sql[1024] = {0};
    sprintf(sql, "select userid from groupuser where groupid=%d and userid!=%d", groupid, userid);

    // 3. 数据库操作
    MySQL mysql;
    if (mysql.Connect())
    {
        MYSQL_RES *res = mysql.Query(sql);
        if (res != nullptr)
        {
            MYSQL_ROW row;
            while ((row = mysql_fetch_row(res)) != nullptr)
            {
                GroupService_::UserId *user_id = response->add_users();
                user_id->set_id(atoi(row[0]));
            }
            mysql_free_result(res);
            response->set_success(true);
        }
        else
        {
            response->set_success(false);
            response->set_msg("GroupService -- GetGroupUsers service query mysql failed");
        }
    }
    else
    {
        response->set_success(false);
        response->set_msg("GroupService -- GetGroupUsers service connect mysql failed");
    }
    done->Run();
}