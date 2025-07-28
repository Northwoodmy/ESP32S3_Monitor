/*
 * DisplayManager - 显示管理器
 * 
 * 功能说明：
 * - 管理LVGL显示界面和页面切换
 * - 处理系统状态显示和更新
 * - 管理显示模式和主题切换
 * - 屏幕模式管理（定时开关、延时熄屏等）
 * - 触摸活动检测和超时控制
 * - 提供高级显示功能接口
 * - WiFi状态、系统信息显示
 * - 触摸按钮和交互处理
 * 
 * 设计特点：
 * - 模块化C++面向对象设计
 * - FreeRTOS任务管理
 * - 线程安全的显示更新
 * - 可扩展的页面管理系统
 * - 智能屏幕模式控制
 */

#ifndef DISPLAY_MANAGER_H
#define DISPLAY_MANAGER_H

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"
#include "lvgl.h"
#include "LVGL_Driver.h"
#include "PowerMonitorData.h"
#include "ConfigStorage.h"

// 新的UI系统头文件
#include "ui.h"
#include "ui2.h"

// 前向声明
class WiFiManager;
class ConfigStorage;
class PSRAMManager;
class WeatherManager;

/**
 * @brief 显示页面枚举
 */
enum DisplayPage {
    PAGE_HOME = 0,      ///< 主页面
    PAGE_POWER_TOTAL,   ///< 总功率页面
    PAGE_POWER_PORT1,   ///< 端口1功率页面
    PAGE_POWER_PORT2,   ///< 端口2功率页面
    PAGE_POWER_PORT3,   ///< 端口3功率页面
    PAGE_POWER_PORT4,   ///< 端口4功率页面
    PAGE_WIFI_STATUS,   ///< WiFi状态页面
    PAGE_SYSTEM_INFO,   ///< 系统信息页面
    PAGE_SETTINGS,      ///< 设置页面
    PAGE_ABOUT,         ///< 关于页面
    PAGE_OTA_PROGRESS,  ///< OTA进度页面
    PAGE_COUNT          ///< 页面总数
};

/**
 * @brief 显示主题枚举
 */
enum DisplayTheme {
    THEME_UI1 = 0,      ///< UI1主题（原UI系统）
    THEME_UI2,          ///< UI2主题（新UI系统）
    THEME_AUTO          ///< 自动主题
};

/**
 * @brief 显示消息结构
 */
struct DisplayMessage {
    enum MessageType {
        MSG_UPDATE_WIFI_STATUS,     ///< 更新WiFi状态
        MSG_UPDATE_SYSTEM_INFO,     ///< 更新系统信息
        MSG_UPDATE_POWER_DATA,      ///< 更新功率数据
        MSG_UPDATE_WEATHER_DATA,    ///< 更新天气数据
        MSG_SWITCH_PAGE,            ///< 切换页面
        MSG_SET_BRIGHTNESS,         ///< 设置亮度
        MSG_SET_THEME,              ///< 设置主题
        MSG_SHOW_NOTIFICATION,      ///< 显示通知
        MSG_SCREEN_MODE_CHANGED,    ///< 屏幕模式改变
        MSG_TOUCH_ACTIVITY,         ///< 触摸活动
        MSG_SCREEN_ON,              ///< 屏幕开启
        MSG_SCREEN_OFF,             ///< 屏幕关闭
        MSG_AUTO_SWITCH_PORT,       ///< 自动切换到端口屏幕
        MSG_UPDATE_OTA_STATUS,      ///< 更新OTA状态
        MSG_OTA_START,              ///< OTA开始
        MSG_OTA_COMPLETE            ///< OTA完成
    } type;
    
    union {
        struct {
            bool connected;
            char ssid[32];
            char ip[16];
            int rssi;
        } wifi_status;
        
        struct {
            uint32_t free_heap;
            uint32_t uptime;
            float cpu_usage;
        } system_info;
        
        struct {
            PowerMonitorData power_data;
        } power_monitor;
        
        struct {
            char temperature[16];
            char weather[32];
            bool valid;
        } weather_data;
        
        struct {
            DisplayPage page;
        } page_switch;
        
        struct {
            uint8_t brightness;
        } brightness;
        
        struct {
            DisplayTheme theme;
        } theme;
        
