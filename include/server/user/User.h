#pragma once
#include <string>

class User
{
public:
    User(int id = -1, std::string name = "", std::string pwd = "", std::string state = "offline")
    {
        this->id_ = id;
        this->name_ = name;
        this->password_ = pwd;
        this->state = state;
    }
    void SetId(int id) { this->id_ = id; }
    void SetName(std::string name) { this->name_ = name; }
    void SetPwd(std::string pwd) { this->password_ = pwd; }
    void SetState(std::string state) { this->state = state; }

    int GetId() { return this->id_; }
    std::string GetName() { return this->name_; }
    std::string GetPwd() { return this->password_; }
    std::string GetState() { return this->state; }

private:
    std::string name_;
    std::string password_;
    int id_;
    std::string state;
};