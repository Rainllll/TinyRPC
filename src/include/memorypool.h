#pragma once

#include <memory>
#include <vector>
#include <mutex>
#include <atomic>
#include <cstddef>

// 内存块
struct MemoryBlock {
    void* data;
    size_t size;
    bool in_use;
    MemoryBlock* next;
    
    MemoryBlock(size_t s) : size(s), in_use(false), next(nullptr) {
        data = std::aligned_alloc(64, s); // 64字节对齐
    }
    
    ~MemoryBlock() {
        if (data) {
            std::free(data);
        }
    }
};

// 内存池
class MemoryPool {
public:
    static MemoryPool& GetInstance();
    
    // 分配内存
    void* Allocate(size_t size);
    
    // 释放内存
    void Deallocate(void* ptr);
    
    // 设置块大小
    void SetBlockSize(size_t block_size) { block_size_ = block_size; }
    
    // 预分配内存
    void PreAllocate(size_t count);
    
    // 获取统计信息
    size_t GetAllocatedCount() const { return allocated_count_; }
    size_t GetFreeCount() const { return free_count_; }
    size_t GetTotalMemory() const { return total_memory_; }
    
private:
    MemoryPool();
    ~MemoryPool();
    
    // 创建新的内存块
    MemoryBlock* CreateBlock(size_t size);
    
    // 查找合适的内存块
    MemoryBlock* FindFreeBlock(size_t size);
    
private:
    std::vector<std::unique_ptr<MemoryBlock>> blocks_;
    MemoryBlock* free_list_;
    std::mutex mutex_;
    
    size_t block_size_ = 4096; // 默认4KB
    std::atomic<size_t> allocated_count_{0};
    std::atomic<size_t> free_count_{0};
    std::atomic<size_t> total_memory_{0};
    
    // 禁止拷贝
    MemoryPool(const MemoryPool&) = delete;
    MemoryPool& operator=(const MemoryPool&) = delete;
};

// RAII内存管理器
class MemoryGuard {
public:
    MemoryGuard(size_t size) : ptr_(MemoryPool::GetInstance().Allocate(size)) {}
    ~MemoryGuard() {
        if (ptr_) {
            MemoryPool::GetInstance().Deallocate(ptr_);
        }
    }
    
    void* Get() const { return ptr_; }
    
    // 禁止拷贝，允许移动
    MemoryGuard(const MemoryGuard&) = delete;
    MemoryGuard& operator=(const MemoryGuard&) = delete;
    
    MemoryGuard(MemoryGuard&& other) noexcept : ptr_(other.ptr_) {
        other.ptr_ = nullptr;
    }
    
    MemoryGuard& operator=(MemoryGuard&& other) noexcept {
        if (this != &other) {
            if (ptr_) {
                MemoryPool::GetInstance().Deallocate(ptr_);
            }
            ptr_ = other.ptr_;
            other.ptr_ = nullptr;
        }
        return *this;
    }
    
private:
    void* ptr_;
};