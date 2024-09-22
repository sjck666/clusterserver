# 查找当前目录下的所有源文件
aux_source_directory(. SRC_LIST)
aux_source_directory(./db DB_LIST)
aux_source_directory(./Interface INT_LIST)
aux_source_directory(./proto ACC_LIST)
aux_source_directory(./user USER_LIST)
aux_source_directory(./offlinemsg OFFLINE_LIST)
aux_source_directory(./redis REDIS_LIST)
# 生成可执行文件
add_executable(Server ${SRC_LIST} ${DB_LIST} ${INT_LIST} ${ACC_LIST} ${USER_LIST} ${OFFLINE_LIST} ${REDIS_LIST})
find_package(Protobuf REQUIRED)
# 链接库
target_link_libraries(Server MyRpc ${Protobuf_LIBRARY} muduo_net muduo_base mysqlclient  zookeeper_mt hiredis pthread )
add_subdirectory(AccountService)
add_subdirectory(FriendService)
add_subdirectory(GroupService)