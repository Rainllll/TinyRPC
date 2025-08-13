#include <iostream>
#include <thread>
#include <vector>
#include <chrono>
#include <atomic>
#include <random>
#include "../src/include/mprpcapplication.h"
#include "../src/include/mprpcchannel.h"
#include "../src/include/metrics.h"
#include "../example/user.pb.h"

class BenchmarkTool {
public:
    BenchmarkTool(int threads, int duration_seconds, int qps_target)
        : thread_count_(threads), duration_seconds_(duration_seconds), 
          qps_target_(qps_target), running_(false) {}
    
    void Run() {
        std::cout << "Starting benchmark with " << thread_count_ << " threads for " 
                  << duration_seconds_ << " seconds..." << std::endl;
        
        running_ = true;
        auto start_time = std::chrono::steady_clock::now();
        
        // 启动工作线程
        std::vector<std::thread> workers;
        for (int i = 0; i < thread_count_; ++i) {
            workers.emplace_back(&BenchmarkTool::WorkerThread, this, i);
        }
        
        // 启动统计线程
        std::thread stats_thread(&BenchmarkTool::StatsThread, this);
        
        // 等待指定时间
        std::this_thread::sleep_for(std::chrono::seconds(duration_seconds_));
        
        // 停止测试
        running_ = false;
        
        // 等待所有线程结束
        for (auto& worker : workers) {
            worker.join();
        }
        stats_thread.join();
        
        // 输出最终结果
        PrintFinalResults(start_time);
    }
    
private:
    void WorkerThread(int worker_id) {
        // 初始化随机数生成器
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(1, 1000);
        
        // 创建RPC客户端
        MprpcChannel channel;
        fixbug::UserServiceRpc_Stub stub(&channel);
        
        while (running_) {
            auto request_start = std::chrono::steady_clock::now();
            
            // 准备请求
            fixbug::LoginRequest request;
            request.set_name("benchmark_user_" + std::to_string(dis(gen)));
            request.set_pwd("password123");
            
            fixbug::LoginResponse response;
            
            try {
                // 发起RPC调用
                stub.Login(nullptr, &request, &response, nullptr);
                
                auto request_end = std::chrono::steady_clock::now();
                auto latency = std::chrono::duration_cast<std::chrono::microseconds>(
                    request_end - request_start).count();
                
                // 更新统计信息
                total_requests_++;
                if (response.result().errcode() == 0) {
                    successful_requests_++;
                }
                
                total_latency_us_ += latency;
                
                // 更新最小/最大延迟
                uint64_t current_min = min_latency_us_.load();
                while (latency < current_min && 
                       !min_latency_us_.compare_exchange_weak(current_min, latency)) {}
                
                uint64_t current_max = max_latency_us_.load();
                while (latency > current_max && 
                       !max_latency_us_.compare_exchange_weak(current_max, latency)) {}
                
            } catch (const std::exception& e) {
                failed_requests_++;
                std::cerr << "Request failed: " << e.what() << std::endl;
            }
            
            // 控制QPS
            if (qps_target_ > 0) {
                auto sleep_time = std::chrono::microseconds(1000000 / (qps_target_ / thread_count_));
                std::this_thread::sleep_for(sleep_time);
            }
        }
    }
    
    void StatsThread() {
        auto last_time = std::chrono::steady_clock::now();
        uint64_t last_requests = 0;
        
        while (running_) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
            
            auto current_time = std::chrono::steady_clock::now();
            uint64_t current_requests = total_requests_.load();
            
            auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(
                current_time - last_time).count();
            
            if (elapsed > 0) {
                uint64_t qps = (current_requests - last_requests) / elapsed;
                double success_rate = successful_requests_.load() * 100.0 / 
                                    std::max(current_requests, 1UL);
                
                std::cout << "QPS: " << qps 
                         << ", Success Rate: " << std::fixed << std::setprecision(2) << success_rate << "%"
                         << ", Total Requests: " << current_requests << std::endl;
                
                last_time = current_time;
                last_requests = current_requests;
            }
        }
    }
    
    void PrintFinalResults(std::chrono::steady_clock::time_point start_time) {
        auto end_time = std::chrono::steady_clock::now();
        auto total_duration = std::chrono::duration_cast<std::chrono::seconds>(
            end_time - start_time).count();
        
        uint64_t total = total_requests_.load();
        uint64_t successful = successful_requests_.load();
        uint64_t failed = failed_requests_.load();
        
        std::cout << "\n========== Benchmark Results ==========" << std::endl;
        std::cout << "Duration: " << total_duration << " seconds" << std::endl;
        std::cout << "Total Requests: " << total << std::endl;
        std::cout << "Successful Requests: " << successful << std::endl;
        std::cout << "Failed Requests: " << failed << std::endl;
        std::cout << "Success Rate: " << std::fixed << std::setprecision(2) 
                  << (successful * 100.0 / std::max(total, 1UL)) << "%" << std::endl;
        
        if (total_duration > 0) {
            std::cout << "Average QPS: " << total / total_duration << std::endl;
        }
        
        if (total > 0) {
            std::cout << "Average Latency: " << std::fixed << std::setprecision(2)
                      << (total_latency_us_.load() / total) / 1000.0 << " ms" << std::endl;
            std::cout << "Min Latency: " << std::fixed << std::setprecision(2)
                      << min_latency_us_.load() / 1000.0 << " ms" << std::endl;
            std::cout << "Max Latency: " << std::fixed << std::setprecision(2)
                      << max_latency_us_.load() / 1000.0 << " ms" << std::endl;
        }
        
        // 输出框架性能指标
        auto& collector = MetricsCollector::GetInstance();
        auto metrics = collector.GetGlobalMetrics();
        
        std::cout << "\n========== Framework Metrics ==========" << std::endl;
        std::cout << "Framework Total Requests: " << metrics.request_count.load() << std::endl;
        std::cout << "Framework Success Rate: " << std::fixed << std::setprecision(2)
                  << metrics.GetSuccessRate() * 100 << "%" << std::endl;
        std::cout << "Framework Average Latency: " << std::fixed << std::setprecision(2)
                  << metrics.GetAverageLatency() << " ms" << std::endl;
    }
    
private:
    int thread_count_;
    int duration_seconds_;
    int qps_target_;
    std::atomic<bool> running_;
    
    std::atomic<uint64_t> total_requests_{0};
    std::atomic<uint64_t> successful_requests_{0};
    std::atomic<uint64_t> failed_requests_{0};
    std::atomic<uint64_t> total_latency_us_{0};
    std::atomic<uint64_t> min_latency_us_{UINT64_MAX};
    std::atomic<uint64_t> max_latency_us_{0};
};

int main(int argc, char** argv) {
    // 解析命令行参数
    int threads = 4;
    int duration = 60;
    int qps_target = 0; // 0表示不限制QPS
    
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if (arg.find("--threads=") == 0) {
            threads = std::stoi(arg.substr(10));
        } else if (arg.find("--duration=") == 0) {
            std::string duration_str = arg.substr(11);
            if (duration_str.back() == 's') {
                duration_str.pop_back();
            }
            duration = std::stoi(duration_str);
        } else if (arg.find("--qps=") == 0) {
            qps_target = std::stoi(arg.substr(6));
        }
    }
    
    // 初始化框架
    MprpcApplication::Init(argc, argv);
    
    // 运行基准测试
    BenchmarkTool benchmark(threads, duration, qps_target);
    benchmark.Run();
    
    return 0;
}