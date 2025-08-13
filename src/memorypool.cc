#include "memorypool.h"
#include <algorithm>
#include <iostream>

MemoryPool& MemoryPool::GetInstance() {
    static MemoryPool instance;
    return instance;
}

MemoryPool::MemoryPool() : free_list_(nullptr) {
    // 预分配一些内存块
    PreAllocate(10);
}

MemoryPool::~MemoryPool() {
    std::lock_guard<std::mutex> lock(mutex_);
    blocks_.clear();
    free_list_ = nullptr;
}

void* MemoryPool::Allocate(size_t size) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // 查找合适的空闲块
    MemoryBlock* block = FindFreeBlock(size);
    
    if (!block) {
        // 没有合适的空闲块，创建新块
        block = CreateBlock(std::max(size, block_size_));
        if (!block) {
            return nullptr;
        }
    }
    
    block->in_use = true;
    allocated_count_++;
    free_count_--;
    
    return block->data;
}

void MemoryPool::Deallocate(void* ptr) {
    if (!ptr) return;
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    // 查找对应的内存块
    for (auto& block_ptr : blocks_) {
        if (block_ptr->data == ptr) {
            if (block_ptr->in_use) {
                block_ptr->in_use = false;
                allocated_count_--;
                free_count_++;
                
                // 将块添加到空闲链表
                block_ptr->next = free_list_;
                free_list_ = block_ptr.get();
            }
            return;
        }
    }
}

MemoryBlock* MemoryPool::CreateBlock(size_t size) {
    try {
        auto block = std::make_unique<MemoryBlock>(size);
        if (!block->data) {
            return nullptr;
        }
        
        MemoryBlock* raw_ptr = block.get();
        blocks_.push_back(std::move(block));
        
        total_memory_ += size;
        free_count_++;
        
        // 添加到空闲链表
        raw_ptr->next = free_list_;
        free_list_ = raw_ptr;
        
        return raw_ptr;
    } catch (const std::exception& e) {
        std::cerr << "Failed to create memory block: " << e.what() << std::endl;
        return nullptr;
    }
}

MemoryBlock* MemoryPool::FindFreeBlock(size_t size) {
    MemoryBlock* prev = nullptr;
    MemoryBlock* current = free_list_;
    
    while (current) {
        if (!current->in_use && current->size >= size) {
            // 从空闲链表中移除
            if (prev) {
                prev->next = current->next;
            } else {
                free_list_ = current->next;
            }
            current->next = nullptr;
            return current;
        }
        prev = current;
        current = current->next;
    }
    
    return nullptr;
}

void MemoryPool::PreAllocate(size_t count) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    for (size_t i = 0; i < count; ++i) {
        CreateBlock(block_size_);
    }
}