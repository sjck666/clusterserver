#include "Redis.h"

Redis::Redis() : _publishContext(nullptr), _subscribeContext(nullptr)
{

}
Redis::~Redis()
{
    if(_publishContext != nullptr)
    {
        redisFree(_publishContext);
        _publishContext = nullptr;
    }
    if(_subscribeContext != nullptr)
    {
        redisFree(_subscribeContext);
        _subscribeContext = nullptr;
    }
}

// 连接redis
bool Redis::RedisConnect()
{
    // 负责publish发布消息的redis上下文连接
    _publishContext = redisConnect("127.0.0.1", 6379);
    if(_publishContext == nullptr || _publishContext->err)
    {
        if(_publishContext)
        {
            printf("Error: %s\n", _publishContext->errstr);
        }
        else
        {
            printf("Can't allocate redis context\n");
        }
        return false;
    }
    // 负责subscribe订阅消息的redis上下文连接
    _subscribeContext = redisConnect("127.0.0.1", 6379);
    if(_subscribeContext == nullptr || _subscribeContext->err)
    {
        if(_subscribeContext)
        {
            printf("Error: %s\n", _subscribeContext->errstr);
        }
        else
        {
            printf("Can't allocate redis context\n");
        }
        return false;
    }

    // 创建一个线程，专门用来接收订阅消息
    std::thread t([&](){
        RedisHandleMessage();
    });
    t.detach();

    std::cout << "Redis connect success" << std::endl;
    return true;

}



// 消息订阅
bool Redis::RedisSubscribe(int channel)
{
    // SUBSCRIBE命令本身会造成线程阻塞等待通道里面发生消息，这里只做订阅通道，不接收通道消息
    // 通道消息的接收专门在redisHandleMessage线程中处理
    // 只负责发送命令，不阻塞接收redis server响应消息，否则和redisHandleMessage线程冲突
    if(REDIS_ERR == redisAppendCommand(_subscribeContext, "SUBSCRIBE %d", channel)) // 不直接执行命令，而是将命令放入缓冲区
    {
        printf("subscribe channel %d failed\n", channel);
        return false;
    }
    // redisBufferWrite可以循环发送缓冲区中的命令，直到发送完毕，返回REDIS_OK，（done置为1）
    int done = 0;
    while(!done)
    {
        if(REDIS_ERR == redisBufferWrite(this->_subscribeContext, &done))
        {
            printf("subscribe channel %d failed\n", channel);
            return false;
        }
    }
    // redisGetReply (这里不接收消息，让RedisHandleMessage线程接收)
    return true;

}

// 在指定的channel上发布消息
bool Redis::RedisPublish(int channel, std::string message)
{
    redisReply* reply = (redisReply*)redisCommand(_publishContext, "PUBLISH %d %s", channel, message.c_str());
    if(reply == nullptr)
    {
        printf("publish message failed\n");
        return false;
    }
    freeReplyObject(reply);
    return true;
}

// 在独立线程中接收订阅消息,循环阻塞等待消息
void Redis::RedisHandleMessage()
{
    redisReply* reply = nullptr;
    while(REDIS_OK == redisGetReply(this->_subscribeContext, (void**)&reply)) // 阻塞等待消息
    {
        // 订阅收到的消息是一个带三元素的数组
        if(reply != nullptr && reply->element[2] != nullptr && reply->element[2]->str != nullptr)
        {
            // 调用业务层的回调函数,给业务层上报通道上发生的消息
            _subscribeCallback(atoi(reply->element[1]->str), reply->element[2]->str);

        }
        freeReplyObject(reply);
    }
    std::cerr << ">>>>>>>>>>>>>>>> RedisHandleMessage exit" << std::endl;
}

// 取消订阅
bool Redis::RedisUnsubscribe(int channel)
{
    if(REDIS_ERR == redisAppendCommand(_subscribeContext, "UNSUBSCRIBE %d", channel))
    {
        printf("unsubscribe channel %d failed\n", channel);
        return false;
    }
    int done = 0;
    while(!done)
    {
        if(REDIS_ERR == redisBufferWrite(this->_subscribeContext, &done))
        {
            printf("unsubscribe channel %d failed\n", channel);
            return false;
        }
    }
    return true;
}

// 初始化向业务层上报消息的回调函数, callback业务层注册的回调函数
void Redis::InitSubscribeCallback(std::function<void(int, std::string)> callback)
{
   this->_subscribeCallback = callback;
}