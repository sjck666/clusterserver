#pragma once
#include <hiredis/hiredis.h>
#include <thread>
#include <functional>
#include <iostream>
class Redis {
public:
    Redis();
    ~Redis();

    // 连接redis
    bool RedisConnect();

    // // 设置key-value
    // bool RedisSet(const char* key, const char* value);

    // // 获取key-value
    // bool RedisGet(const char* key, char* value);

    // // 删除key
    // bool RedisDel(const char* key);

    // // 设置key-value并设置过期时间
    // bool RedisSetex(const char* key, const char* value, int seconds);

    // // 设置key-value并设置过期时间
    // bool RedisSetex(const char* key, const char* value, int seconds, std::function<void()> callback);

    // 消息订阅
    bool RedisSubscribe(int channel);

    // 消息发布
    bool RedisPublish(int channel, std::string message);

    // 在独立线程中接收订阅消息
    void RedisHandleMessage();

    // 取消订阅
    bool RedisUnsubscribe(int channel);

    // 初始化向业务层上报消息的回调函数
    void InitSubscribeCallback(std::function<void(int, std::string)> callback);

private:
    // hredis上下文,负责publish消息
    redisContext* _publishContext;

    // hredis上下文,负责subscribe消息
    redisContext* _subscribeContext;

    // 订阅回调,负责处理订阅消息,收到订阅的消息，给service层上报(int channel, string message)，（先设置感兴趣的消息通道，再设置回调函数）
    std::function<void(int, std::string)> _subscribeCallback;

    // 订阅退出标志
    bool m_subscribeExit;

};