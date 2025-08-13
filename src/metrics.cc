#include "metrics.h"
#include <sstream>
#include <algorithm>
#include <set>

MetricsCollector& MetricsCollector::GetInstance() {
    static MetricsCollector instance;
    return instance;
}

void MetricsCollector::RecordRequestStart(const std::string& service, const std::string& method) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::string key = GetKey(service, method);
    method_metrics_[key].request_count++;
    service_metrics_[service].request_count++;
    global_metrics_.request_count++;
}

void MetricsCollector::RecordRequestEnd(const std::string& service, const std::string& method,
                                       bool success, std::chrono::milliseconds latency) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::string key = GetKey(service, method);
    uint64_t latency_ms = latency.count();
    
    // 更新方法级指标
    auto& method_metric = method_metrics_[key];
    if (success) {
        method_metric.success_count++;
    } else {
        method_metric.error_count++;
    }
    method_metric.total_latency_ms += latency_ms;
    UpdateMinMax(method_metric.min_latency_ms, method_metric.max_latency_ms, latency_ms);
    
    // 更新服务级指标
    auto& service_metric = service_metrics_[service];
    if (success) {
        service_metric.success_count++;
    } else {
        service_metric.error_count++;
    }
    service_metric.total_latency_ms += latency_ms;
    UpdateMinMax(service_metric.min_latency_ms, service_metric.max_latency_ms, latency_ms);
    
    // 更新全局指标
    if (success) {
        global_metrics_.success_count++;
    } else {
        global_metrics_.error_count++;
    }
    global_metrics_.total_latency_ms += latency_ms;
    UpdateMinMax(global_metrics_.min_latency_ms, global_metrics_.max_latency_ms, latency_ms);
}

Metrics MetricsCollector::GetMetrics(const std::string& service, const std::string& method) const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::string key = GetKey(service, method);
    auto it = method_metrics_.find(key);
    return it != method_metrics_.end() ? it->second : Metrics{};
}

Metrics MetricsCollector::GetServiceMetrics(const std::string& service) const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = service_metrics_.find(service);
    return it != service_metrics_.end() ? it->second : Metrics{};
}

Metrics MetricsCollector::GetGlobalMetrics() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return global_metrics_;
}

std::string MetricsCollector::ExportPrometheusMetrics() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::ostringstream oss;
    
    // 全局指标
    oss << "# HELP tinyrpc_requests_total Total number of RPC requests\n";
    oss << "# TYPE tinyrpc_requests_total counter\n";
    oss << "tinyrpc_requests_total " << global_metrics_.request_count.load() << "\n";
    
    oss << "# HELP tinyrpc_requests_success_total Total number of successful RPC requests\n";
    oss << "# TYPE tinyrpc_requests_success_total counter\n";
    oss << "tinyrpc_requests_success_total " << global_metrics_.success_count.load() << "\n";
    
    oss << "# HELP tinyrpc_requests_error_total Total number of failed RPC requests\n";
    oss << "# TYPE tinyrpc_requests_error_total counter\n";
    oss << "tinyrpc_requests_error_total " << global_metrics_.error_count.load() << "\n";
    
    oss << "# HELP tinyrpc_request_duration_ms Request duration in milliseconds\n";
    oss << "# TYPE tinyrpc_request_duration_ms histogram\n";
    
    // 方法级指标
    for (const auto& [key, metrics] : method_metrics_) {
        size_t pos = key.find("::");
        if (pos == std::string::npos) continue;
        
        std::string service = key.substr(0, pos);
        std::string method = key.substr(pos + 2);
        
        oss << "tinyrpc_requests_total{service=\"" << service << "\",method=\"" << method << "\"} "
            << metrics.request_count.load() << "\n";
        
        oss << "tinyrpc_requests_success_total{service=\"" << service << "\",method=\"" << method << "\"} "
            << metrics.success_count.load() << "\n";
        
        oss << "tinyrpc_requests_error_total{service=\"" << service << "\",method=\"" << method << "\"} "
            << metrics.error_count.load() << "\n";
        
        if (metrics.request_count.load() > 0) {
            oss << "tinyrpc_request_duration_ms_avg{service=\"" << service << "\",method=\"" << method << "\"} "
                << metrics.GetAverageLatency() << "\n";
            
            oss << "tinyrpc_request_duration_ms_min{service=\"" << service << "\",method=\"" << method << "\"} "
                << metrics.min_latency_ms.load() << "\n";
            
            oss << "tinyrpc_request_duration_ms_max{service=\"" << service << "\",method=\"" << method << "\"} "
                << metrics.max_latency_ms.load() << "\n";
        }
    }
    
    return oss.str();
}

void MetricsCollector::ResetMetrics() {
    std::lock_guard<std::mutex> lock(mutex_);
    method_metrics_.clear();
    service_metrics_.clear();
    global_metrics_ = Metrics{};
}

void MetricsCollector::ResetMetrics(const std::string& service, const std::string& method) {
    std::lock_guard<std::mutex> lock(mutex_);
    std::string key = GetKey(service, method);
    method_metrics_.erase(key);
}

std::vector<std::string> MetricsCollector::GetServices() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::set<std::string> services;
    
    for (const auto& [key, _] : method_metrics_) {
        size_t pos = key.find("::");
        if (pos != std::string::npos) {
            services.insert(key.substr(0, pos));
        }
    }
    
    return std::vector<std::string>(services.begin(), services.end());
}

std::vector<std::string> MetricsCollector::GetMethods(const std::string& service) const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<std::string> methods;
    
    for (const auto& [key, _] : method_metrics_) {
        size_t pos = key.find("::");
        if (pos != std::string::npos && key.substr(0, pos) == service) {
            methods.push_back(key.substr(pos + 2));
        }
    }
    
    return methods;
}

void MetricsCollector::UpdateMinMax(std::atomic<uint64_t>& min_val, std::atomic<uint64_t>& max_val, uint64_t value) {
    // 更新最小值
    uint64_t current_min = min_val.load();
    while (value < current_min && !min_val.compare_exchange_weak(current_min, value)) {
        // 继续尝试
    }
    
    // 更新最大值
    uint64_t current_max = max_val.load();
    while (value > current_max && !max_val.compare_exchange_weak(current_max, value)) {
        // 继续尝试
    }
}