        struct {
            char text[64];
            uint32_t duration_ms;
        } notification;
        
        struct {
            ScreenMode mode;
            int startHour;
            int startMinute;
            int endHour;
            int endMinute;
            int timeoutMinutes;
            bool autoRotationEnabled;
            int staticRotation;
        } screen_mode;
        
        struct {
            int port_index;         ///< 端口索引（0-3）
            uint32_t duration_ms;   ///< 显示时长（毫秒）
        } auto_switch_port;
        
        struct {
            int status;             ///< OTA状态 (0=idle, 1=uploading, 2=downloading, 3=writing, 4=success, 5=failed)
            float progress;         ///< 进度百分比 (0.0-100.0)
            uint32_t totalSize;     ///< 总文件大小
            uint32_t writtenSize;   ///< 已写入大小
            char statusText[64];    ///< 状态文本描述
            char errorMessage[128]; ///< 错误信息
            bool isServerOTA;       ///< 是否为服务器OTA
        } ota_status;
    } data;
};

/**
 * @brief 显示管理器类
 * 
 * 负责管理ESP32S3监控系统的所有显示功能，包括：
 * - 多页面UI管理
 * - 实时状态显示
 * - 主题和亮度控制
 * - 屏幕模式管理（定时开关、延时熄屏等）
 * - 触摸交互处理
 * - 通知和消息显示
 */
class DisplayManager {
public:
    /**
     * @brief 亮度渐变方向枚举
     */
    enum FadeDirection {
        FADE_TO_ON = 0,                 ///< 渐变到开启状态（亮起）
        FADE_TO_OFF                     ///< 渐变到关闭状态（变暗）
    };

    /**
     * @brief 构造函数
     */
    DisplayManager();
    
    /**
     * @brief 析构函数
     */
    ~DisplayManager();
    
    /**
     * @brief 初始化显示管理器
     * 
     * @param lvgl_driver LVGL驱动实例指针
     * @param wifi_manager WiFi管理器实例指针
     * @param config_storage 配置存储实例指针
     * @param psram_manager PSRAM管理器实例指针（可选）
     * @param weather_manager 天气管理器实例指针（可选）
     * @return true 初始化成功，false 初始化失败
     */
    bool init(LVGLDriver* lvgl_driver, WiFiManager* wifi_manager, ConfigStorage* config_storage, PSRAMManager* psram_manager = nullptr, WeatherManager* weather_manager = nullptr);
    
    /**
     * @brief 启动显示管理器任务
     * 
     * @return true 启动成功，false 启动失败
     */
    bool start();
    
    /**
     * @brief 停止显示管理器任务
     */
    void stop();
    
    /**
     * @brief 切换显示页面
     * 
     * @param page 目标页面
     */
    void switchPage(DisplayPage page);
    
    /**
     * @brief 更新WiFi状态显示
     * 
     * @param connected 连接状态
     * @param ssid WiFi SSID
     * @param ip IP地址
     * @param rssi 信号强度
     */
    void updateWiFiStatus(bool connected, const char* ssid, const char* ip, int rssi);
    
    /**
     * @brief 更新系统信息显示
     * 
     * @param free_heap 可用内存
     * @param uptime 运行时间
     * @param cpu_usage CPU使用率
     */
    void updateSystemInfo(uint32_t free_heap, uint32_t uptime, float cpu_usage);
    
    /**
     * @brief 设置显示亮度
     * 
     * @param brightness 亮度值（0-100）
     */
    void setBrightness(uint8_t brightness);
    
    /**
     * @brief 获取当前亮度
     * 
     * @return 当前亮度值（0-100）
     */
    uint8_t getBrightness() const;
    
    /**
     * @brief 设置显示主题
     * 
     * @param theme 主题类型
     */
    void setTheme(DisplayTheme theme);
    
    /**
     * @brief 获取当前主题
     * 
     * @return 当前主题类型
     */
    DisplayTheme getTheme() const;
    
    /**
     * @brief 显示通知消息
     * 
     * @param text 通知文本
     * @param duration_ms 显示时长（毫秒）
     */
    void showNotification(const char* text, uint32_t duration_ms = 3000);
    
    /**
     * @brief 更新功率监控数据
     * 
     * @param power_data 功率监控数据
     */
    void updatePowerData(const PowerMonitorData& power_data);
    
