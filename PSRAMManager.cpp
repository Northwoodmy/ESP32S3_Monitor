/*
 * PSRAMManager.cpp - PSRAM内存管理器类实现文件
 * ESP32S3监控项目 - PSRAM内存管理模块
 */

#include "PSRAMManager.h"
#include <ArduinoJson.h>

PSRAMManager::PSRAMManager() 
    : m_initialized(false)
    , m_running(false)
    , m_debugMode(false)
    , m_monitorTaskHandle(nullptr)
    , m_mutex(nullptr)
    , m_monitorCallback(nullptr) {
    
    // 初始化统计信息
    memset(&m_statistics, 0, sizeof(PSRAMStatistics));
    
    // 初始化内存池
    for (int i = 0; i < POOL_COUNT; i++) {
        m_pools[i] = MemoryPool();
    }
    
    printf("[PSRAMManager] PSRAM管理器已创建\n");
}

PSRAMManager::~PSRAMManager() {
    stop();
    
    // 释放所有分配的内存
    for (auto& block : m_allocatedBlocks) {
        if (block.isAllocated && block.address) {
            heap_caps_free(block.address);
        }
    }
    m_allocatedBlocks.clear();
    
    // 销毁互斥锁
    if (m_mutex) {
        vSemaphoreDelete(m_mutex);
        m_mutex = nullptr;
    }
    
    printf("[PSRAMManager] PSRAM管理器已销毁\n");
}

bool PSRAMManager::init() {
    if (m_initialized) {
        printf("[PSRAMManager] PSRAM管理器已经初始化\n");
        return true;
    }
    
    printf("[PSRAMManager] 正在初始化PSRAM管理器...\n");
    
    // 创建互斥锁
    m_mutex = xSemaphoreCreateMutex();
    if (!m_mutex) {
        printf("[PSRAMManager] 互斥锁创建失败\n");
        return false;
    }
    
    // 检查PSRAM是否可用
    if (!isPSRAMAvailable()) {
        printf("[PSRAMManager] PSRAM不可用或未初始化\n");
        return false;
    }
    
    // 获取PSRAM基本信息
    size_t psramSize = ESP.getPsramSize();
    size_t freePsram = ESP.getFreePsram();
    
    printf("[PSRAMManager] PSRAM总大小: %u KB\n", psramSize / 1024);
    printf("[PSRAMManager] PSRAM可用: %u KB\n", freePsram / 1024);
    
    // 初始化统计信息
    m_statistics.totalSize = psramSize;
    m_statistics.freeSize = freePsram;
    m_statistics.usedSize = psramSize - freePsram;
    
    // 不再预分配大内存池，改为按需分配
    // 初始化内存池结构但不分配实际内存
    for (int i = 0; i < POOL_COUNT; i++) {
        m_pools[i].baseAddress = nullptr;
        m_pools[i].totalSize = 0;
        m_pools[i].usedSize = 0;
        m_pools[i].initialized = false;
    }
    
    printf("[PSRAMManager] 采用按需分配策略，不预分配内存池\n");
    
    m_initialized = true;
    printf("[PSRAMManager] PSRAM管理器初始化完成\n");
    
    return true;
}

void PSRAMManager::start() {
    if (!m_initialized) {
        printf("[PSRAMManager] 请先初始化PSRAM管理器\n");
        return;
    }
    
    if (m_running) {
        printf("[PSRAMManager] PSRAM监控任务已在运行\n");
        return;
    }
    
    printf("[PSRAMManager] 启动PSRAM监控任务...\n");
    
    m_running = true;
    
    // 创建监控任务（运行在核心0，优先级1）
    BaseType_t result = xTaskCreatePinnedToCore(
        monitorTask,
        "PSRAMMonitor",
        4096,
        this,
        1,              // 低优先级监控任务
        &m_monitorTaskHandle,
        0               // 运行在核心0
    );
    
    if (result == pdPASS) {
        printf("[PSRAMManager] PSRAM监控任务启动成功\n");
    } else {
        m_running = false;
        printf("[PSRAMManager] PSRAM监控任务启动失败\n");
    }
}

