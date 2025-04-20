#aux_source_directory(. SRC_LIST)
# 设置源文件列表（替代原来的aux_source_directory手动指定更可靠）
set(SRC_LIST 
    mprpcapplication.cc    # 应用主程序实现
    mprpcconfig.cc         # 配置加载模块
    rpcheader.pb.cc        # Protobuf生成的消息头
    rpcprovider.cc         # RPC服务提供者实现 
    mprpcchannel.cc        # 自定义RPC通信通道
    mprpccontroller.cc     # RPC控制器实现
    logger.cc              # 日志模块
    zookeeperutil.cc)      # ZooKeeper客户端工具

# 创建静态库目标（mprpc为库名称）
add_library(mprpc ${SRC_LIST})

# 链接第三方依赖库
target_link_libraries(mprpc 
    muduo_net     # 网络库（基于muduo）
    muduo_base    # 基础库（基于muduo）
    pthread       # POSIX线程库
    zookeeper_mt) # ZooKeeper多线程客户端库