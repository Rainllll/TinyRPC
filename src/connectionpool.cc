#include "connectionpool.h"
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <iostream>

// Connection 实现
Connection::Connection(int fd, const std::string& address) 
    : fd_(fd), address_(address), last_used_(std::chrono::steady_clock::now()) {
}

Connection::~Connection() {
    if (fd_ != -1) {
        close(fd_);
    }
}

bool Connection::IsValid() const {
    if (fd_ == -1) return false;
    
    // 检查socket是否仍然有效
    int error = 0;
    socklen_t len = sizeof(error);
    int ret = getsockopt(fd_, SOL_SOCKET, SO_ERROR, &error, &len);
    return ret == 0 && error == 0;
}

// ConnectionPool 实现
ConnectionPool& ConnectionPool::GetInstance() {
    static ConnectionPool instance;
    return instance;
}

ConnectionPool::ConnectionPool() {
    StartCleanupThread();
}

ConnectionPool::~ConnectionPool() {
    StopCleanupThread();
}

std::shared_ptr<Connection> ConnectionPool::GetConnection(const std::string& address) {
    std::unique_lock<std::mutex> lock(mutex_);
    
    auto& pool = pools_[address];
    
    // 尝试从池中获取可用连接
    while (!pool.empty()) {
        auto conn = pool.front();
        pool.pop();
        
        if (conn->IsValid()) {
            conn->SetLastUsed(std::chrono::steady_clock::now());
            return conn;
        }
    }
    
    // 池中没有可用连接，创建新连接
    lock.unlock();
    return CreateConnection(address);
}

void ConnectionPool::ReturnConnection(std::shared_ptr<Connection> conn) {
    if (!conn || !conn->IsValid()) {
        return;
    }
    
    std::lock_guard<std::mutex> lock(mutex_);
    auto& pool = pools_[conn->GetAddress()];
    
    // 检查池大小限制
    if (pool.size() < max_connections_) {
        conn->SetLastUsed(std::chrono::steady_clock::now());
        pool.push(conn);
    }
    // 如果池已满，连接会自动销毁
}

std::shared_ptr<Connection> ConnectionPool::CreateConnection(const std::string& address) {
    auto [ip, port] = ParseAddress(address);
    
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        return nullptr;
    }
    
    // 设置非阻塞模式
    int flags = fcntl(sockfd, F_GETFL, 0);
    fcntl(sockfd, F_SETFL, flags | O_NONBLOCK);
    
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = inet_addr(ip.c_str());
    
    int ret = connect(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr));
    if (ret == -1 && errno != EINPROGRESS) {
        close(sockfd);
        return nullptr;
    }
    
    // 等待连接完成
    fd_set write_fds;
    FD_ZERO(&write_fds);
    FD_SET(sockfd, &write_fds);
    
    struct timeval timeout;
    timeout.tv_sec = connection_timeout_.count();
    timeout.tv_usec = 0;
    
    ret = select(sockfd + 1, nullptr, &write_fds, nullptr, &timeout);
    if (ret <= 0) {
        close(sockfd);
        return nullptr;
    }
    
    // 检查连接是否成功
    int error = 0;
    socklen_t len = sizeof(error);
    if (getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &error, &len) < 0 || error != 0) {
        close(sockfd);
        return nullptr;
    }
    
    // 恢复阻塞模式
    fcntl(sockfd, F_SETFL, flags);
    
    return std::make_shared<Connection>(sockfd, address);
}

std::pair<std::string, uint16_t> ConnectionPool::ParseAddress(const std::string& address) {
    size_t pos = address.find(':');
    if (pos == std::string::npos) {
        return {"127.0.0.1", 8000}; // 默认值
    }
    
    std::string ip = address.substr(0, pos);
    uint16_t port = static_cast<uint16_t>(std::stoi(address.substr(pos + 1)));
    return {ip, port};
}

void ConnectionPool::StartCleanupThread() {
    cleanup_running_ = true;
    cleanup_thread_ = std::thread([this]() {
        while (cleanup_running_) {
            std::this_thread::sleep_for(std::chrono::seconds(60)); // 每分钟清理一次
            CleanupExpiredConnections();
        }
    });
}

void ConnectionPool::StopCleanupThread() {
    cleanup_running_ = false;
    if (cleanup_thread_.joinable()) {
        cleanup_thread_.join();
    }
}

void ConnectionPool::CleanupExpiredConnections() {
    std::lock_guard<std::mutex> lock(mutex_);
    auto now = std::chrono::steady_clock::now();
    
    for (auto& [address, pool] : pools_) {
        std::queue<std::shared_ptr<Connection>> new_pool;
        
        while (!pool.empty()) {
            auto conn = pool.front();
            pool.pop();
            
            if (conn->IsValid() && (now - conn->GetLastUsed()) < max_idle_time_) {
                new_pool.push(conn);
            }
            // 过期或无效的连接会被自动销毁
        }
        
        pool = std::move(new_pool);
    }
}