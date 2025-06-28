/*
 * DisplayManager - 显示管理器实现
 * 
 * 该文件实现了ESP32S3监控系统的显示管理功能，包括：
 * - 多页面UI管理和切换
 * - 实时状态显示更新
 * - 主题和亮度控制
 * - 触摸交互事件处理
 * - 通知消息显示
 * 
 * 技术特点：
 * - FreeRTOS任务调度
 * - 线程安全的消息队列
 * - LVGL UI组件管理
 * - 模块化页面设计
 */

#include "DisplayManager.h"
#include "WiFiManager.h"
#include "ConfigStorage.h"
#include <WiFi.h>
#include <cstring>
#include <cstdio>

/**
 * @brief 构造函数
 */
DisplayManager::DisplayManager() 
    : m_initialized(false)
    , m_running(false)
    , m_taskHandle(nullptr)
    , m_messageQueue(nullptr)
    , m_lvglDriver(nullptr)
    , m_wifiManager(nullptr)
    , m_configStorage(nullptr)
    , m_currentPage(PAGE_HOME)
    , m_currentTheme(THEME_LIGHT)
    , m_brightness(80)
    , m_screen(nullptr)
    , m_pageContainer(nullptr)
    , m_navigationBar(nullptr)
    , m_statusBar(nullptr)
    , m_notificationBar(nullptr)
    , m_wifiIcon(nullptr)
    , m_timeLabel(nullptr)
    , m_batteryIcon(nullptr)
    , m_notificationLabel(nullptr)
    , m_notificationTimer(nullptr)
{
    // 初始化页面对象数组
    for (int i = 0; i < PAGE_COUNT; i++) {
        m_pages[i] = nullptr;
    }
    
    printf("[DisplayManager] 显示管理器已创建\n");
}

/**
 * @brief 析构函数
 */
DisplayManager::~DisplayManager() {
    stop();
    
    // 清理消息队列
    if (m_messageQueue) {
        vQueueDelete(m_messageQueue);
        m_messageQueue = nullptr;
    }
    
    printf("[DisplayManager] 显示管理器已销毁\n");
}

/**
 * @brief 初始化显示管理器
 */
bool DisplayManager::init(LVGLDriver* lvgl_driver, WiFiManager* wifi_manager, ConfigStorage* config_storage) {
    if (m_initialized) {
        printf("[DisplayManager] 警告：重复初始化\n");
        return true;
    }
    
    if (!lvgl_driver || !wifi_manager || !config_storage) {
        printf("[DisplayManager] 错误：无效的依赖参数\n");
        return false;
    }
    
    printf("[DisplayManager] 开始初始化显示管理器...\n");
    
    // 保存依赖对象
    m_lvglDriver = lvgl_driver;
    m_wifiManager = wifi_manager;
    m_configStorage = config_storage;
    
    // 创建消息队列
    m_messageQueue = xQueueCreate(MESSAGE_QUEUE_SIZE, sizeof(DisplayMessage));
    if (!m_messageQueue) {
        printf("[DisplayManager] 错误：创建消息队列失败\n");
        return false;
    }
    
    // 检查LVGL驱动是否已初始化
    if (!m_lvglDriver->isInitialized()) {
        printf("[DisplayManager] 错误：LVGL驱动未初始化\n");
        return false;
    }
    
    // 获取LVGL锁并创建UI
    if (m_lvglDriver->lock(5000)) {
        // 创建主屏幕
        m_screen = lv_scr_act();
        if (!m_screen) {
            printf("[DisplayManager] 错误：获取主屏幕失败\n");
            m_lvglDriver->unlock();
            return false;
        }
        
        // 初始化页面容器
        initPageContainers();
        
        // 创建状态栏
        createStatusBar();
        
        // 创建导航栏
        createNavigationBar();
        
        // 创建主页面
        createHomePage();
        
        // 应用默认主题
        applyTheme();
        
        m_lvglDriver->unlock();
        
        printf("[DisplayManager] UI界面创建完成\n");
    } else {
        printf("[DisplayManager] 错误：获取LVGL锁超时\n");
        return false;
    }
    
    m_initialized = true;
    printf("[DisplayManager] 显示管理器初始化完成\n");
    return true;
}

/**
 * @brief 启动显示管理器任务
 */
