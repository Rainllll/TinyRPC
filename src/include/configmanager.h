#pragma once

#include <string>
#include <unordered_map>
#include <mutex>
#include <functional>
#include <thread>
#include <atomic>

// 配置变更回调函数类型
using ConfigChangeCallback = std::function<void(const std::string& key, const std::string& value)>;

// 增强的配置管理器
class ConfigManager {
public:
    static ConfigManager& GetInstance();
    
    // 加载配置文件
    bool LoadConfig(const std::string& config_file);
    
    // 获取配置值
    std::string GetConfig(const std::string& key, const std::string& default_value = "") const;
    int GetIntConfig(const std::string& key, int default_value = 0) const;
    bool GetBoolConfig(const std::string& key, bool default_value = false) const;
    double GetDoubleConfig(const std::string& key, double default_value = 0.0) const;
    
    // 设置配置值
    void SetConfig(const std::string& key, const std::string& value);
    void SetConfig(const std::string& key, int value);
    void SetConfig(const std::string& key, bool value);
    void SetConfig(const std::string& key, double value);
    
    // 注册配置变更回调
    void RegisterCallback(const std::string& key, ConfigChangeCallback callback);
    
    // 启动配置文件监控
    void StartFileWatcher(const std::string& config_file);
    void StopFileWatcher();
    
    // 重新加载配置
    bool ReloadConfig();
    
    // 获取所有配置
    std::unordered_map<std::string, std::string> GetAllConfigs() const;
    
    // 保存配置到文件
    bool SaveConfig(const std::string& config_file) const;
    
private:
    ConfigManager() = default;
    ~ConfigManager();
    
    // 解析配置行
    bool ParseConfigLine(const std::string& line);
    
    // 触发配置变更回调
    void TriggerCallback(const std::string& key, const std::string& value);
    
    // 文件监控线程函数
    void FileWatcherThread();
    
    // 获取文件修改时间
    time_t GetFileModifyTime(const std::string& file_path) const;
    
private:
    mutable std::mutex mutex_;
    std::unordered_map<std::string, std::string> configs_;
    std::unordered_map<std::string, std::vector<ConfigChangeCallback>> callbacks_;
    
    std::string config_file_path_;
    std::atomic<bool> file_watcher_running_{false};
    std::thread file_watcher_thread_;
    time_t last_modify_time_{0};
    
    // 禁止拷贝
    ConfigManager(const ConfigManager&) = delete;
    ConfigManager& operator=(const ConfigManager&) = delete;
};