    /**
     * @brief 更新天气数据显示
     * 
     * @param temperature 温度字符串
     * @param weather 天气状况字符串
     */
    void updateWeatherData(const char* temperature, const char* weather);
    
    /**
     * @brief 切换到功率监控页面
     * 
     * @param port_index 端口索引（0=总功率，1-4=端口1-4）
     */
    void switchToPowerPage(int port_index);
    
    /**
     * @brief 获取当前功率数据
     * 
     * @return 当前功率数据
     */
    const PowerMonitorData& getCurrentPowerData() const;
    
    /**
     * @brief 获取当前页面
     * 
     * @return 当前显示页面
     */
    DisplayPage getCurrentPage() const;
    
    /**
     * @brief 检查是否已初始化
     * 
     * @return true 已初始化，false 未初始化
     */
    bool isInitialized() const;
    
    /**
     * @brief 检查任务是否在运行
     * 
     * @return true 任务运行中，false 任务未运行
     */
    bool isRunning() const;
    
    /**
     * @brief 更新端口详细信息显示
     */
    void updatePortDetailDisplay(int port_index);
    
    /**
     * @brief 更新时间显示
     */
    void updateTimeDisplay();
    
    /**
     * @brief 更新功率数据显示
     */
    void updatePowerDataDisplay();
    
    /**
     * @brief 更新功率条显示
     */
    void updatePowerBars();
    
    /**
     * @brief 更新天气显示
     */
    void updateWeatherDisplay();
    
    /**
     * @brief 更新UI2系统端口详细页面数据
     */
    void updateUI2PortDetailPages();
    
    // === 屏幕模式管理功能 ===
    
    /**
     * @brief 加载屏幕模式配置
     * 
     * @return true 加载成功，false 加载失败
     */
    bool loadScreenModeConfig();
    
    /**
     * @brief 设置屏幕模式
     * 
     * @param mode 屏幕模式
     * @param startHour 开始小时（定时模式）
     * @param startMinute 开始分钟（定时模式）
     * @param endHour 结束小时（定时模式）
     * @param endMinute 结束分钟（定时模式）
     * @param timeoutMinutes 延时分钟（延时模式）
     * @param autoRotationEnabled 自动旋转是否启用
     */
    void setScreenMode(ScreenMode mode, int startHour = 8, int startMinute = 0, 
                       int endHour = 22, int endMinute = 0, int timeoutMinutes = 10, bool autoRotationEnabled = true);
    
    /**
     * @brief 获取当前屏幕模式
     * 
     * @return 当前屏幕模式
     */
    ScreenMode getScreenMode() const;
    
    /**
     * @brief 检查屏幕是否应该开启
     * 
     * @return true 应该开启，false 应该关闭
     */
    bool shouldScreenBeOn() const;
    
    // === OTA升级进度显示功能 ===
    
    /**
     * @brief 开始OTA升级显示
     * 
     * @param isServerOTA 是否为服务器OTA升级
     */
    void startOTADisplay(bool isServerOTA = false);
    
    /**
     * @brief 更新OTA升级状态
     * 
     * @param status OTA状态 (0=idle, 1=uploading, 2=downloading, 3=writing, 4=success, 5=failed)
     * @param progress 进度百分比 (0.0-100.0)
     * @param totalSize 总文件大小
     * @param writtenSize 已写入大小
     * @param statusText 状态文本描述
     * @param errorMessage 错误信息（可选）
     */
    void updateOTAStatus(int status, float progress, uint32_t totalSize, uint32_t writtenSize, 
                        const char* statusText, const char* errorMessage = nullptr);
    
    /**
     * @brief 完成OTA升级显示
     * 
     * @param success 是否成功
     * @param message 完成消息
     */
    void completeOTADisplay(bool success, const char* message);
    
    // === 功率控制屏幕功能 ===
    
    /**
     * @brief 设置功率控制阈值
     * 
     * 注意：功率控制仅在延时模式(SCREEN_MODE_TIMEOUT)下生效，不影响其他屏幕模式
     * 
     * @param enablePowerControl 是否启用功率控制
     * @param powerOffThreshold 功率关闭阈值（mW，小于此值延时关闭屏幕）
     * @param powerOnThreshold 功率开启阈值（mW，大于此值立即开启屏幕）
     */
    void setPowerControlThresholds(bool enablePowerControl, int powerOffThreshold = 1000, int powerOnThreshold = 2000);
    