bool DisplayManager::start() {
    if (!m_initialized) {
        printf("[DisplayManager] 错误：未初始化，无法启动任务\n");
        return false;
    }
    
    if (m_running) {
        printf("[DisplayManager] 警告：任务已在运行\n");
        return true;
    }
    
    // 创建显示管理器任务
    BaseType_t result = xTaskCreatePinnedToCore(
        displayTaskEntry,           // 任务函数
        "DisplayManager",           // 任务名称
        TASK_STACK_SIZE,           // 栈大小
        this,                      // 任务参数
        TASK_PRIORITY,             // 任务优先级
        &m_taskHandle,             // 任务句柄
        TASK_CORE                  // 运行核心
    );
    
    if (result != pdPASS) {
        printf("[DisplayManager] 错误：创建任务失败\n");
        return false;
    }
    
    m_running = true;
    printf("[DisplayManager] 显示管理器任务已启动（核心%d，优先级%d）\n", TASK_CORE, TASK_PRIORITY);
    return true;
}

/**
 * @brief 停止显示管理器任务
 */
void DisplayManager::stop() {
    if (!m_running) {
        return;
    }
    
    m_running = false;
    
    // 等待任务结束
    if (m_taskHandle) {
        vTaskDelete(m_taskHandle);
        m_taskHandle = nullptr;
        printf("[DisplayManager] 显示管理器任务已停止\n");
    }
}

/**
 * @brief 显示管理器任务静态入口
 */
void DisplayManager::displayTaskEntry(void* arg) {
    DisplayManager* manager = static_cast<DisplayManager*>(arg);
    if (manager) {
        manager->displayTask();
    }
    vTaskDelete(nullptr);
}

/**
 * @brief 显示管理器任务执行函数
 */
void DisplayManager::displayTask() {
    printf("[DisplayManager] 显示管理器任务开始运行\n");
    
    DisplayMessage msg;
    TickType_t lastUpdateTime = 0;
    const TickType_t updateInterval = pdMS_TO_TICKS(1000); // 1秒更新间隔
    
    while (m_running) {
        // 处理消息队列中的消息
        if (xQueueReceive(m_messageQueue, &msg, pdMS_TO_TICKS(100)) == pdTRUE) {
            processMessage(msg);
        }
        
        // 定期更新状态栏
        TickType_t currentTime = xTaskGetTickCount();
        if (currentTime - lastUpdateTime >= updateInterval) {
            if (m_lvglDriver->lock(100)) {
                updateStatusBar();
                m_lvglDriver->unlock();
            }
            lastUpdateTime = currentTime;
        }
        
        // 短暂延迟，避免占用过多CPU
        vTaskDelay(pdMS_TO_TICKS(50));
    }
    
    printf("[DisplayManager] 显示管理器任务结束\n");
}

/**
 * @brief 处理显示消息
 */
void DisplayManager::processMessage(const DisplayMessage& msg) {
    if (!m_lvglDriver->lock(1000)) {
        printf("[DisplayManager] 警告：处理消息时获取LVGL锁失败\n");
        return;
    }
    
    switch (msg.type) {
        case DisplayMessage::MSG_UPDATE_WIFI_STATUS:
            // 更新WiFi状态显示
            // 这里可以更新WiFi图标和相关信息
            printf("[DisplayManager] 更新WiFi状态：%s\n", 
                   msg.data.wifi_status.connected ? "已连接" : "未连接");
            break;
            
        case DisplayMessage::MSG_UPDATE_SYSTEM_INFO:
            // 更新系统信息显示
            printf("[DisplayManager] 更新系统信息：内存=%d KB，运行时间=%d秒\n",
                   msg.data.system_info.free_heap / 1024,
                   msg.data.system_info.uptime);
            break;
            
        case DisplayMessage::MSG_SWITCH_PAGE:
            // 切换页面
            if (msg.data.page_switch.page < PAGE_COUNT) {
                m_currentPage = msg.data.page_switch.page;
                // 这里实现页面切换逻辑
                printf("[DisplayManager] 切换到页面：%d\n", m_currentPage);
            }
            break;
            
        case DisplayMessage::MSG_SET_BRIGHTNESS:
            // 设置亮度
            m_brightness = msg.data.brightness.brightness;
            m_lvglDriver->setBrightness(m_brightness);
            printf("[DisplayManager] 设置亮度：%d%%\n", m_brightness);
            break;
            
        case DisplayMessage::MSG_SET_THEME:
            // 设置主题
            m_currentTheme = msg.data.theme.theme;
            applyTheme();
            printf("[DisplayManager] 设置主题：%d\n", m_currentTheme);
            break;
            
        case DisplayMessage::MSG_SHOW_NOTIFICATION:
            // 显示通知
            if (m_notificationLabel) {
                lv_label_set_text(m_notificationLabel, msg.data.notification.text);
                lv_obj_clear_flag(m_notificationBar, LV_OBJ_FLAG_HIDDEN);
                printf("[DisplayManager] 显示通知：%s\n", msg.data.notification.text);
                
                // 设置通知自动隐藏定时器（简化实现）
                // 实际项目中可以使用LVGL动画或定时器
            }
            break;
    }
    
    m_lvglDriver->unlock();
}