void PSRAMManager::stop() {
    if (!m_running) {
        return;
    }
    
    printf("[PSRAMManager] 停止PSRAM监控任务...\n");
    
    m_running = false;
    
    if (m_monitorTaskHandle) {
        vTaskDelete(m_monitorTaskHandle);
        m_monitorTaskHandle = nullptr;
    }
    
    printf("[PSRAMManager] PSRAM监控任务已停止\n");
}

void* PSRAMManager::allocate(size_t size, PSRAMPoolType pool) {
    return allocate(size, "未指定用途", pool);
}

void* PSRAMManager::allocate(size_t size, const String& purpose, PSRAMPoolType pool) {
    if (!m_initialized || size == 0) {
        return nullptr;
    }
    
    if (!takeMutex()) {
        printf("[PSRAMManager] 获取互斥锁失败\n");
        return nullptr;
    }
    
    // 直接从PSRAM分配，不使用内存池
    void* ptr = heap_caps_malloc(size, MALLOC_CAP_SPIRAM);
    
    if (ptr) {
        // 记录分配信息
        String taskName = getCurrentTaskName();
        String poolName = getPoolName(pool);
        String fullPurpose = poolName + " - " + purpose;
        
        m_allocatedBlocks.emplace_back(ptr, size, taskName, fullPurpose);
        m_ptrSizeMap[ptr] = size;
        
        // 更新统计信息
        m_statistics.allocationCount++;
        m_statistics.usedSize += size;
        m_statistics.freeSize -= size;
        
        if (m_debugMode) {
            printf("[PSRAMManager] ✓ 分配PSRAM: %u字节, 任务: %s, 池: %s, 用途: %s\n", 
                   size, taskName.c_str(), poolName.c_str(), purpose.c_str());
        }
        
        logAllocation(ptr, size, fullPurpose);
    } else {
        printf("[PSRAMManager] ✗ PSRAM分配失败: %u字节, 池: %s, 用途: %s\n", size, getPoolName(pool).c_str(), purpose.c_str());
    }
    
    giveMutex();
    return ptr;
}

void* PSRAMManager::allocateAligned(size_t size, size_t alignment, PSRAMPoolType pool) {
    if (!m_initialized || size == 0 || alignment == 0) {
        return nullptr;
    }
    
    // 增加对齐所需的额外空间
    size_t alignedSize = alignSize(size, alignment);
    void* ptr = allocate(alignedSize, "对齐内存分配", pool);
    
    if (ptr) {
        // 对地址进行对齐
        void* alignedPtr = alignAddress(ptr, alignment);
        if (alignedPtr != ptr) {
            // 如果需要对齐调整，更新记录
            if (takeMutex()) {
                for (auto& block : m_allocatedBlocks) {
                    if (block.address == ptr) {
                        block.address = alignedPtr;
                        break;
                    }
                }
                giveMutex();
            }
        }
        return alignedPtr;
    }
    
    return nullptr;
}

bool PSRAMManager::deallocate(void* ptr) {
    if (!ptr || !m_initialized) {
        return false;
    }
    
    if (!takeMutex()) {
        return false;
    }
    
    // 查找并移除分配记录
    bool found = false;
    size_t size = 0;
    
    for (auto it = m_allocatedBlocks.begin(); it != m_allocatedBlocks.end(); ++it) {
        if (it->address == ptr) {
            size = it->size;
            m_allocatedBlocks.erase(it);
            found = true;
            break;
        }
    }
    
    auto sizeIt = m_ptrSizeMap.find(ptr);
    if (sizeIt != m_ptrSizeMap.end()) {
        if (!found) {
            size = sizeIt->second;
            found = true;
        }
        m_ptrSizeMap.erase(sizeIt);
    }
    
    if (found) {
        heap_caps_free(ptr);
        
        // 更新统计信息
        m_statistics.freeCount++;
        m_statistics.usedSize -= size;
        m_statistics.freeSize += size;
        
        if (m_debugMode) {
            printf("[PSRAMManager] 释放PSRAM: %u字节\n", size);
        }
        
        logDeallocation(ptr);
    }
    
    giveMutex();
    return found;
}