    /**
     * @brief 获取功率控制启用状态
     * 
     * @return true 功率控制已启用，false 功率控制未启用
     */
    bool isPowerControlEnabled() const;
    
    /**
     * @brief 获取当前功率状态
     * 
     * @return 当前总功率（mW）
     */
    int getCurrentPower() const;
    
    /**
     * @brief 启用或禁用自动切换端口功能
     * 
     * @param enabled true启用，false禁用
     */
    void setAutoSwitchEnabled(bool enabled);
    
    /**
     * @brief 获取自动切换端口功能状态
     * 
     * @return true已启用，false已禁用
     */
    bool isAutoSwitchEnabled() const;
    
    /**
     * @brief 设置基于总功率的自动页面切换功能
     * 
     * @param enabled 是否启用
     * @param lowPowerThreshold 低功率阈值（mW），小于此值显示待机页面
     * @param highPowerThreshold 高功率阈值（mW），大于此值显示总功率页面
     * @param checkInterval 功率检查间隔（毫秒）
     */
    void setPowerBasedAutoSwitch(bool enabled, int lowPowerThreshold = 1000, int highPowerThreshold = 2000, uint32_t checkInterval = 2000);
    
    /**
     * @brief 获取基于总功率的自动页面切换功能状态
     * 
     * @return true 已启用，false 未启用
     */
    bool isPowerBasedAutoSwitchEnabled() const;
    
    /**
     * @brief 手动切换页面（会标记为手动切换）
     * 
     * @param page 目标页面
     */
    void manualSwitchPage(DisplayPage page);
    
    /**
     * @brief 强制开启屏幕
     */
    void forceScreenOn();
    
    /**
     * @brief 强制关闭屏幕
     */
    void forceScreenOff();
    
    /**
     * @brief 通知触摸活动
     * 
     * 用于延时模式的活动检测
     */
    void notifyTouchActivity();
    
    /**
     * @brief 检查屏幕是否开启
     * 
     * @return true 屏幕开启，false 屏幕关闭
     */
    bool isScreenOn() const;
    
    /**
     * @brief 检查触摸唤醒功能是否可用
     * 
     * @return true 触摸唤醒可用，false 不可用
     */
    bool isTouchWakeupEnabled() const;
    
    /**
     * @brief 获取触摸活动状态信息
     * 
     * @param lastTouchTime 最后触摸时间
     * @param timeSinceLastTouch 距离最后触摸的时间间隔（毫秒）
     * @param isInLowPower 是否处于低功率状态
     */
    void getTouchWakeupStatus(uint32_t& lastTouchTime, uint32_t& timeSinceLastTouch, bool& isInLowPower) const;
    
    // === 亮度渐变控制功能 ===
    
    /**
     * @brief 启用或禁用亮度渐变功能
     * 
     * @param enabled true启用渐变，false禁用渐变
     * @param fadeDurationMs 渐变持续时间（毫秒），默认1000ms
     */
    void setFadingEnabled(bool enabled, uint32_t fadeDurationMs = 1000);
    
    /**
     * @brief 获取亮度渐变功能状态
     * 
     * @return true渐变已启用，false渐变未启用
     */
    bool isFadingEnabled() const;
    
    /**
     * @brief 检查是否正在执行亮度渐变
     * 
     * @return true正在渐变，false未在渐变
     */
    bool isFading() const;
    
    /**
     * @brief 设置渐变持续时间
     * 
     * @param fadeDurationMs 渐变持续时间（毫秒）
     */
    void setFadeDuration(uint32_t fadeDurationMs);
    
    /**
     * @brief 获取当前渐变亮度值
     * 
     * @return 当前渐变亮度值（0-100）
     */
    uint8_t getCurrentFadingBrightness() const;
    
    /**
     * @brief 根据当前主题切换到对应的端口屏幕
     */
    void switchToPortScreen(int port_index);
    
    /**
     * @brief 设置全局实例指针（供UI系统回调使用）
     */
    static void setInstance(DisplayManager* instance);
    
    /**
     * @brief 获取全局实例指针
     */
    static DisplayManager* getInstance();
    