/**
 * @brief 初始化页面容器
 */
void DisplayManager::initPageContainers() {
    // 创建主页面容器
    m_pageContainer = lv_obj_create(m_screen);
    lv_obj_set_size(m_pageContainer, LV_HOR_RES, LV_VER_RES - 60); // 预留状态栏和导航栏空间
    lv_obj_set_pos(m_pageContainer, 0, 30); // 状态栏高度30
    lv_obj_set_style_bg_color(m_pageContainer, lv_color_white(), 0);
    lv_obj_set_style_border_width(m_pageContainer, 0, 0);
    lv_obj_set_style_pad_all(m_pageContainer, 0, 0);
}

/**
 * @brief 创建状态栏
 */
void DisplayManager::createStatusBar() {
    // 创建状态栏容器
    m_statusBar = lv_obj_create(m_screen);
    lv_obj_set_size(m_statusBar, LV_HOR_RES, 30);
    lv_obj_set_pos(m_statusBar, 0, 0);
    lv_obj_set_style_bg_color(m_statusBar, lv_color_hex(0x2196F3), 0);
    lv_obj_set_style_border_width(m_statusBar, 0, 0);
    lv_obj_set_style_pad_all(m_statusBar, 5, 0);
    
    // WiFi图标（简化为文本标签）
    m_wifiIcon = lv_label_create(m_statusBar);
    lv_label_set_text(m_wifiIcon, LV_SYMBOL_WIFI);
    lv_obj_set_style_text_color(m_wifiIcon, lv_color_white(), 0);
    lv_obj_align(m_wifiIcon, LV_ALIGN_LEFT_MID, 0, 0);
    
    // 时间标签
    m_timeLabel = lv_label_create(m_statusBar);
    lv_label_set_text(m_timeLabel, "00:00");
    lv_obj_set_style_text_color(m_timeLabel, lv_color_white(), 0);
    lv_obj_align(m_timeLabel, LV_ALIGN_CENTER, 0, 0);
    
    // 电池图标（预留）
    m_batteryIcon = lv_label_create(m_statusBar);
    lv_label_set_text(m_batteryIcon, LV_SYMBOL_BATTERY_FULL);
    lv_obj_set_style_text_color(m_batteryIcon, lv_color_white(), 0);
    lv_obj_align(m_batteryIcon, LV_ALIGN_RIGHT_MID, 0, 0);
}

/**
 * @brief 创建导航栏
 */
void DisplayManager::createNavigationBar() {
    // 创建导航栏容器
    m_navigationBar = lv_obj_create(m_screen);
    lv_obj_set_size(m_navigationBar, LV_HOR_RES, 30);
    lv_obj_set_pos(m_navigationBar, 0, LV_VER_RES - 30);
    lv_obj_set_style_bg_color(m_navigationBar, lv_color_hex(0xF5F5F5), 0);
    lv_obj_set_style_border_width(m_navigationBar, 1, 0);
    lv_obj_set_style_border_color(m_navigationBar, lv_color_hex(0xE0E0E0), 0);
    lv_obj_set_style_pad_all(m_navigationBar, 2, 0);
    
    // 创建导航按钮
    const char* nav_labels[] = {"主页", "WiFi", "系统", "设置", "关于"};
    int btn_width = LV_HOR_RES / PAGE_COUNT;
    
    for (int i = 0; i < PAGE_COUNT; i++) {
        lv_obj_t* btn = lv_btn_create(m_navigationBar);
        lv_obj_set_size(btn, btn_width - 4, 26);
        lv_obj_set_pos(btn, i * btn_width + 2, 2);
        lv_obj_set_style_bg_color(btn, lv_color_hex(0x2196F3), 0);
        lv_obj_set_style_radius(btn, 3, 0);
        
        lv_obj_t* label = lv_label_create(btn);
        lv_label_set_text(label, nav_labels[i]);
        lv_obj_set_style_text_color(label, lv_color_white(), 0);
        lv_obj_center(label);
        
        // 设置按钮事件回调
        lv_obj_add_event_cb(btn, buttonEventCallback, LV_EVENT_CLICKED, (void*)(intptr_t)i);
    }
}

/**
 * @brief 创建主页面UI
 */
