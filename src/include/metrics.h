#pragma once

#include <atomic>
#include <chrono>
#include <unordered_map>
#include <mutex>
#include <string>
#include <vector>

// 性能指标
struct Metrics {
    std::atomic<uint64_t> request_count{0};
    std::atomic<uint64_t> success_count{0};
    std::atomic<uint64_t> error_count{0};
    std::atomic<uint64_t> total_latency_ms{0};
    std::atomic<uint64_t> min_latency_ms{UINT64_MAX};
    std::atomic<uint64_t> max_latency_ms{0};
    
    double GetAverageLatency() const {
        uint64_t count = request_count.load();
        return count > 0 ? static_cast<double>(total_latency_ms.load()) / count : 0.0;
    }
    
    double GetSuccessRate() const {
        uint64_t count = request_count.load();
        return count > 0 ? static_cast<double>(success_count.load()) / count : 0.0;
    }
};

// 性能监控器
class MetricsCollector {
public:
    static MetricsCollector& GetInstance();
    
    // 记录请求开始
    void RecordRequestStart(const std::string& service, const std::string& method);
    
    // 记录请求完成
    void RecordRequestEnd(const std::string& service, const std::string& method, 
                         bool success, std::chrono::milliseconds latency);
    
    // 获取指标
    Metrics GetMetrics(const std::string& service, const std::string& method) const;
    Metrics GetServiceMetrics(const std::string& service) const;
    Metrics GetGlobalMetrics() const;
    
    // 导出Prometheus格式指标
    std::string ExportPrometheusMetrics() const;
    
    // 重置指标
    void ResetMetrics();
    void ResetMetrics(const std::string& service, const std::string& method);
    
    // 获取所有服务列表
    std::vector<std::string> GetServices() const;
    std::vector<std::string> GetMethods(const std::string& service) const;
    
private:
    MetricsCollector() = default;
    ~MetricsCollector() = default;
    
    std::string GetKey(const std::string& service, const std::string& method) const {
        return service + "::" + method;
    }
    
    void UpdateMinMax(std::atomic<uint64_t>& min_val, std::atomic<uint64_t>& max_val, uint64_t value);
    
private:
    mutable std::mutex mutex_;
    std::unordered_map<std::string, Metrics> method_metrics_;
    std::unordered_map<std::string, Metrics> service_metrics_;
    Metrics global_metrics_;
    
    // 禁止拷贝
    MetricsCollector(const MetricsCollector&) = delete;
    MetricsCollector& operator=(const MetricsCollector&) = delete;
};

// RAII计时器
class Timer {
public:
    Timer(const std::string& service, const std::string& method)
        : service_(service), method_(method), start_time_(std::chrono::steady_clock::now()) {
        MetricsCollector::GetInstance().RecordRequestStart(service_, method_);
    }
    
    ~Timer() {
        auto end_time = std::chrono::steady_clock::now();
        auto latency = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time_);
        MetricsCollector::GetInstance().RecordRequestEnd(service_, method_, success_, latency);
    }
    
    void SetSuccess(bool success) { success_ = success; }
    
private:
    std::string service_;
    std::string method_;
    std::chrono::steady_clock::time_point start_time_;
    bool success_ = true;
};