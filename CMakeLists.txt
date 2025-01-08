cmake_minimum_required(VERSION 3.0)

project(MrChat)

# 配置编译选项
set(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} -g)

# 配置最终的可执行文件输出的路径
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)

# 配置头文件的搜索路径
include_directories(${PROJECT_SOURCE_DIR}/include)
include_directories(${PROJECT_SOURCE_DIR}/include/server)
include_directories(${PROJECT_SOURCE_DIR}/include/server/Interface)
include_directories(${PROJECT_SOURCE_DIR}/include/server/user)
include_directories(${PROJECT_SOURCE_DIR}/include/server/db)
include_directories(${PROJECT_SOURCE_DIR}/include/server/redis)
include_directories(${PROJECT_SOURCE_DIR}/include/server/proto)
include_directories(${PROJECT_SOURCE_DIR}/include/server/AccountService)
include_directories(${PROJECT_SOURCE_DIR}/include/server/offlinemsg)
include_directories(${PROJECT_SOURCE_DIR}/include/server/FriendService)
include_directories(${PROJECT_SOURCE_DIR}/include/server/GroupService)
link_directories(/usr/lib64/mysql)

# 加载子目录
add_subdirectory(src)