bool PSRAMManager::deallocateAll(const String& taskName) {
    if (!m_initialized || taskName.isEmpty()) {
        return false;
    }
    
    if (!takeMutex()) {
        return false;
    }
    
    std::vector<void*> ptrsToFree;
    
    // 查找指定任务分配的所有内存块
    for (const auto& block : m_allocatedBlocks) {
        if (block.taskName == taskName && block.isAllocated) {
            ptrsToFree.push_back(block.address);
        }
    }
    
    giveMutex();
    
    // 释放找到的内存块
    bool allFreed = true;
    for (void* ptr : ptrsToFree) {
        if (!deallocate(ptr)) {
            allFreed = false;
        }
    }
    
    printf("[PSRAMManager] 释放任务 %s 的PSRAM内存: %u个块\n", 
           taskName.c_str(), ptrsToFree.size());
    
    return allFreed;
}

void* PSRAMManager::allocateForTask(size_t size, const String& taskName, const String& purpose) {
    String fullPurpose = taskName + " - " + purpose;
    return allocate(size, fullPurpose, POOL_GENERAL);
}

void* PSRAMManager::allocateGraphicsBuffer(size_t width, size_t height, size_t bytesPerPixel) {
    size_t bufferSize = width * height * bytesPerPixel;
    String purpose = "图形缓冲区 " + String(width) + "x" + String(height) + "x" + String(bytesPerPixel);
    
    // 图形缓冲区需要对齐到32字节边界以优化DMA传输
    return allocateAligned(bufferSize, 32, POOL_GRAPHICS);
}

void* PSRAMManager::allocateDataBuffer(size_t size, const String& purpose) {
    String fullPurpose = "数据缓冲区 - " + purpose;
    return allocate(size, fullPurpose, POOL_BUFFER);
}

TaskHandle_t PSRAMManager::createTaskWithPSRAMStack(TaskFunction_t taskFunction, 
                                                    const char* taskName,
                                                    uint32_t stackSize,
                                                    void* parameters,
                                                    UBaseType_t priority,
                                                    BaseType_t coreID) {
    if (!m_initialized || !taskFunction || !taskName) {
        printf("[PSRAMManager] 创建任务失败: 参数无效\n");
        return nullptr;
    }
    
    // 确保栈大小是4字节对齐的
    uint32_t alignedStackSize = (stackSize + 3) & ~3;
    
    // 为任务栈分配PSRAM内存，并确保8字节对齐
    void* stackBuffer = allocateAligned(alignedStackSize * sizeof(StackType_t), 8, POOL_TASK_STACK);
    if (!stackBuffer) {
        printf("[PSRAMManager] 为任务 %s 分配PSRAM栈失败 (大小: %u)\n", taskName, alignedStackSize);
        return nullptr;
    }
    
    // 任务控制块必须分配在内部SRAM中，不能放在PSRAM中！
    // 因为FreeRTOS调度器需要快速访问TCB
    StaticTask_t* taskBuffer = (StaticTask_t*)heap_caps_malloc(sizeof(StaticTask_t), MALLOC_CAP_INTERNAL);
    if (!taskBuffer) {
        deallocate(stackBuffer);
        printf("[PSRAMManager] 为任务 %s 分配SRAM控制块失败\n", taskName);
        return nullptr;
    }
    
    TaskHandle_t taskHandle = nullptr;
    
    printf("[PSRAMManager] 创建任务 %s: 栈=PSRAM(%uB), TCB=SRAM(%uB)\n", 
           taskName, alignedStackSize * sizeof(StackType_t), sizeof(StaticTask_t));
    
    if (coreID == tskNO_AFFINITY) {
        taskHandle = xTaskCreateStatic(taskFunction, taskName, alignedStackSize, parameters, 
                                       priority, (StackType_t*)stackBuffer, taskBuffer);
    } else {
        taskHandle = xTaskCreateStaticPinnedToCore(taskFunction, taskName, alignedStackSize, 
                                                   parameters, priority, (StackType_t*)stackBuffer, 
                                                   taskBuffer, coreID);
    }
    
    if (taskHandle) {
        printf("[PSRAMManager] ✓ 成功创建PSRAM任务: %s (栈: %u字节在PSRAM)\n", taskName, alignedStackSize * sizeof(StackType_t));
        
        // 记录任务控制块，以便后续清理
        if (takeMutex()) {
            String tcbPurpose = String(taskName) + " - TCB(SRAM)";
            m_allocatedBlocks.emplace_back(taskBuffer, sizeof(StaticTask_t), getCurrentTaskName(), tcbPurpose);
            giveMutex();
        }
    } else {
        printf("[PSRAMManager] ✗ 创建PSRAM任务失败: %s\n", taskName);
        deallocate(stackBuffer);
        heap_caps_free(taskBuffer);
    }
    
    return taskHandle;
}

