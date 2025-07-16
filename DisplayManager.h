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
        MSG_SCREEN_OFF              ///< 屏幕关闭
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
        } screen_mode;
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
     */
    void setScreenMode(ScreenMode mode, int startHour = 8, int startMinute = 0, 
                       int endHour = 22, int endMinute = 0, int timeoutMinutes = 10);
    
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
    
    /**
     * @brief 创建主页面UI
     */
    void createHomePage();
    
    /**
     * @brief 创建WiFi状态页面UI
     */
    void createWiFiStatusPage();
    
    /**
     * @brief 创建系统信息页面UI
     */
    void createSystemInfoPage();
    
    /**
     * @brief 创建设置页面UI
     */
    void createSettingsPage();
    
    /**
     * @brief 创建关于页面UI
     */
    void createAboutPage();
    
    /**
     * @brief 创建总功率页面UI
     */
    void createTotalPowerPage();
    
    /**
     * @brief 创建端口功率页面UI
     * 
     * @param port_index 端口索引（1-4）
     */
    void createPortPowerPage(int port_index);
    
    /**
     * @brief 更新功率显示
     */
    void updatePowerDisplay();
    
    /**
     * @brief 设置页面滑动事件
     * 
     * @param page 页面对象
     */
    void setupPageSwipeEvents(lv_obj_t* page);
    
    /**
     * @brief 滑动事件回调
     * 
     * @param event 事件对象
     */
    static void swipeEventCallback(lv_event_t* event);
    
    /**
     * @brief 创建导航栏
     */
    void createNavigationBar();
    
    /**
     * @brief 创建状态栏
     */
    void createStatusBar();
    
    /**
     * @brief 更新状态栏信息
     */
    void updateStatusBar();
    
    /**
     * @brief 按钮事件回调函数
     * 
     * @param event 事件对象
     */
    static void buttonEventCallback(lv_event_t* event);
    
    /**
     * @brief 页面切换动画回调
     * 
     * @param animation 动画对象
     */
    static void pageAnimationCallback(lv_anim_t* animation);
    
    /**
     * @brief 应用主题样式
     */
    void applyTheme();
    
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
    
    /**
     * @brief 初始化页面容器
     */
    void initPageContainers();
    
    /**
     * @brief 清理页面资源
     * 
     * @param page 要清理的页面
     */
    void cleanupPage(DisplayPage page);
    
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

private:
    // 成员变量
    bool m_initialized;                 ///< 初始化状态
    bool m_running;                     ///< 运行状态
    TaskHandle_t m_taskHandle;          ///< 任务句柄
    QueueHandle_t m_messageQueue;       ///< 消息队列
    
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
    lv_obj_t* m_pageContainer;          ///< 页面容器
    lv_obj_t* m_navigationBar;          ///< 导航栏
    lv_obj_t* m_statusBar;              ///< 状态栏
    lv_obj_t* m_notificationBar;        ///< 通知栏
    
    // 页面对象数组
    lv_obj_t* m_pages[PAGE_COUNT];      ///< 页面对象数组
    
    // 状态栏组件
    lv_obj_t* m_wifiIcon;               ///< WiFi图标
    lv_obj_t* m_timeLabel;              ///< 时间标签
    lv_obj_t* m_batteryIcon;            ///< 电池图标（预留）
    
    // 通知相关
    lv_obj_t* m_notificationLabel;      ///< 通知标签
    TaskHandle_t m_notificationTimer;   ///< 通知定时器
    
    // 功率监控相关
    PowerMonitorData m_powerData;       ///< 功率监控数据
    lv_obj_t* m_powerLabels[5][10];     ///< 功率显示标签[页面][标签]
    lv_obj_t* m_statusIndicators[4];    ///< 端口状态指示器数组（总功率页面）
    lv_obj_t* m_portStatusDots[4];      ///< 端口页面状态指示器数组
    int m_currentPortPage;              ///< 当前端口页面索引
    
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
    bool m_isInLowPowerMode;            ///< 是否处于低功率模式
    
    // 任务配置
    static const uint32_t TASK_STACK_SIZE = 8 * 1024;    ///< 任务栈大小
    static const UBaseType_t TASK_PRIORITY = 3;          ///< 任务优先级
    static const BaseType_t TASK_CORE = 0;               ///< 任务运行核心
    static const uint32_t MESSAGE_QUEUE_SIZE = 10;       ///< 消息队列大小
    
    // 屏幕模式相关常量
    static const uint32_t SCREEN_MODE_CHECK_INTERVAL = 1000;  ///< 屏幕模式检查间隔（毫秒）
    static const uint32_t TOUCH_TIMEOUT_MARGIN = 5000;        ///< 触摸超时边距（毫秒）
};

#endif // DISPLAY_MANAGER_H 