
#include "AccountService.h"
using namespace std;
// 注册业务
void AccountService::Register(::google::protobuf::RpcController *controller,
                const ::AccountService_::RegisterRequest *request,
                ::AccountService_::RegisterResponse *response,
                ::google::protobuf::Closure *done)
{
    const std::string &name = request->name();
    const std::string &password = request->password();

    User user;
    user.SetName(name);
    user.SetPwd(password);
    bool state = _userService.Insert(user);

    if (state)
    {
        response->set_is_success(true);
        response->set_id(user.GetId());
        // std::cout << "name: " << name << " password: " << password << std::endl;
    }
    else
    {
        response->set_is_success(false);
        response->set_msg("AccountService -- Register service failed");
    }
    done->Run();

}
void AccountService::Login(::google::protobuf::RpcController *controller,
            const ::AccountService_::LoginRequest *request,
            ::AccountService_::LoginResponse *response,
            ::google::protobuf::Closure *done)
{
    const int id = request->id();
    const std::string &password = request->password();

    // 查询用户信息
    User user = _userService.Query(id);
    
    // 验证用户存在
    if (user.GetId() != id)
    {
        response->set_is_success(false);
        response->set_msg("AccountService -- Login service failed, user not exist");
        done->Run();
        return;
    }

    // 验证密码
    if (user.GetPwd() != password)
    {
        response->set_is_success(false);
        response->set_msg("AccountService -- Login service failed, password error");
        done->Run();
        return;
    }

    // 用户已存在且密码正确
    if (user.GetState() == "online")
    {
        response->set_is_success(false);
        response->set_msg("AccountService -- Login service failed, user already logged in");
        done->Run();
        return;
    }
    
    // 用户状态更新为在线
    user.SetState("online");
    _userService.UpdateState(user);

    // 登录成功，设置响应
    response->set_is_success(true);
    response->set_id(user.GetId());
    response->set_name(user.GetName());
    // 调用完成回调
    done->Run();
}