bool PSRAMManager::isPSRAMAvailable() const {
    return ESP.getPsramSize() > 0;
}

PSRAMStatistics PSRAMManager::getStatistics() {
    if (takeMutex()) {
        updateStatistics();
        PSRAMStatistics stats = m_statistics;
        giveMutex();
        return stats;
    }
    return PSRAMStatistics();
}

size_t PSRAMManager::getTotalSize() const {
    return ESP.getPsramSize();
}

size_t PSRAMManager::getUsedSize() const {
    return getTotalSize() - ESP.getFreePsram();
}

size_t PSRAMManager::getFreeSize() const {
    return ESP.getFreePsram();
}

size_t PSRAMManager::getLargestFreeBlock() const {
    return heap_caps_get_largest_free_block(MALLOC_CAP_SPIRAM);
}

float PSRAMManager::getUsagePercent() const {
    size_t total = getTotalSize();
    if (total == 0) return 0.0f;
    return ((float)getUsedSize() / total) * 100.0f;
}

float PSRAMManager::getFragmentationRate() const {
    size_t freeSize = getFreeSize();
    size_t largestBlock = getLargestFreeBlock();
    
    if (freeSize == 0) return 0.0f;
    return (1.0f - ((float)largestBlock / freeSize)) * 100.0f;
}

std::vector<PSRAMBlockInfo> PSRAMManager::getAllocatedBlocks() {
    std::vector<PSRAMBlockInfo> blocks;
    
    if (takeMutex()) {
        blocks = m_allocatedBlocks;
        giveMutex();
    }
    
    return blocks;
}

PSRAMBlockInfo* PSRAMManager::getBlockInfo(void* ptr) {
    if (!ptr || !takeMutex()) {
        return nullptr;
    }
    
    PSRAMBlockInfo* info = nullptr;
    for (auto& block : m_allocatedBlocks) {
        if (block.address == ptr) {
            info = &block;
            break;
        }
    }
    
    giveMutex();
    return info;
}

uint32_t PSRAMManager::getBlockCount() const {
    return m_allocatedBlocks.size();
}

bool PSRAMManager::createPool(PSRAMPoolType type, size_t size) {
    if (type >= POOL_COUNT || size == 0) {
        return false;
    }
    
    if (m_pools[type].initialized) {
        printf("[PSRAMManager] 内存池 %d 已经初始化\n", type);
        return true;
    }
    
    void* poolMemory = heap_caps_malloc(size, MALLOC_CAP_SPIRAM);
    if (!poolMemory) {
        printf("[PSRAMManager] 内存池 %d 创建失败\n", type);
        return false;
    }
    
    m_pools[type].baseAddress = poolMemory;
    m_pools[type].totalSize = size;
    m_pools[type].usedSize = 0;
    m_pools[type].initialized = true;
    
    printf("[PSRAMManager] 内存池 %d 创建成功: %u KB\n", type, size / 1024);
    return true;
}

bool PSRAMManager::destroyPool(PSRAMPoolType type) {
    if (type >= POOL_COUNT || !m_pools[type].initialized) {
        return false;
    }
    
    if (m_pools[type].baseAddress) {
        heap_caps_free(m_pools[type].baseAddress);
    }
    
    m_pools[type] = MemoryPool();
    printf("[PSRAMManager] 内存池 %d 已销毁\n", type);
    return true;
}