    /**
     * @brief 更新当前页面状态（供UI系统回调使用）
     */
    void updateCurrentPage(DisplayPage page);
    
    /**
     * @brief 根据屏幕对象更新当前页面状态
     */
    void updateCurrentPageByScreen(lv_obj_t* screen);
    
    /**
     * @brief 获取LVGL驱动实例
     * 
     * @return LVGL驱动指针
     */
    LVGLDriver* getLVGLDriver() const;

private:
    /**
     * @brief 显示管理器任务静态入口
     * 
     * @param arg 任务参数（DisplayManager实例指针）
     */
    static void displayTaskEntry(void* arg);
    
    /**
     * @brief 显示管理器任务执行函数
     */
    void displayTask();
    
    /**
     * @brief 处理显示消息
     * 
     * @param msg 显示消息
     */
    void processMessage(const DisplayMessage& msg);
    
    // 旧款手动UI创建和回调函数已删除 - 现在使用SquareLine Studio生成的UI1和UI2系统
    
    /**
     * @brief 切换UI系统
     * 
     * @param theme UI主题类型
     */
    void switchUISystem(DisplayTheme theme);
    
    /**
     * @brief 初始化UI1系统
     */
    void initUI1System();
    
    /**
     * @brief 初始化UI2系统
     */
    void initUI2System();
    
    /**
     * @brief 销毁UI1系统
     */
    void destroyUI1System();
    
    /**
     * @brief 销毁UI2系统
     */
    void destroyUI2System();
    
    // 旧款手动UI管理函数已删除 - 现在使用SquareLine Studio生成的UI1和UI2系统
    
    // === 屏幕模式管理私有方法 ===
    
    /**
     * @brief 处理屏幕模式逻辑
     */
    void processScreenModeLogic();
    
    /**
     * @brief 检查定时模式是否应该开启屏幕
     */
    bool isInScheduledTime() const;
    
    /**
     * @brief 检查延时模式是否应该关闭屏幕
     * 
     * 延时模式下需要同时满足两个条件才会熄屏：
     * 1. 延时时间到达
     * 2. 总功率小于1W
     */
    bool isTimeoutExpired() const;
    
    /**
     * @brief 执行屏幕开启操作
     */
    void performScreenOn();
    
    /**
     * @brief 执行屏幕关闭操作
     */
    void performScreenOff();
    
    /**
     * @brief 重置延时计时器
     */
    void resetTimeoutTimer();
    
    /**
     * @brief 处理基于功率的延时逻辑
     * 
     * 在延时模式下，当功率小于1W时开始计算延时；
     * 当功率超过1W时重置延时计时器。
     */
    void processPowerBasedTimeoutLogic();
    
    // === 功率控制屏幕私有方法 ===
    
    /**
     * @brief 检查功率状态并管理屏幕
     * 
     * 仅在延时模式(SCREEN_MODE_TIMEOUT)下生效
     */
    void processPowerControlLogic();
    
    /**
     * @brief 检查是否应该基于功率开启屏幕
     * 
     * @return true 应该开启，false 应该关闭或保持当前状态
     */
    bool shouldScreenBeOnBasedOnPower() const;
    
    // === 自动切换端口私有方法 ===
    
    /**
     * @brief 检查端口功率变化并触发自动切换
     */
    void checkPortPowerChange();
    
    /**
     * @brief 执行自动切换到端口屏幕
     * 
     * @param port_index 端口索引（0-3）
     * @param duration_ms 显示时长（毫秒）
     */
    void performAutoSwitchToPort(int port_index, uint32_t duration_ms);
    
    /**
     * @brief 检查自动切换是否应该结束
     */
    void checkAutoSwitchTimeout();
    
    /**
     * @brief 恢复到切换前的页面
     */
    void restorePreviousPage();
    
    // === 基于总功率的自动页面切换私有方法 ===
    
    /**
     * @brief 检查总功率变化并执行自动页面切换
     */
    void checkPowerBasedPageSwitch();
    
    /**
     * @brief 根据总功率决定应该显示的页面
     * 
     * @return 应该显示的页面
     */
    DisplayPage getTargetPageByPower() const;
    
    /**
     * @brief 检查是否为端口页面
     * 
     * @param page 页面类型
     * @return true 是端口页面，false 不是端口页面
     */
    bool isPortPage(DisplayPage page) const;
    
