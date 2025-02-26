# TinyRPC

TinyRPC 是一个轻量级的 RPC（远程过程调用）框架，旨在提供简单、高效、易用的 RPC 功能，适合学习和小型项目。

## 特性

- **轻量化**：专注于核心功能，避免过度复杂。
- **高性能**：采用高效的序列化方式和网络通信模型。
- **模块化设计**：易于扩展和维护。
- **跨平台支持**：兼容常见的操作系统。

## 功能

1. **服务注册与发现**
   - 支持服务端注册服务。
   - 客户端通过服务名调用服务。

2. **序列化与反序列化**
   - 使用 Protocol Buffers 作为默认的序列化协议。

3. **多线程支持**
   - 支持并发处理多个客户端请求。

4. **错误处理机制**
   - 提供可靠的错误检测与处理。

## 环境依赖

在开始使用 TinyRPC 之前，请确保你的开发环境满足以下要求：

- **操作系统**：Linux / macOS / Windows
- **编译器**：支持 C++11 或更高版本的编译器
- **依赖库**：
  - Protocol Buffers
  - CMake 3.10 或更高版本

## 安装与构建

### 1. 克隆仓库

```bash
git clone https://github.com/Rainllll/TinyRPC.git
cd TinyRPC
```

### 2. 构建项目

```bash
mkdir build
cd build
cmake ..
make
```

### 3. 运行测试

构建完成后，可运行项目中的测试文件以验证功能：

```bash
./tinyrpc_test
```

## 快速开始

以下是使用 TinyRPC 的基本步骤：

### 服务端

1. 定义服务接口：

使用 Protocol Buffers 定义服务接口和消息结构。例如：

```protobuf
syntax = "proto3";

service ExampleService {
  rpc SayHello (HelloRequest) returns (HelloResponse);
}

message HelloRequest {
  string name = 1;
}

message HelloResponse {
  string message = 1;
}
```

2. 实现服务逻辑：

在服务端代码中实现上述接口逻辑。

3. 启动服务端：

调用 TinyRPC 提供的 API 启动服务端，监听指定端口。

### 客户端

1. 引入生成的客户端代码。
2. 创建 RPC 客户端实例。
3. 调用服务并处理返回结果。

## 目录结构

```plaintext
TinyRPC/
├── src/            # 源代码
├── include/        # 头文件
├── proto/          # Protocol Buffers 文件
├── test/           # 测试代码
├── build/          # 构建目录
└── README.md       # 项目说明文件
```

## 贡献

欢迎大家贡献代码和提出建议！

1. 提交 Issue 报告 Bug 或提出新功能需求。
2. Fork 本仓库，开发新功能并提交 Pull Request。

## 许可证

该项目基于 [MIT 许可证](LICENSE) 许可进行分发和使用。

---

感谢使用 TinyRPC！如果你有任何问题或建议，请通过 GitHub 与我们联系。