size_t PSRAMManager::getPoolUsage(PSRAMPoolType type) {
    if (type >= POOL_COUNT || !m_pools[type].initialized) {
        return 0;
    }
    
    return m_pools[type].usedSize;
}

bool PSRAMManager::defragment() {
    // 简单的内存整理实现
    printf("[PSRAMManager] 开始PSRAM内存整理...\n");
    
    // 这里可以实现更复杂的内存整理算法
    // 目前只是触发系统的垃圾回收
    garbageCollect();
    
    printf("[PSRAMManager] PSRAM内存整理完成\n");
    return true;
}

void PSRAMManager::garbageCollect() {
    if (takeMutex()) {
        cleanupExpiredBlocks();
        updateStatistics();
        giveMutex();
    }
}

bool PSRAMManager::validateHeap() {
    return heap_caps_check_integrity_all(true);
}

void PSRAMManager::printMemoryMap() {
    printf("\n=== PSRAM内存映射 ===\n");
    printf("PSRAM总大小: %u KB\n", getTotalSize() / 1024);
    printf("已使用: %u KB (%.1f%%)\n", getUsedSize() / 1024, getUsagePercent());
    printf("可用: %u KB\n", getFreeSize() / 1024);
    printf("最大连续块: %u KB\n", getLargestFreeBlock() / 1024);
    printf("碎片率: %.1f%%\n", getFragmentationRate());
    printf("分配的内存块数量: %u\n", getBlockCount());
    
    if (takeMutex()) {
        printf("\n=== 分配的内存块详情 ===\n");
        for (const auto& block : m_allocatedBlocks) {
            printf("地址: %p, 大小: %u字节, 任务: %s, 用途: %s\n",
                   block.address, block.size, 
                   block.taskName.c_str(), block.purpose.c_str());
        }
        giveMutex();
    }
    
    printf("=====================\n\n");
}

void PSRAMManager::printStatistics() {
    PSRAMStatistics stats = getStatistics();
    
    printf("\n=== PSRAM统计信息 ===\n");
    printf("总大小: %u KB\n", stats.totalSize / 1024);
    printf("已使用: %u KB\n", stats.usedSize / 1024);
    printf("可用: %u KB\n", stats.freeSize / 1024);
    printf("最大块: %u KB\n", stats.largestFreeBlock / 1024);
    printf("分配次数: %u\n", stats.allocationCount);
    printf("释放次数: %u\n", stats.freeCount);
    printf("碎片率: %.1f%%\n", stats.fragmentationRate);
    printf("==================\n\n");
}

String PSRAMManager::getStatusJSON() {
    DynamicJsonDocument doc(1024);
    PSRAMStatistics stats = getStatistics();
    
    doc["available"] = isPSRAMAvailable();
    doc["totalSize"] = stats.totalSize;
    doc["usedSize"] = stats.usedSize;
    doc["freeSize"] = stats.freeSize;
    doc["largestFreeBlock"] = stats.largestFreeBlock;
    doc["usagePercent"] = getUsagePercent();
    doc["fragmentationRate"] = stats.fragmentationRate;
    doc["allocationCount"] = stats.allocationCount;
    doc["freeCount"] = stats.freeCount;
    doc["blockCount"] = getBlockCount();
    
    JsonArray blocks = doc.createNestedArray("allocatedBlocks");
    if (takeMutex()) {
        for (const auto& block : m_allocatedBlocks) {
            JsonObject blockObj = blocks.createNestedObject();
            blockObj["address"] = String((unsigned long)block.address, HEX);
            blockObj["size"] = block.size;
            blockObj["taskName"] = block.taskName;
            blockObj["purpose"] = block.purpose;
            blockObj["allocTime"] = block.allocTime;
        }
        giveMutex();
    }
    
    String result;
    serializeJson(doc, result);
    return result;
}

void PSRAMManager::setDebugMode(bool enabled) {
    m_debugMode = enabled;
    printf("[PSRAMManager] 调试模式: %s\n", enabled ? "开启" : "关闭");
}