    /**
     * @brief 检查是否应该执行自动切换
     * 
     * @return true 应该执行自动切换，false 不应该执行
     */
    bool shouldExecuteAutoSwitch() const;
    
    // === OTA升级进度私有方法 ===
    
    /**
     * @brief 创建OTA进度显示页面
     */
    void createOTAProgressPage();
    
    /**
     * @brief 销毁OTA进度显示页面
     */
    void destroyOTAProgressPage();
    
    /**
     * @brief 更新OTA进度条显示
     * 
     * @param progress 进度百分比 (0.0-100.0)
     */
    void updateOTAProgressBar(float progress);
    
    /**
     * @brief 更新OTA状态文本显示
     * 
     * @param statusText 状态文本
     */
    void updateOTAStatusText(const char* statusText);
    
    /**
     * @brief 更新OTA大小信息显示
     * 
     * @param totalSize 总文件大小
     * @param writtenSize 已写入大小
     */
    void updateOTASizeInfo(uint32_t totalSize, uint32_t writtenSize);
    
         /**
      * @brief 显示OTA错误信息
      * 
      * @param errorMessage 错误信息
      */
     void showOTAError(const char* errorMessage);
     
     /**
      * @brief 重新布局OTA进度页面（适应屏幕旋转）
      */
     void relayoutOTAProgressPage();
    
    // === 亮度渐变私有方法 ===
    
    /**
     * @brief 启动亮度渐变
     * 
     * @param targetBrightness 目标亮度值（0-100）
     * @param direction 渐变方向
     */
    void startFading(uint8_t targetBrightness, FadeDirection direction);
    
    /**
     * @brief 处理亮度渐变逻辑
     * 
     * 在主任务循环中定期调用，更新渐变进度
     */
    void processFading();
    
    /**
     * @brief 停止当前的亮度渐变
     */
    void stopFading();
    
    /**
     * @brief 执行即时屏幕开启操作（不使用渐变）
     */
    void performScreenOnImmediate();
    
    /**
     * @brief 执行即时屏幕关闭操作（不使用渐变）
     */
    void performScreenOffImmediate();
    
private:
    // 成员变量
    bool m_initialized;                 ///< 初始化状态
    bool m_running;                     ///< 运行状态
    TaskHandle_t m_taskHandle;          ///< 任务句柄
    QueueHandle_t m_messageQueue;       ///< 消息队列
    
    // 静态全局实例指针，用于UI系统回调
    static DisplayManager* s_instance;
    
    // 外部依赖
    LVGLDriver* m_lvglDriver;           ///< LVGL驱动指针
    WiFiManager* m_wifiManager;         ///< WiFi管理器指针
    ConfigStorage* m_configStorage;     ///< 配置存储指针
    PSRAMManager* m_psramManager;        ///< PSRAM管理器指针
    WeatherManager* m_weatherManager;   ///< 天气管理器指针
    
    // 显示状态
    DisplayPage m_currentPage;          ///< 当前页面
    DisplayTheme m_currentTheme;        ///< 当前主题
    uint8_t m_brightness;               ///< 当前亮度
    bool m_uiSystemActive;              ///< UI系统是否已激活
    
    // LVGL对象
    lv_obj_t* m_screen;                 ///< 主屏幕对象
    
    // 功率监控相关
    PowerMonitorData m_powerData;       ///< 功率监控数据
    
    // === 屏幕模式管理成员变量 ===
    ScreenMode m_screenMode;            ///< 当前屏幕模式
    int m_screenStartHour;              ///< 定时开始小时
    int m_screenStartMinute;            ///< 定时开始分钟
    int m_screenEndHour;                ///< 定时结束小时
    int m_screenEndMinute;              ///< 定时结束分钟
    int m_screenTimeoutMinutes;         ///< 延时分钟数
    
    bool m_screenOn;                    ///< 屏幕开启状态
    uint32_t m_lastTouchTime;           ///< 最后触摸时间
    uint32_t m_lastScreenModeCheck;     ///< 最后屏幕模式检查时间
    
