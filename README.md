# TinyRPC - 高性能轻量级RPC框架

<div align="center">

![TinyRPC Logo](https://img.shields.io/badge/TinyRPC-v2.0-blue.svg)
![C++](https://img.shields.io/badge/C++-17-blue.svg)
![License](https://img.shields.io/badge/License-MIT-green.svg)
![Build Status](https://img.shields.io/badge/Build-Passing-brightgreen.svg)

**一个现代化、高性能的C++ RPC框架，专为分布式系统设计**

[快速开始](#快速开始) • [文档](#文档) • [示例](#示例) • [性能测试](#性能测试) • [贡献指南](#贡献指南)

</div>

## 🚀 特性亮点

### 🔥 核心特性
- **🚄 高性能**: 基于Muduo网络库，支持高并发处理
- **🔗 连接池**: 智能连接复用，减少连接开销
- **💾 内存池**: 高效内存管理，减少内存分配开销
- **📊 性能监控**: 内置Prometheus指标导出
- **🔍 服务发现**: 基于ZooKeeper的服务注册与发现
- **⚡ 零拷贝**: 优化的序列化和网络传输

### 🛠️ 技术栈
- **网络库**: Muduo (高性能事件驱动)
- **序列化**: Protocol Buffers
- **服务发现**: ZooKeeper
- **构建系统**: CMake
- **监控**: Prometheus指标格式

### 📈 性能优化
- **连接池管理**: 自动连接复用和清理
- **内存池优化**: 减少内存分配碎片
- **异步处理**: 非阻塞I/O操作
- **智能缓存**: 服务发现结果缓存

## 📋 环境要求

### 系统要求
- **操作系统**: Linux / macOS / Windows (WSL)
- **编译器**: GCC 7.0+ / Clang 6.0+ (支持C++17)
- **内存**: 最小512MB，推荐2GB+

### 依赖库
```bash
# Ubuntu/Debian
sudo apt-get install -y \
    build-essential \
    cmake \
    libprotobuf-dev \
    protobuf-compiler \
    libzookeeper-mt-dev \
    libmuduo-dev

# CentOS/RHEL
sudo yum install -y \
    gcc-c++ \
    cmake3 \
    protobuf-devel \
    zookeeper-native-devel
```

## 🚀 快速开始

### 1. 获取源码
```bash
git clone https://github.com/Rainllll/TinyRPC.git
cd TinyRPC
```

### 2. 构建项目
```bash
# 创建构建目录
mkdir build && cd build

# 配置构建
cmake .. -DCMAKE_BUILD_TYPE=Release

# 编译
make -j$(nproc)

# 安装（可选）
sudo make install
```

### 3. 运行示例
```bash
# 启动服务端
./bin/userservice_server

# 启动客户端（新终端）
./bin/userservice_client
```

## 📖 使用指南

### 定义服务接口

创建 `user.proto` 文件：

```protobuf
syntax = "proto3";
package tinyrpc.example;

// 用户服务定义
service UserService {
    rpc Login(LoginRequest) returns(LoginResponse);
    rpc Register(RegisterRequest) returns(RegisterResponse);
    rpc GetUserInfo(GetUserInfoRequest) returns(GetUserInfoResponse);
}

// 登录请求
message LoginRequest {
    string username = 1;
    string password = 2;
}

// 登录响应
message LoginResponse {
    int32 code = 1;
    string message = 2;
    string token = 3;
}
```

### 实现服务端

```cpp
#include "tinyrpc/rpcprovider.h"
#include "user.pb.h"

class UserServiceImpl : public tinyrpc::example::UserService {
public:
    void Login(google::protobuf::RpcController* controller,
               const tinyrpc::example::LoginRequest* request,
               tinyrpc::example::LoginResponse* response,
               google::protobuf::Closure* done) override {
        
        // 业务逻辑处理
        std::string username = request->username();
        std::string password = request->password();
        
        // 模拟登录验证
        if (username == "admin" && password == "123456") {
            response->set_code(0);
            response->set_message("Login successful");
            response->set_token("jwt_token_here");
        } else {
            response->set_code(-1);
            response->set_message("Invalid credentials");
        }
        
        // 调用完成回调
        done->Run();
    }
};

int main(int argc, char** argv) {
    // 初始化框架
    TinyRPC::Application::Init(argc, argv);
    
    // 创建服务提供者
    TinyRPC::RpcProvider provider;
    
    // 注册服务
    provider.RegisterService(new UserServiceImpl());
    
    // 启动服务
    provider.Run();
    
    return 0;
}
```

### 实现客户端

```cpp
#include "tinyrpc/rpcchannel.h"
#include "user.pb.h"

int main(int argc, char** argv) {
    // 初始化框架
    TinyRPC::Application::Init(argc, argv);
    
    // 创建RPC通道
    TinyRPC::RpcChannel channel;
    
    // 创建服务存根
    tinyrpc::example::UserService_Stub stub(&channel);
    
    // 准备请求
    tinyrpc::example::LoginRequest request;
    request.set_username("admin");
    request.set_password("123456");
    
    // 准备响应
    tinyrpc::example::LoginResponse response;
    
    // 发起RPC调用
    stub.Login(nullptr, &request, &response, nullptr);
    
    // 处理响应
    if (response.code() == 0) {
        std::cout << "Login successful! Token: " << response.token() << std::endl;
    } else {
        std::cout << "Login failed: " << response.message() << std::endl;
    }
    
    return 0;
}
```

## 📊 性能监控

### 启用监控
```cpp
#include "tinyrpc/metrics.h"

// 获取性能指标
auto& collector = TinyRPC::MetricsCollector::GetInstance();
auto metrics = collector.GetGlobalMetrics();

std::cout << "Total requests: " << metrics.request_count << std::endl;
std::cout << "Success rate: " << metrics.GetSuccessRate() * 100 << "%" << std::endl;
std::cout << "Average latency: " << metrics.GetAverageLatency() << "ms" << std::endl;
```

### Prometheus集成
```cpp
// 导出Prometheus格式指标
std::string prometheus_metrics = collector.ExportPrometheusMetrics();
// 可以通过HTTP接口暴露给Prometheus采集
```

## 🔧 配置说明

创建 `tinyrpc.conf` 配置文件：

```ini
# 服务器配置
rpcserverip=127.0.0.1
rpcserverport=8000

# ZooKeeper配置
zookeeperip=127.0.0.1
zookeeperport=2181

# 连接池配置
connectionpool.max_connections=100
connectionpool.max_idle_time=300
connectionpool.connection_timeout=10

# 内存池配置
memorypool.block_size=4096
memorypool.prealloc_count=10

# 日志配置
log.level=INFO
log.file=/var/log/tinyrpc.log
```

## 📁 项目结构

```
TinyRPC/
├── 📁 src/                     # 核心源码
│   ├── 📁 include/            # 头文件
│   │   ├── 🔧 rpcprovider.h   # RPC服务提供者
│   │   ├── 🔧 rpcchannel.h    # RPC通信通道
│   │   ├── 🔧 connectionpool.h # 连接池
│   │   ├── 🔧 memorypool.h    # 内存池
│   │   └── 🔧 metrics.h       # 性能监控
│   ├── 📄 rpcprovider.cc      # 服务提供者实现
│   ├── 📄 rpcchannel.cc       # 通信通道实现
│   ├── 📄 connectionpool.cc   # 连接池实现
│   ├── 📄 memorypool.cc       # 内存池实现
│   └── 📄 metrics.cc          # 监控实现
├── 📁 example/                # 示例代码
│   ├── 📁 callee/            # 服务端示例
│   ├── 📁 caller/            # 客户端示例
│   └── 📄 *.proto            # Protocol Buffers定义
├── 📁 test/                   # 测试代码
├── 📁 docs/                   # 文档
├── 📄 CMakeLists.txt          # 构建配置
└── 📄 README.md              # 项目说明
```

## 🎯 性能测试

### 基准测试结果

| 指标 | 数值 | 说明 |
|------|------|------|
| **QPS** | 50,000+ | 每秒请求数 |
| **延迟** | < 1ms | P99延迟 |
| **并发** | 10,000+ | 最大并发连接 |
| **内存** | < 100MB | 运行时内存占用 |

### 运行性能测试
```bash
# 编译性能测试
make benchmark

# 运行基准测试
./bin/tinyrpc_benchmark --threads=8 --duration=60s
```

## 🔍 故障排查

### 常见问题

**Q: 连接ZooKeeper失败**
```bash
# 检查ZooKeeper状态
zkServer.sh status

# 检查网络连接
telnet 127.0.0.1 2181
```

**Q: 编译错误**
```bash
# 检查依赖库
pkg-config --cflags --libs protobuf
ldconfig -p | grep muduo
```

**Q: 性能问题**
```bash
# 查看性能指标
curl http://localhost:8080/metrics

# 检查系统资源
top -p $(pgrep tinyrpc)
```

## 🤝 贡献指南

我们欢迎所有形式的贡献！

### 开发流程
1. Fork 项目
2. 创建特性分支 (`git checkout -b feature/AmazingFeature`)
3. 提交更改 (`git commit -m 'Add some AmazingFeature'`)
4. 推送到分支 (`git push origin feature/AmazingFeature`)
5. 创建 Pull Request

### 代码规范
- 遵循 [Google C++ Style Guide](https://google.github.io/styleguide/cppguide.html)
- 使用 `clang-format` 格式化代码
- 添加单元测试覆盖新功能

## 📄 许可证

本项目采用 MIT 许可证 - 查看 [LICENSE](LICENSE) 文件了解详情。

## 🙏 致谢

- [Muduo](https://github.com/chenshuo/muduo) - 高性能网络库
- [Protocol Buffers](https://developers.google.com/protocol-buffers) - 序列化框架
- [ZooKeeper](https://zookeeper.apache.org/) - 分布式协调服务

## 📞 联系我们

- **作者**: Rainllll
- **邮箱**: your.email@example.com
- **项目主页**: https://github.com/Rainllll/TinyRPC
- **问题反馈**: https://github.com/Rainllll/TinyRPC/issues

---

<div align="center">

**⭐ 如果这个项目对你有帮助，请给我们一个星标！**

Made with ❤️ by TinyRPC Team

</div>