void PSRAMManager::setMonitorCallback(PSRAMCallback callback) {
    m_monitorCallback = callback;
}

void PSRAMManager::monitorTask(void* parameter) {
    PSRAMManager* manager = static_cast<PSRAMManager*>(parameter);
    
    printf("[PSRAMManager] PSRAM监控任务开始运行\n");
    
    while (manager->m_running) {
        if (manager->takeMutex()) {
            manager->updateStatistics();
            manager->cleanupExpiredBlocks();
            
            // 触发监控回调
            if (manager->m_monitorCallback) {
                manager->m_monitorCallback(manager->m_statistics);
            }
            
            manager->giveMutex();
        }
        
        // 每5秒更新一次统计信息
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
    
    printf("[PSRAMManager] PSRAM监控任务结束\n");
    vTaskDelete(nullptr);
}

void PSRAMManager::updateStatistics() {
    m_statistics.totalSize = ESP.getPsramSize();
    m_statistics.freeSize = ESP.getFreePsram();
    m_statistics.usedSize = m_statistics.totalSize - m_statistics.freeSize;
    m_statistics.largestFreeBlock = heap_caps_get_largest_free_block(MALLOC_CAP_SPIRAM);
    
    // 计算碎片率
    if (m_statistics.freeSize > 0) {
        m_statistics.fragmentationRate = 
            (1.0f - ((float)m_statistics.largestFreeBlock / m_statistics.freeSize)) * 100.0f;
    } else {
        m_statistics.fragmentationRate = 0.0f;
    }
}

void PSRAMManager::cleanupExpiredBlocks() {
    // 清理无效的内存块记录
    m_allocatedBlocks.erase(
        std::remove_if(m_allocatedBlocks.begin(), m_allocatedBlocks.end(),
                       [this](const PSRAMBlockInfo& block) {
                           return !isValidPSRAMAddress(block.address);
                       }),
        m_allocatedBlocks.end()
    );
}

String PSRAMManager::getCurrentTaskName() {
    TaskHandle_t currentTask = xTaskGetCurrentTaskHandle();
    if (currentTask) {
        return String(pcTaskGetTaskName(currentTask));
    }
    return "Unknown";
}

String PSRAMManager::getPoolName(PSRAMPoolType pool) {
    switch (pool) {
        case POOL_GENERAL: return "通用池";
        case POOL_GRAPHICS: return "图形池";
        case POOL_BUFFER: return "缓冲池";
        case POOL_TASK_STACK: return "任务栈池";
        default: return "未知池";
    }
}

void PSRAMManager::logAllocation(void* ptr, size_t size, const String& purpose) {
    if (m_debugMode) {
        printf("[PSRAMManager] 分配: %p, %u字节, %s\n", ptr, size, purpose.c_str());
    }
}

void PSRAMManager::logDeallocation(void* ptr) {
    if (m_debugMode) {
        printf("[PSRAMManager] 释放: %p\n", ptr);
    }
}

bool PSRAMManager::isValidPSRAMAddress(void* ptr) {
    if (!ptr) return false;
    
    // 检查地址是否在PSRAM范围内
    uintptr_t addr = (uintptr_t)ptr;
    return (addr >= 0x3D800000 && addr < 0x3E000000); // ESP32S3 PSRAM地址范围
}

size_t PSRAMManager::alignSize(size_t size, size_t alignment) {
    return (size + alignment - 1) & ~(alignment - 1);
}

void* PSRAMManager::alignAddress(void* addr, size_t alignment) {
    uintptr_t address = (uintptr_t)addr;
    uintptr_t aligned = (address + alignment - 1) & ~(alignment - 1);
    return (void*)aligned;
}

bool PSRAMManager::takeMutex(TickType_t timeout) {
    if (!m_mutex) return false;
    return xSemaphoreTake(m_mutex, timeout) == pdTRUE;
}

void PSRAMManager::giveMutex() {
    if (m_mutex) {
        xSemaphoreGive(m_mutex);
    }
} 