    // === 功率控制屏幕成员变量 ===
    bool m_powerControlEnabled;         ///< 是否启用功率控制
    int m_powerOffThreshold;            ///< 功率关闭阈值（mW）
    int m_powerOnThreshold;             ///< 功率开启阈值（mW）
    uint32_t m_lowPowerStartTime;       ///< 低功率开始时间
    
    // === OTA升级进度成员变量 ===
    lv_obj_t* m_otaScreen;              ///< OTA进度屏幕对象
    lv_obj_t* m_otaProgressBar;         ///< OTA进度条对象
    lv_obj_t* m_otaStatusLabel;         ///< OTA状态标签对象
    lv_obj_t* m_otaProgressLabel;       ///< OTA进度百分比标签对象
    lv_obj_t* m_otaSizeLabel;           ///< OTA文件大小标签对象
    lv_obj_t* m_otaErrorLabel;          ///< OTA错误信息标签对象
    DisplayPage m_previousPageForOTA;   ///< OTA开始前的页面，用于恢复
    bool m_otaDisplayActive;            ///< OTA显示是否激活
    bool m_isInLowPowerMode;            ///< 是否处于低功率模式
    
    // === 自动切换端口成员变量 ===
    bool m_autoSwitchEnabled;           ///< 是否启用自动切换端口功能
    int m_previousPortPower[4];         ///< 上一次端口功率值（mW）
    bool m_previousPortState[4];        ///< 上一次端口状态
    DisplayPage m_previousPage;         ///< 切换前的页面
    uint32_t m_autoSwitchStartTime;     ///< 自动切换开始时间
    uint32_t m_autoSwitchDuration;      ///< 自动切换持续时间
    bool m_isInAutoSwitchMode;          ///< 是否处于自动切换模式
    int m_currentAutoSwitchPort;        ///< 当前自动切换的端口索引
    uint32_t m_lastAutoSwitchTime[4];   ///< 每个端口最后一次自动切换的时间
    
    // === 基于总功率的自动页面切换成员变量 ===
    bool m_powerBasedAutoSwitchEnabled; ///< 是否启用基于总功率的自动页面切换
    int m_lowPowerThreshold;            ///< 低功率阈值（mW），小于此值显示待机页面
    int m_highPowerThreshold;           ///< 高功率阈值（mW），大于此值显示总功率页面
    bool m_isManualSwitch;              ///< 是否为手动切换
    DisplayPage m_lastManualPage;       ///< 最后一次手动切换的页面
    uint32_t m_lastPowerCheckTime;      ///< 最后一次功率检查时间
    uint32_t m_powerCheckInterval;      ///< 功率检查间隔（毫秒）
    int m_lastTotalPower;               ///< 上一次总功率值（mW）
    uint32_t m_lastPageSwitchTime;      ///< 最后一次页面切换时间，用于10秒冷却
    DisplayPage m_pendingTargetPage;    ///< 待切换的目标页面
    uint32_t m_pageChangeStartTime;     ///< 页面变化开始时间，用于5秒持续检查
    bool m_isPageChangePending;         ///< 是否有待处理的页面变化
    
    // === 亮度渐变成员变量 ===
    bool m_fadingEnabled;               ///< 是否启用亮度渐变
    uint8_t m_currentFadingBrightness;  ///< 当前渐变亮度值
    uint8_t m_targetFadingBrightness;   ///< 目标渐变亮度值
    uint32_t m_fadeStartTime;           ///< 渐变开始时间
    uint32_t m_fadeDuration;            ///< 渐变持续时间（毫秒）
    bool m_isFading;                    ///< 是否正在渐变中
    FadeDirection m_fadeDirection;      ///< 渐变方向
    
    // 任务配置
    static const uint32_t TASK_STACK_SIZE = 9 * 1024;    ///< 任务栈大小
    static const UBaseType_t TASK_PRIORITY = 3;          ///< 任务优先级
    static const BaseType_t TASK_CORE = 0;               ///< 任务运行核心
    static const uint32_t MESSAGE_QUEUE_SIZE = 10;       ///< 消息队列大小
    
    // 屏幕模式相关常量
    static const uint32_t SCREEN_MODE_CHECK_INTERVAL = 1000;  ///< 屏幕模式检查间隔（毫秒）
    static const uint32_t TOUCH_TIMEOUT_MARGIN = 5000;        ///< 触摸超时边距（毫秒）
};

#endif // DISPLAY_MANAGER_H 