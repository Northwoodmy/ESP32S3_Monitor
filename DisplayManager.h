/*
 * DisplayManager - 显示管理器
 * 
 * 功能说明：
 * - 管理LVGL显示界面和页面切换
 * - 处理系统状态显示和更新
 * - 管理显示模式和主题切换
 * - 提供高级显示功能接口
 * - WiFi状态、系统信息显示
 * - 触摸按钮和交互处理
 * 
 * 设计特点：
 * - 模块化C++面向对象设计
 * - FreeRTOS任务管理
 * - 线程安全的显示更新
 * - 可扩展的页面管理系统
 */

#ifndef DISPLAY_MANAGER_H
#define DISPLAY_MANAGER_H

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"
#include "lvgl.h"
#include "LVGL_Driver.h"

// 前向声明
class WiFiManager;
class ConfigStorage;

/**
 * @brief 显示页面枚举
 */
enum DisplayPage {
    PAGE_HOME = 0,      ///< 主页面
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
    THEME_LIGHT = 0,    ///< 浅色主题
    THEME_DARK,         ///< 深色主题
    THEME_AUTO          ///< 自动主题
};

/**
 * @brief 显示消息结构
 */
struct DisplayMessage {
    enum MessageType {
        MSG_UPDATE_WIFI_STATUS,     ///< 更新WiFi状态
        MSG_UPDATE_SYSTEM_INFO,     ///< 更新系统信息
        MSG_SWITCH_PAGE,            ///< 切换页面
        MSG_SET_BRIGHTNESS,         ///< 设置亮度
        MSG_SET_THEME,              ///< 设置主题
        MSG_SHOW_NOTIFICATION       ///< 显示通知
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
    } data;
};

/**
 * @brief 显示管理器类
 * 
 * 负责管理ESP32S3监控系统的所有显示功能，包括：
 * - 多页面UI管理
 * - 实时状态显示
 * - 主题和亮度控制
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
     * @return true 初始化成功，false 初始化失败
     */
    bool init(LVGLDriver* lvgl_driver, WiFiManager* wifi_manager, ConfigStorage* config_storage);
    
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
     * @brief 初始化页面容器
     */
    void initPageContainers();
    
    /**
     * @brief 清理页面资源
     * 
     * @param page 要清理的页面
     */
    void cleanupPage(DisplayPage page);

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
    
    // 显示状态
    DisplayPage m_currentPage;          ///< 当前页面
    DisplayTheme m_currentTheme;        ///< 当前主题
    uint8_t m_brightness;               ///< 当前亮度
    
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
    
    // 任务配置
    static const uint32_t TASK_STACK_SIZE = 8 * 1024;   ///< 任务栈大小
    static const UBaseType_t TASK_PRIORITY = 3;         ///< 任务优先级
    static const uint32_t TASK_CORE = 1;                ///< 任务运行核心
    static const uint32_t MESSAGE_QUEUE_SIZE = 10;      ///< 消息队列大小
};

#endif // DISPLAY_MANAGER_H 