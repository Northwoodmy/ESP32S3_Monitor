/*
 * TimeManager.h - 时间管理器类头文件
 * ESP32S3监控项目 - NTP网络时间同步模块
 */

#ifndef TIMEMANAGER_H
#define TIMEMANAGER_H

#include <Arduino.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include <WiFi.h>
#include <time.h>
#include <sys/time.h>

// 前向声明
class PSRAMManager;
class WiFiManager;
class ConfigStorage;

// NTP服务器配置结构体
struct NTPServerConfig {
    String primaryServer;      // 主NTP服务器
    String secondaryServer;    // 备用NTP服务器
    String timezone;           // 时区设置
    int syncInterval;          // 同步间隔（分钟）
    
    NTPServerConfig() : 
        primaryServer("pool.ntp.org"),
        secondaryServer("time.nist.gov"),
        timezone("CST-8"),           // 中国标准时间
        syncInterval(60) {}          // 默认60分钟同步一次
};

// 时间同步状态枚举
enum TimeSyncStatus {
    TIME_NOT_SYNCED,        // 未同步
    TIME_SYNCING,          // 同步中
    TIME_SYNCED,           // 已同步
    TIME_SYNC_FAILED       // 同步失败
};

// 时间信息结构体
struct TimeInfo {
    int year;
    int month;
    int day;
    int hour;
    int minute;
    int second;
    int weekday;           // 0=Sunday, 1=Monday, ...
    unsigned long timestamp;
    bool valid;
    
    TimeInfo() : year(0), month(0), day(0), hour(0), minute(0), 
                 second(0), weekday(0), timestamp(0), valid(false) {}
};

class TimeManager {
public:
    TimeManager();
    ~TimeManager();
    
    // 初始化时间管理器
    bool init(PSRAMManager* psramManager, WiFiManager* wifiManager, ConfigStorage* configStorage = nullptr);
    
    // 启动时间同步任务
    bool start();
    
    // 停止时间管理器
    void stop();
    
    // NTP配置管理
    void setNTPConfig(const NTPServerConfig& config);
    NTPServerConfig getNTPConfig() const;
    void setTimezone(const String& timezone);
    void setSyncInterval(int minutes);  // 设置同步间隔（分钟）
    
    // 时间同步操作
    bool syncTimeNow();                 // 立即同步时间
    TimeSyncStatus getSyncStatus() const;
    unsigned long getLastSyncTime() const;  // 获取最后同步时间戳
    int getSyncFailCount() const;       // 获取同步失败次数
    
    // 时间获取接口
    TimeInfo getCurrentTime();          // 获取当前时间
    String getFormattedTime(const String& format = "%Y-%m-%d %H:%M:%S") const;  // 格式化时间字符串
    String getDateString() const;             // 获取日期字符串 YYYY-MM-DD
    String getTimeString() const;             // 获取时间字符串 HH:MM:SS
    String getDateTimeString() const;         // 获取完整日期时间字符串
    unsigned long getTimestamp();       // 获取Unix时间戳
    
    // 时区相关
    void setTimezone(float utcOffset);  // 设置UTC偏移（小时）
    float getTimezoneOffset() const;    // 获取时区偏移
    
    // 状态查询
    bool isTimeValid() const;           // 时间是否有效
    bool isWiFiConnected() const;       // WiFi是否连接
    String getStatusString() const;     // 获取状态字符串
    String getStatusJSON() const;       // 获取JSON格式状态
    
    // 调试和监控
    void printTimeInfo();               // 打印时间信息
    void setDebugMode(bool enabled);    // 设置调试模式
    
    // 时间回调函数类型定义
    typedef void (*TimeUpdateCallback)(const TimeInfo& timeInfo);
    void setTimeUpdateCallback(TimeUpdateCallback callback);  // 设置时间更新回调
    
private:
    // 成员变量
    PSRAMManager* m_psramManager;
    WiFiManager* m_wifiManager;
    ConfigStorage* m_configStorage;
    
    TaskHandle_t m_taskHandle;
    SemaphoreHandle_t m_mutex;
    
    bool m_initialized;
    bool m_running;
    bool m_debugMode;
    
    NTPServerConfig m_ntpConfig;
    TimeSyncStatus m_syncStatus;
    unsigned long m_lastSyncTime;
    int m_syncFailCount;
    TimeUpdateCallback m_updateCallback;
    
    struct tm m_timeinfo;
    bool m_timeValid;
    float m_timezoneOffset;
    
    // 任务配置
    static const uint32_t TASK_STACK_SIZE = 4096;    // 任务栈大小
    static const UBaseType_t TASK_PRIORITY = 2;      // 任务优先级
    static const BaseType_t TASK_CORE = 0;           // 运行在核心0
    
    // 内部方法
    static void timeTaskEntry(void* parameter);      // 任务入口函数
    void timeTask();                                 // 任务主循环
    
    bool performNTPSync();                           // 执行NTP同步
    bool configureNTP();                             // 配置NTP
    void updateLocalTime();                          // 更新本地时间
    void notifyTimeUpdate();                         // 通知时间更新
    
    void loadConfig();                               // 加载配置
    void saveConfig();                               // 保存配置
    
    String formatTime(const struct tm* timeinfo, const String& format) const;  // 格式化时间
    bool isValidTimeZone(const String& timezone);   // 验证时区格式
    
    // 线程安全辅助
    bool takeMutex(TickType_t timeout = pdMS_TO_TICKS(1000)) const;
    void giveMutex() const;
    
    // 常量定义
    static const int MAX_SYNC_RETRY = 3;             // 最大重试次数
    static const int NTP_TIMEOUT_MS = 10000;        // NTP超时时间
    static const unsigned long MIN_SYNC_INTERVAL = 5000;  // 最小同步间隔5秒
};

#endif // TIMEMANAGER_H 