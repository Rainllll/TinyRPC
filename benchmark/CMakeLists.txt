# 基准测试CMakeLists.txt
cmake_minimum_required(VERSION 3.10)

# 设置可执行文件输出路径
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)

# 包含头文件路径
include_directories(${PROJECT_SOURCE_DIR}/src/include)
include_directories(${PROJECT_SOURCE_DIR}/example)

# 链接库路径
link_directories(${PROJECT_SOURCE_DIR}/lib)

# 创建基准测试可执行文件
add_executable(tinyrpc_benchmark 
    benchmark.cc
    ${PROJECT_SOURCE_DIR}/example/user.pb.cc
)

# 链接必要的库
target_link_libraries(tinyrpc_benchmark 
    mprpc
    protobuf
    muduo_net
    muduo_base
    pthread
    zookeeper_mt
)