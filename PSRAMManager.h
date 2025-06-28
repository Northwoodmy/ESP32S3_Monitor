/*
 * PSRAMManager.h - PSRAM内存管理器类头文件
 * ESP32S3监控项目 - PSRAM内存管理模块
 */

#ifndef PSRAMMANAGER_H
#define PSRAMMANAGER_H

#include <Arduino.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_heap_caps.h"
#include "esp_psram.h"
#include <vector>
#include <map>

// PSRAM分配块信息结构体
struct PSRAMBlockInfo {
    void* address;          // 内存块地址
    size_t size;           // 内存块大小
    String taskName;       // 分配任务名称
    unsigned long allocTime; // 分配时间戳
    String purpose;        // 分配用途描述
    bool isAllocated;      // 是否已分配
    
    PSRAMBlockInfo(void* addr, size_t sz, const String& task, const String& desc)
        : address(addr), size(sz), taskName(task), allocTime(millis()), 
          purpose(desc), isAllocated(true) {}
};

// PSRAM统计信息结构体
struct PSRAMStatistics {
    size_t totalSize;           // PSRAM总大小
    size_t usedSize;           // 已使用大小
    size_t freeSize;           // 可用大小
    size_t largestFreeBlock;   // 最大可用连续块
    uint32_t allocationCount;  // 分配次数
    uint32_t freeCount;        // 释放次数
    float fragmentationRate;   // 碎片率
    
    PSRAMStatistics() : totalSize(0), usedSize(0), freeSize(0), 
                       largestFreeBlock(0), allocationCount(0), 
                       freeCount(0), fragmentationRate(0.0f) {}
};

// PSRAM内存池类型
enum PSRAMPoolType {
    POOL_GENERAL,           // 通用内存池
    POOL_GRAPHICS,          // 图形显示专用池
    POOL_BUFFER,           // 缓冲区专用池
    POOL_TASK_STACK,       // 任务栈专用池
    POOL_COUNT             // 内存池数量
};

class PSRAMManager {
public:
    PSRAMManager();
    ~PSRAMManager();
    
    // 初始化PSRAM管理器
    bool init();
    
    // 启动PSRAM监控任务
    void start();
    
    // 停止PSRAM管理器
    void stop();
    
    // PSRAM内存分配接口
    void* allocate(size_t size, PSRAMPoolType pool = POOL_GENERAL);
    void* allocate(size_t size, const String& purpose, PSRAMPoolType pool = POOL_GENERAL);
    void* allocateAligned(size_t size, size_t alignment, PSRAMPoolType pool = POOL_GENERAL);
    
    // PSRAM内存释放接口
    bool deallocate(void* ptr);
    bool deallocateAll(const String& taskName);
    
    // 专用分配接口
    void* allocateForTask(size_t size, const String& taskName, const String& purpose);
    void* allocateGraphicsBuffer(size_t width, size_t height, size_t bytesPerPixel);
    void* allocateDataBuffer(size_t size, const String& purpose);
    TaskHandle_t createTaskWithPSRAMStack(TaskFunction_t taskFunction, 
                                          const char* taskName,
                                          uint32_t stackSize,
                                          void* parameters,
                                          UBaseType_t priority,
                                          BaseType_t coreID = tskNO_AFFINITY);
    
    // PSRAM状态查询
    bool isPSRAMAvailable() const;
    PSRAMStatistics getStatistics();
    size_t getTotalSize() const;
    size_t getUsedSize() const;
    size_t getFreeSize() const;
    size_t getLargestFreeBlock() const;
    float getUsagePercent() const;
    float getFragmentationRate() const;
    
    // 内存块信息查询
    std::vector<PSRAMBlockInfo> getAllocatedBlocks();
    PSRAMBlockInfo* getBlockInfo(void* ptr);
    uint32_t getBlockCount() const;
    
    // 内存池管理
    bool createPool(PSRAMPoolType type, size_t size);
    bool destroyPool(PSRAMPoolType type);
    size_t getPoolUsage(PSRAMPoolType type);
    
    // 内存优化和维护
    bool defragment();
    void garbageCollect();
    bool validateHeap();
    
    // 调试和监控
    void printMemoryMap();
    void printStatistics();
    String getStatusJSON();
    void setDebugMode(bool enabled);
    
    // 性能监控回调
    typedef void (*PSRAMCallback)(PSRAMStatistics stats);
    void setMonitorCallback(PSRAMCallback callback);
    
private:
    bool m_initialized;
    bool m_running;
    bool m_debugMode;
    TaskHandle_t m_monitorTaskHandle;
    SemaphoreHandle_t m_mutex;
    
    // 内存池管理
    struct MemoryPool {
        void* baseAddress;
        size_t totalSize;
        size_t usedSize;
        bool initialized;
        
        MemoryPool() : baseAddress(nullptr), totalSize(0), usedSize(0), initialized(false) {}
    };
    
    MemoryPool m_pools[POOL_COUNT];
    
    // 分配块跟踪
    std::vector<PSRAMBlockInfo> m_allocatedBlocks;
    std::map<void*, size_t> m_ptrSizeMap;
    
    // 统计数据
    PSRAMStatistics m_statistics;
    PSRAMCallback m_monitorCallback;
    
    // 内部方法
    static void monitorTask(void* parameter);
    void updateStatistics();
    void cleanupExpiredBlocks();
    String getCurrentTaskName();
    String getPoolName(PSRAMPoolType pool);
    void logAllocation(void* ptr, size_t size, const String& purpose);
    void logDeallocation(void* ptr);
    bool isValidPSRAMAddress(void* ptr);
    
    // 内存对齐辅助函数
    size_t alignSize(size_t size, size_t alignment);
    void* alignAddress(void* addr, size_t alignment);
    
    // 线程安全辅助
    bool takeMutex(TickType_t timeout = pdMS_TO_TICKS(1000));
    void giveMutex();
};

#endif // PSRAMMANAGER_H 