void DisplayManager::createHomePage() {
    // 创建主页面
    m_pages[PAGE_HOME] = lv_obj_create(m_pageContainer);
    lv_obj_set_size(m_pages[PAGE_HOME], LV_HOR_RES, LV_VER_RES - 60);
    lv_obj_set_style_bg_color(m_pages[PAGE_HOME], lv_color_white(), 0);
    lv_obj_set_style_border_width(m_pages[PAGE_HOME], 0, 0);
    lv_obj_set_style_pad_all(m_pages[PAGE_HOME], 10, 0);
    
    // 标题标签
    lv_obj_t* title = lv_label_create(m_pages[PAGE_HOME]);
    lv_label_set_text(title, "ESP32S3 监控系统");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(title, lv_color_hex(0x2196F3), 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 20);
    
    // 状态信息标签
    lv_obj_t* status_label = lv_label_create(m_pages[PAGE_HOME]);
    lv_label_set_text(status_label, "系统运行正常");
    lv_obj_align(status_label, LV_ALIGN_CENTER, 0, -50);
    
    // 快捷操作按钮
    lv_obj_t* wifi_btn = lv_btn_create(m_pages[PAGE_HOME]);
    lv_obj_set_size(wifi_btn, 120, 40);
    lv_obj_align(wifi_btn, LV_ALIGN_CENTER, -70, 20);
    lv_obj_set_style_bg_color(wifi_btn, lv_color_hex(0x4CAF50), 0);
    
    lv_obj_t* wifi_label = lv_label_create(wifi_btn);
    lv_label_set_text(wifi_label, "WiFi状态");
    lv_obj_set_style_text_color(wifi_label, lv_color_white(), 0);
    lv_obj_center(wifi_label);
    
    lv_obj_t* info_btn = lv_btn_create(m_pages[PAGE_HOME]);
    lv_obj_set_size(info_btn, 120, 40);
    lv_obj_align(info_btn, LV_ALIGN_CENTER, 70, 20);
    lv_obj_set_style_bg_color(info_btn, lv_color_hex(0xFF9800), 0);
    
    lv_obj_t* info_label = lv_label_create(info_btn);
    lv_label_set_text(info_label, "系统信息");
    lv_obj_set_style_text_color(info_label, lv_color_white(), 0);
    lv_obj_center(info_label);
    
         // 版本信息
     lv_obj_t* version_label = lv_label_create(m_pages[PAGE_HOME]);
     lv_label_set_text(version_label, "版本: v4.0.0");
     lv_obj_set_style_text_color(version_label, lv_color_hex(0x757575), 0);
     lv_obj_align(version_label, LV_ALIGN_BOTTOM_MID, 0, -20);
}

/**
 * @brief 更新状态栏信息
 */
void DisplayManager::updateStatusBar() {
    if (!m_timeLabel || !m_wifiIcon) {
        return;
    }
    
    // 更新时间显示（简化实现）
    uint32_t uptime = millis() / 1000;
    uint32_t hours = uptime / 3600;
    uint32_t minutes = (uptime % 3600) / 60;
    char time_str[16];
    snprintf(time_str, sizeof(time_str), "%02d:%02d", hours % 24, minutes);
    lv_label_set_text(m_timeLabel, time_str);
    
    // 更新WiFi图标
    if (m_wifiManager && m_wifiManager->isConnected()) {
        lv_obj_set_style_text_color(m_wifiIcon, lv_color_white(), 0);
    } else {
        lv_obj_set_style_text_color(m_wifiIcon, lv_color_hex(0xFFCDD2), 0);
    }
}

/**
 * @brief 按钮事件回调函数
 */
void DisplayManager::buttonEventCallback(lv_event_t* event) {
    lv_obj_t* btn = lv_event_get_target(event);
    int page_index = (int)(intptr_t)lv_event_get_user_data(event);
    
    printf("[DisplayManager] 导航按钮点击：页面%d\n", page_index);
    
    // 这里可以发送切换页面消息到消息队列
    // 由于需要获取DisplayManager实例，实际实现中可能需要不同的方法
}

/**
 * @brief 应用主题样式
 */
