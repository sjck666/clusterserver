aux_source_directory(. CLIENT_SRC)
aux_source_directory(../server/proto PROTO_SRC)
aux_source_directory(../server/offlinemsg OFFLINEMSG_SRC)
aux_source_directory(../server/db DB_SRC)
aux_source_directory(../server/redis REDIS_SRC)
add_executable(client ${CLIENT_SRC} ${PROTO_SRC} ${OFFLINEMSG_SRC} ${DB_SRC} ${REDIS_SRC})

find_package(Protobuf REQUIRED)
# 链接库
target_link_libraries(client MyRpc ${Protobuf_LIBRARY} muduo_net muduo_base mysqlclient  zookeeper_mt hiredis pthread )