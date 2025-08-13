#pragma once

#include <memory>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <chrono>
#include <unordered_map>
#include <thread>

// 连接对象
class Connection {
public:
    Connection(int fd, const std::string& address);
    ~Connection();
    
    int GetFd() const { return fd_; }
    const std::string& GetAddress() const { return address_; }
    bool IsValid() const;
    void SetLastUsed(std::chrono::steady_clock::time_point time) { last_used_ = time; }
    std::chrono::steady_clock::time_point GetLastUsed() const { return last_used_; }
    
private:
    int fd_;
    std::string address_;
    std::chrono::steady_clock::time_point last_used_;
};

// 连接池
class ConnectionPool {
public:
    static ConnectionPool& GetInstance();
    
    // 获取连接
    std::shared_ptr<Connection> GetConnection(const std::string& address);
    
    // 归还连接
    void ReturnConnection(std::shared_ptr<Connection> conn);
    
    // 设置连接池参数
    void SetMaxConnections(int max_conn) { max_connections_ = max_conn; }
    void SetMaxIdleTime(int seconds) { max_idle_time_ = std::chrono::seconds(seconds); }
    void SetConnectionTimeout(int seconds) { connection_timeout_ = std::chrono::seconds(seconds); }
    
    // 启动清理线程
    void StartCleanupThread();
    void StopCleanupThread();
    
private:
    ConnectionPool();
    ~ConnectionPool();
    
    // 创建新连接
    std::shared_ptr<Connection> CreateConnection(const std::string& address);
    
    // 清理过期连接
    void CleanupExpiredConnections();
    
    // 解析地址
    std::pair<std::string, uint16_t> ParseAddress(const std::string& address);
    
private:
    std::unordered_map<std::string, std::queue<std::shared_ptr<Connection>>> pools_;
    std::mutex mutex_;
    std::condition_variable cv_;
    
    int max_connections_ = 100;
    std::chrono::seconds max_idle_time_{300}; // 5分钟
    std::chrono::seconds connection_timeout_{10}; // 10秒
    
    std::atomic<bool> cleanup_running_{false};
    std::thread cleanup_thread_;
    
    // 禁止拷贝
    ConnectionPool(const ConnectionPool&) = delete;
    ConnectionPool& operator=(const ConnectionPool&) = delete;
};