void DisplayManager::applyTheme() {
    // 根据当前主题设置颜色方案
    lv_color_t primary_color, bg_color, text_color;
    
    switch (m_currentTheme) {
        case THEME_LIGHT:
            primary_color = lv_color_hex(0x2196F3);
            bg_color = lv_color_white();
            text_color = lv_color_black();
            break;
            
        case THEME_DARK:
            primary_color = lv_color_hex(0x1976D2);
            bg_color = lv_color_hex(0x303030);
            text_color = lv_color_white();
            break;
            
        case THEME_AUTO:
        default:
            // 简化实现：默认使用浅色主题
            primary_color = lv_color_hex(0x2196F3);
            bg_color = lv_color_white();
            text_color = lv_color_black();
            break;
    }
    
    // 应用主题到页面容器
    if (m_pageContainer) {
        lv_obj_set_style_bg_color(m_pageContainer, bg_color, 0);
    }
}

// 其他页面创建函数的简化实现
void DisplayManager::createWiFiStatusPage() {
    // WiFi状态页面实现
}

void DisplayManager::createSystemInfoPage() {
    // 系统信息页面实现
}

void DisplayManager::createSettingsPage() {
    // 设置页面实现
}

void DisplayManager::createAboutPage() {
    // 关于页面实现
}

void DisplayManager::cleanupPage(DisplayPage page) {
    // 页面清理实现
}

// 公共接口实现
void DisplayManager::switchPage(DisplayPage page) {
    DisplayMessage msg;
    msg.type = DisplayMessage::MSG_SWITCH_PAGE;
    msg.data.page_switch.page = page;
    
    if (m_messageQueue) {
        xQueueSend(m_messageQueue, &msg, pdMS_TO_TICKS(100));
    }
}

void DisplayManager::updateWiFiStatus(bool connected, const char* ssid, const char* ip, int rssi) {
    DisplayMessage msg;
    msg.type = DisplayMessage::MSG_UPDATE_WIFI_STATUS;
    msg.data.wifi_status.connected = connected;
    msg.data.wifi_status.rssi = rssi;
    
    if (ssid) {
        strncpy(msg.data.wifi_status.ssid, ssid, sizeof(msg.data.wifi_status.ssid) - 1);
        msg.data.wifi_status.ssid[sizeof(msg.data.wifi_status.ssid) - 1] = '\0';
    }
    
    if (ip) {
        strncpy(msg.data.wifi_status.ip, ip, sizeof(msg.data.wifi_status.ip) - 1);
        msg.data.wifi_status.ip[sizeof(msg.data.wifi_status.ip) - 1] = '\0';
    }
    
    if (m_messageQueue) {
        xQueueSend(m_messageQueue, &msg, pdMS_TO_TICKS(100));
    }
}

void DisplayManager::updateSystemInfo(uint32_t free_heap, uint32_t uptime, float cpu_usage) {
    DisplayMessage msg;
    msg.type = DisplayMessage::MSG_UPDATE_SYSTEM_INFO;
    msg.data.system_info.free_heap = free_heap;
    msg.data.system_info.uptime = uptime;
    msg.data.system_info.cpu_usage = cpu_usage;
    
    if (m_messageQueue) {
        xQueueSend(m_messageQueue, &msg, pdMS_TO_TICKS(100));
    }
}

void DisplayManager::setBrightness(uint8_t brightness) {
    DisplayMessage msg;
    msg.type = DisplayMessage::MSG_SET_BRIGHTNESS;
    msg.data.brightness.brightness = brightness;
    
    if (m_messageQueue) {
        xQueueSend(m_messageQueue, &msg, pdMS_TO_TICKS(100));
    }
}

void DisplayManager::setTheme(DisplayTheme theme) {
    DisplayMessage msg;
    msg.type = DisplayMessage::MSG_SET_THEME;
    msg.data.theme.theme = theme;
    
    if (m_messageQueue) {
        xQueueSend(m_messageQueue, &msg, pdMS_TO_TICKS(100));
    }
}

void DisplayManager::showNotification(const char* text, uint32_t duration_ms) {
    if (!text) return;
    
    DisplayMessage msg;
    msg.type = DisplayMessage::MSG_SHOW_NOTIFICATION;
    msg.data.notification.duration_ms = duration_ms;
    
    strncpy(msg.data.notification.text, text, sizeof(msg.data.notification.text) - 1);
    msg.data.notification.text[sizeof(msg.data.notification.text) - 1] = '\0';
    
    if (m_messageQueue) {
        xQueueSend(m_messageQueue, &msg, pdMS_TO_TICKS(100));
    }
}

// 获取器实现
uint8_t DisplayManager::getBrightness() const {
    return m_brightness;
}

DisplayTheme DisplayManager::getTheme() const {
    return m_currentTheme;
}

DisplayPage DisplayManager::getCurrentPage() const {
    return m_currentPage;
}

bool DisplayManager::isInitialized() const {
    return m_initialized;
}

bool DisplayManager::isRunning() const {
    return m_running;
} 