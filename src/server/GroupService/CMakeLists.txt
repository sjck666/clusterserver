set(SRC_LIST GroupService.cpp  
                        ../db/MySQL.cpp ../proto/GroupService.pb.cc 
                        ../user/UserService.cpp main.cpp ../offlinemsg/OfflineMsg.cpp ../redis/Redis.cpp)
# 生成可执行文件
add_executable(group ${SRC_LIST})
# 链接库文件
find_package(Protobuf REQUIRED)
# 链接库
target_link_libraries(group MyRpc ${Protobuf_LIBRARY} muduo_net muduo_base mysqlclient  zookeeper_mt hiredis pthread )
