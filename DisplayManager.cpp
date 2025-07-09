/*
 * DisplayManager - 显示管理器实现
 * 
 * 该文件实现了ESP32S3监控系统的显示管理功能，包括：
 * - 多页面UI管理和切换
 * - 实时状态显示更新
 * - 主题和亮度控制
 * - 触摸交互事件处理
 * - 通知消息显示
 * - 集成SquareLine Studio生成的UI系统
 * 
 * 技术特点：
 * - FreeRTOS任务调度
 * - 线程安全的消息队列
 * - LVGL UI组件管理
 * - 模块化页面设计
 * - 功率监控数据实时更新
 */

#include "DisplayManager.h"
#include "WiFiManager.h"
#include "ConfigStorage.h"
#include "PSRAMManager.h"
#include "TimeManager.h"
#include "WeatherManager.h"
#include <WiFi.h>
#include <cstring>
#include <cstdio>
#include <time.h>

// 外部声明新UI系统的屏幕对象
extern lv_obj_t * ui_standbySCREEN;
extern lv_obj_t * ui_totalpowerSCREEN;
extern lv_obj_t * ui_prot1SCREEN;
extern lv_obj_t * ui_prot2SCREEN;
extern lv_obj_t * ui_prot3SCREEN;
extern lv_obj_t * ui_prot4SCREEN;
extern lv_obj_t * ui_port1SCREEN12;
extern lv_obj_t * ui_port2SCREEN22;
extern lv_obj_t * ui_port3SCREEN32;
extern lv_obj_t * ui_port4SCREEN42;

// 外部声明新UI系统的标签对象
extern lv_obj_t * ui_timeLabel;
extern lv_obj_t * ui_dataLabel;
extern lv_obj_t * ui_weekLabel;
extern lv_obj_t * ui_totalpowerlabel;
extern lv_obj_t * ui_port1power;
extern lv_obj_t * ui_port2power;
extern lv_obj_t * ui_port3power;
extern lv_obj_t * ui_port4power;
extern lv_obj_t * ui_port1powerbar;
extern lv_obj_t * ui_port2powerbar;
extern lv_obj_t * ui_port3powerbar;
extern lv_obj_t * ui_port4powerbar;

// 外部声明天气相关组件
extern lv_obj_t * ui_temperatureLabel;
extern lv_obj_t * ui_weatherLabel;

// 外部声明端口功率页面标签
extern lv_obj_t * ui_port1powerlabel;
extern lv_obj_t * ui_port1voltage;
extern lv_obj_t * ui_port1current;
extern lv_obj_t * ui_port2powerlabel;
extern lv_obj_t * ui_port2voltage;
extern lv_obj_t * ui_port2current;
extern lv_obj_t * ui_port3powerlabel;
extern lv_obj_t * ui_port3voltage;
extern lv_obj_t * ui_port3current;
extern lv_obj_t * ui_port4powerlabel;
extern lv_obj_t * ui_port4voltage;
extern lv_obj_t * ui_port4current;

// 外部声明端口详细信息组件
// 端口1详细信息
extern lv_obj_t * ui_port1state;
extern lv_obj_t * ui_port1protocol;
extern lv_obj_t * ui_port1manufactuervid;
extern lv_obj_t * ui_port1cablevid;
extern lv_obj_t * ui_port1maxvbusvoltage;
extern lv_obj_t * ui_port1maxvbuscurrent;

// 端口2详细信息
extern lv_obj_t * ui_port2state;
extern lv_obj_t * ui_port2protocol;
extern lv_obj_t * ui_port2manufactuervid;
extern lv_obj_t * ui_port2cablevid;
extern lv_obj_t * ui_port2maxvbusvoltage;
extern lv_obj_t * ui_port2maxvbuscurrent;

// 端口3详细信息
extern lv_obj_t * ui_port3state;
extern lv_obj_t * ui_port3protocol;
extern lv_obj_t * ui_port3manufactuervid;
extern lv_obj_t * ui_port3cablevid;
extern lv_obj_t * ui_port3maxvbusvoltage;
extern lv_obj_t * ui_port3maxvbuscurrent;

// 端口4详细信息
extern lv_obj_t * ui_port4state;
extern lv_obj_t * ui_port4protocol;
extern lv_obj_t * ui_port4manufactuervid;
extern lv_obj_t * ui_port4cablevid;
extern lv_obj_t * ui_port4maxvbusvoltage;
extern lv_obj_t * ui_port4maxvbuscurrent;

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
    , m_psramManager(nullptr)
    , m_weatherManager(nullptr)
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
    , m_currentPortPage(0)
{
    // 初始化页面对象数组
    for (int i = 0; i < PAGE_COUNT; i++) {
        m_pages[i] = nullptr;
    }
    
    // 初始化功率标签数组
    for (int i = 0; i < 5; i++) {
        for (int j = 0; j < 10; j++) {
            m_powerLabels[i][j] = nullptr;
        }
    }
    
    // 初始化状态指示器数组
    for (int i = 0; i < 4; i++) {
        m_statusIndicators[i] = nullptr;
        m_portStatusDots[i] = nullptr;
    }
    
    // 初始化功率数据
    memset(&m_powerData, 0, sizeof(m_powerData));
    m_powerData.port_count = 4;
    m_powerData.valid = false;
    
    printf("[DisplayManager] 显示管理器已创建（新UI系统）\n");
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
bool DisplayManager::init(LVGLDriver* lvgl_driver, WiFiManager* wifi_manager, ConfigStorage* config_storage, PSRAMManager* psram_manager, WeatherManager* weather_manager) {
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
    m_psramManager = psram_manager;
    m_weatherManager = weather_manager;
    
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
    
    // 获取LVGL锁并初始化新的UI系统
    if (m_lvglDriver->lock(5000)) {
        // 初始化新的UI系统
        ui_init();
        
        // 获取主屏幕
        m_screen = lv_scr_act();
        if (!m_screen) {
            printf("[DisplayManager] 错误：获取主屏幕失败\n");
            m_lvglDriver->unlock();
            return false;
        }
        
        // 显示默认页面（待机屏幕）
        if (ui_standbySCREEN) {
            lv_scr_load(ui_standbySCREEN);
            printf("[DisplayManager] 显示默认页面：待机屏幕\n");
        }
        
        // 初始化时间和日期显示
        updateTimeDisplay();
        
        m_lvglDriver->unlock();
        
        printf("[DisplayManager] 新UI系统初始化完成\n");
    } else {
        printf("[DisplayManager] 错误：获取LVGL锁超时\n");
        return false;
    }
    
    // 从NVS加载保存的亮度设置
    if (m_configStorage->hasBrightnessConfigAsync(3000)) {
        m_brightness = m_configStorage->loadBrightnessAsync(3000);
        printf("[DisplayManager] 加载保存的亮度: %d%%\n", m_brightness);
        
        // 立即应用加载的亮度到硬件
        if (m_lvglDriver) {
            m_lvglDriver->setBrightness(m_brightness);
        }
    } else {
        printf("[DisplayManager] 使用默认亮度: %d%%\n", m_brightness);
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
    
    // 在创建任务之前设置m_running = true，避免竞态条件
    m_running = true;
    
    if (m_psramManager && m_psramManager->isPSRAMAvailable()) {
        // 使用PSRAM栈创建任务
        m_taskHandle = m_psramManager->createTaskWithPSRAMStack(
            displayTaskEntry,           // 任务函数
            "DisplayManager",           // 任务名称
            TASK_STACK_SIZE,           // 栈大小
            this,                      // 任务参数
            TASK_PRIORITY,             // 任务优先级
            TASK_CORE                  // 运行核心
        );
        
        if (m_taskHandle == nullptr) {
            printf("[DisplayManager] 错误：创建PSRAM栈任务失败\n");
            m_running = false;  // 失败时重置状态
            return false;
        }
        
        printf("[DisplayManager] 显示管理器任务(PSRAM栈)已启动\n");
    } else {
        // 回退到SRAM栈创建任务
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
            printf("[DisplayManager] 错误：创建SRAM栈任务失败\n");
            m_running = false;  // 失败时重置状态
            return false;
        }
        
        printf("[DisplayManager] 显示管理器任务(SRAM栈)已启动\n");
    }
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
        BaseType_t queueResult = xQueueReceive(m_messageQueue, &msg, pdMS_TO_TICKS(100));
        if (queueResult == pdTRUE) {
            processMessage(msg);
        }
        
        // 定期更新时间显示
        TickType_t currentTime = xTaskGetTickCount();
        if (currentTime - lastUpdateTime >= updateInterval) {
            // 更新时间显示
            updateTimeDisplay();
            
            // 更新天气显示
            updateWeatherDisplay();
            
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
            
        case DisplayMessage::MSG_UPDATE_POWER_DATA:
            // 功率数据已在updatePowerData中直接更新
            printf("[DisplayManager] 功率数据已更新：总功率=%d mW\n", m_powerData.total_power);
            break;
            
        case DisplayMessage::MSG_UPDATE_WEATHER_DATA:
            // 更新天气数据显示
            if (msg.data.weather_data.valid) {
                printf("[DisplayManager] 天气数据已更新：温度=%s，天气=%s\n", 
                       msg.data.weather_data.temperature, 
                       msg.data.weather_data.weather);
            } else {
                printf("[DisplayManager] 天气数据无效\n");
            }
            break;
            
        case DisplayMessage::MSG_SWITCH_PAGE:
            // 切换页面
            if (msg.data.page_switch.page < PAGE_COUNT) {
                DisplayPage newPage = msg.data.page_switch.page;
                
                // 隐藏当前页面
                if (m_pages[m_currentPage] != nullptr) {
                    lv_obj_add_flag(m_pages[m_currentPage], LV_OBJ_FLAG_HIDDEN);
                }
                
                // 显示新页面
                if (m_pages[newPage] != nullptr) {
                    lv_obj_clear_flag(m_pages[newPage], LV_OBJ_FLAG_HIDDEN);
                    m_currentPage = newPage;
                    printf("[DisplayManager] 切换到页面：%d\n", m_currentPage);
                } else {
                    printf("[DisplayManager] 警告：页面%d未创建\n", newPage);
                }
            }
            break;
            
        case DisplayMessage::MSG_SET_BRIGHTNESS:
            // 设置亮度
            m_brightness = msg.data.brightness.brightness;
            
            // 异步保存亮度到NVS
            if (m_configStorage) {
                bool saveSuccess = m_configStorage->saveBrightnessAsync(m_brightness, 3000);
                if (saveSuccess) {
                    printf("[DisplayManager] 亮度设置已保存: %d%%\n", m_brightness);
                } else {
                    printf("[DisplayManager] 亮度设置保存失败\n");
                }
            }
            
            // 应用亮度到硬件
            if (m_lvglDriver) {
                m_lvglDriver->setBrightness(m_brightness);
            }
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
    
    // 设置用户数据，以便按钮事件回调可以找到DisplayManager实例
    lv_obj_set_user_data(m_pageContainer, this);
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
    
    // 设置用户数据
    lv_obj_set_user_data(m_navigationBar, this);
    
    // 创建导航按钮
    const char* nav_labels[] = {"Home", "Total", "Port1", "Port2", "Port3", "Port4", "WiFi", "System", "Settings", "About"};
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
    lv_label_set_text(title, "ESP32S3 Power Monitor");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(title, lv_color_hex(0x2196F3), 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 20);
    
    // 状态信息标签
    lv_obj_t* status_label = lv_label_create(m_pages[PAGE_HOME]);
    lv_label_set_text(status_label, "System Running");
    lv_obj_align(status_label, LV_ALIGN_CENTER, 0, -50);
    
    // 快捷操作按钮
    lv_obj_t* power_btn = lv_btn_create(m_pages[PAGE_HOME]);
    lv_obj_set_size(power_btn, 120, 40);
    lv_obj_align(power_btn, LV_ALIGN_CENTER, -70, 20);
    lv_obj_set_style_bg_color(power_btn, lv_color_hex(0x4CAF50), 0);
    
    lv_obj_t* power_label = lv_label_create(power_btn);
    lv_label_set_text(power_label, "Power Data");
    lv_obj_set_style_text_color(power_label, lv_color_white(), 0);
    lv_obj_center(power_label);
    
    // 添加点击事件，切换到总功率页面
    lv_obj_add_event_cb(power_btn, buttonEventCallback, LV_EVENT_CLICKED, (void*)(intptr_t)PAGE_POWER_TOTAL);
    
    lv_obj_t* info_btn = lv_btn_create(m_pages[PAGE_HOME]);
    lv_obj_set_size(info_btn, 120, 40);
    lv_obj_align(info_btn, LV_ALIGN_CENTER, 70, 20);
    lv_obj_set_style_bg_color(info_btn, lv_color_hex(0xFF9800), 0);
    
    lv_obj_t* info_label = lv_label_create(info_btn);
    lv_label_set_text(info_label, "System Info");
    lv_obj_set_style_text_color(info_label, lv_color_white(), 0);
    lv_obj_center(info_label);
    
    // 添加点击事件，切换到系统信息页面
    lv_obj_add_event_cb(info_btn, buttonEventCallback, LV_EVENT_CLICKED, (void*)(intptr_t)PAGE_SYSTEM_INFO);
    
    // 版本信息
    lv_obj_t* version_label = lv_label_create(m_pages[PAGE_HOME]);
    lv_label_set_text(version_label, "Version: v6.1.0");
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
    
    // 获取DisplayManager实例
    DisplayManager* manager = nullptr;
    
    // 通过遍历所有DisplayManager实例找到正确的实例
    // 这里使用一个简单的方法：通过用户数据查找
    lv_obj_t* parent = lv_obj_get_parent(btn);
    while (parent != nullptr) {
        void* user_data = lv_obj_get_user_data(parent);
        if (user_data != nullptr) {
            manager = static_cast<DisplayManager*>(user_data);
            break;
        }
        parent = lv_obj_get_parent(parent);
    }
    
    // 如果找不到，使用全局变量作为后备
    if (manager == nullptr) {
        extern DisplayManager displayManager;
        manager = &displayManager;
    }
    
    if (manager && page_index >= 0 && page_index < PAGE_COUNT) {
        manager->switchPage((DisplayPage)page_index);
    }
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
    // 创建系统信息页面
    m_pages[PAGE_SYSTEM_INFO] = lv_obj_create(m_pageContainer);
    lv_obj_set_size(m_pages[PAGE_SYSTEM_INFO], LV_HOR_RES, LV_VER_RES - 60);
    lv_obj_set_style_bg_color(m_pages[PAGE_SYSTEM_INFO], lv_color_white(), 0);
    lv_obj_set_style_border_width(m_pages[PAGE_SYSTEM_INFO], 0, 0);
    lv_obj_set_style_pad_all(m_pages[PAGE_SYSTEM_INFO], 10, 0);
    lv_obj_add_flag(m_pages[PAGE_SYSTEM_INFO], LV_OBJ_FLAG_HIDDEN);
    
    // 页面标题
    lv_obj_t* title = lv_label_create(m_pages[PAGE_SYSTEM_INFO]);
    lv_label_set_text(title, "System Information");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(title, lv_color_hex(0x2196F3), 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 10);
    
    // 系统信息标签
    lv_obj_t* info_label = lv_label_create(m_pages[PAGE_SYSTEM_INFO]);
    lv_label_set_text(info_label, 
        "Chip: ESP32S3\n"
        "CPU: 240MHz\n"
        "Flash: 16MB\n"
        "PSRAM: 8MB\n"
        "Version: v6.1.0");
    lv_obj_set_style_text_color(info_label, lv_color_hex(0x333333), 0);
    lv_obj_align(info_label, LV_ALIGN_CENTER, 0, -20);
    
    // 返回按钮
    lv_obj_t* back_btn = lv_btn_create(m_pages[PAGE_SYSTEM_INFO]);
    lv_obj_set_size(back_btn, 100, 35);
    lv_obj_align(back_btn, LV_ALIGN_BOTTOM_MID, 0, -20);
    lv_obj_set_style_bg_color(back_btn, lv_color_hex(0x2196F3), 0);
    
    lv_obj_t* back_label = lv_label_create(back_btn);
    lv_label_set_text(back_label, "Back");
    lv_obj_set_style_text_color(back_label, lv_color_white(), 0);
    lv_obj_center(back_label);
    
    // 添加返回按钮点击事件
    lv_obj_add_event_cb(back_btn, buttonEventCallback, LV_EVENT_CLICKED, (void*)(intptr_t)PAGE_HOME);
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
        BaseType_t result = xQueueSend(m_messageQueue, &msg, pdMS_TO_TICKS(100));
        if (result != pdTRUE) {
            printf("[DisplayManager] 亮度设置消息发送失败\n");
        }
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

void DisplayManager::updatePowerData(const PowerMonitorData& power_data) {
    // 直接更新内部数据
    m_powerData = power_data;
    
    // 立即更新新UI系统的显示
    updatePowerDataDisplay();
    
    // 更新所有端口的详细信息显示
    for (int i = 0; i < 4; i++) {
        updatePortDetailDisplay(i);
    }
    
    // 同时发送消息到队列进行后续处理
    DisplayMessage msg;
    msg.type = DisplayMessage::MSG_UPDATE_POWER_DATA;
    msg.data.power_monitor.power_data = power_data;
    
    if (m_messageQueue) {
        xQueueSend(m_messageQueue, &msg, pdMS_TO_TICKS(100));
    }
}

void DisplayManager::updateWeatherData(const char* temperature, const char* weather) {
    if (!temperature || !weather) {
        return;
    }
    
    // 立即更新UI显示
    updateWeatherDisplay();
    
    // 发送消息到队列进行后续处理
    DisplayMessage msg;
    msg.type = DisplayMessage::MSG_UPDATE_WEATHER_DATA;
    
    strncpy(msg.data.weather_data.temperature, temperature, sizeof(msg.data.weather_data.temperature) - 1);
    msg.data.weather_data.temperature[sizeof(msg.data.weather_data.temperature) - 1] = '\0';
    
    strncpy(msg.data.weather_data.weather, weather, sizeof(msg.data.weather_data.weather) - 1);
    msg.data.weather_data.weather[sizeof(msg.data.weather_data.weather) - 1] = '\0';
    
    msg.data.weather_data.valid = true;
    
    if (m_messageQueue) {
        xQueueSend(m_messageQueue, &msg, pdMS_TO_TICKS(100));
    }
}

void DisplayManager::switchToPowerPage(int port_index) {
    if (port_index >= 0 && port_index <= 4) {
        DisplayPage target_page = (DisplayPage)(PAGE_POWER_TOTAL + port_index);
        switchPage(target_page);
    }
}

const PowerMonitorData& DisplayManager::getCurrentPowerData() const {
    return m_powerData;
}

void DisplayManager::createTotalPowerPage() {
    // 创建总功率页面
    m_pages[PAGE_POWER_TOTAL] = lv_obj_create(m_pageContainer);
    lv_obj_set_size(m_pages[PAGE_POWER_TOTAL], LV_HOR_RES, LV_VER_RES - 60);
    lv_obj_set_style_bg_color(m_pages[PAGE_POWER_TOTAL], lv_color_hex(0xF0F4FF), 0);
    lv_obj_set_style_border_width(m_pages[PAGE_POWER_TOTAL], 0, 0);
    lv_obj_set_style_pad_all(m_pages[PAGE_POWER_TOTAL], 20, 0);
    lv_obj_add_flag(m_pages[PAGE_POWER_TOTAL], LV_OBJ_FLAG_HIDDEN);
    
    // 设置滑动事件
    setupPageSwipeEvents(m_pages[PAGE_POWER_TOTAL]);
    
    // 页面标题容器
    lv_obj_t* title_container = lv_obj_create(m_pages[PAGE_POWER_TOTAL]);
    lv_obj_set_size(title_container, LV_HOR_RES - 50, 60);
    lv_obj_set_pos(title_container, 25, 10);
    lv_obj_set_style_bg_color(title_container, lv_color_hex(0x6C5CE7), 0);
    lv_obj_set_style_border_width(title_container, 0, 0);
    lv_obj_set_style_radius(title_container, 20, 0);
    lv_obj_set_style_pad_all(title_container, 0, 0);
    
    // 页面标题
    lv_obj_t* title = lv_label_create(title_container);
    lv_label_set_text(title, "⚡ TOTAL POWER ⚡");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(title, lv_color_white(), 0);
    lv_obj_center(title);
    
    // 主功率显示卡片
    lv_obj_t* power_card = lv_obj_create(m_pages[PAGE_POWER_TOTAL]);
    lv_obj_set_size(power_card, LV_HOR_RES - 50, 120);
    lv_obj_set_pos(power_card, 25, 85);
    lv_obj_set_style_bg_color(power_card, lv_color_hex(0x00B894), 0);
    lv_obj_set_style_border_width(power_card, 0, 0);
    lv_obj_set_style_radius(power_card, 25, 0);
    lv_obj_set_style_pad_all(power_card, 20, 0);
    
    // 功率图标
    lv_obj_t* power_icon = lv_label_create(power_card);
    lv_label_set_text(power_icon, LV_SYMBOL_CHARGE);
    lv_obj_set_style_text_font(power_icon, &lv_font_montserrat_28, 0);
    lv_obj_set_style_text_color(power_icon, lv_color_white(), 0);
    lv_obj_align(power_icon, LV_ALIGN_TOP_LEFT, 0, 0);
    
    // 功率单位标签
    lv_obj_t* power_unit = lv_label_create(power_card);
    lv_label_set_text(power_unit, "WATTS");
    lv_obj_set_style_text_font(power_unit, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(power_unit, lv_color_white(), 0);
    lv_obj_align(power_unit, LV_ALIGN_TOP_RIGHT, 0, 5);
    
    // 总功率数值显示
    m_powerLabels[0][0] = lv_label_create(power_card);
    lv_label_set_text(m_powerLabels[0][0], "0.00");
    lv_obj_set_style_text_font(m_powerLabels[0][0], &lv_font_montserrat_48, 0);
    lv_obj_set_style_text_color(m_powerLabels[0][0], lv_color_white(), 0);
    lv_obj_align(m_powerLabels[0][0], LV_ALIGN_CENTER, 0, 0);
    
    // 端口网格布局
    int port_width = (LV_HOR_RES - 80) / 2;  // 减去边距和间距
    int port_height = 85;
    
    // 端口颜色主题
    uint32_t port_colors[] = {0xFF6B6B, 0x4ECDC4, 0x45B7D1, 0xFFA726};
    const char* port_icons[] = {"1️⃣", "2️⃣", "3️⃣", "4️⃣"};
    
    for (int i = 0; i < 4; i++) {
        int row = i / 2;
        int col = i % 2;
        
        lv_obj_t* port_card = lv_obj_create(m_pages[PAGE_POWER_TOTAL]);
        lv_obj_set_size(port_card, port_width, port_height);
        lv_obj_set_pos(port_card, 25 + col * (port_width + 20), 220 + row * (port_height + 15));
        lv_obj_set_style_bg_color(port_card, lv_color_hex(port_colors[i]), 0);
        lv_obj_set_style_border_width(port_card, 0, 0);
        lv_obj_set_style_radius(port_card, 18, 0);
        lv_obj_set_style_pad_all(port_card, 12, 0);
        
        // 端口图标
        lv_obj_t* port_icon = lv_label_create(port_card);
        lv_label_set_text(port_icon, port_icons[i]);
        lv_obj_set_style_text_font(port_icon, &lv_font_montserrat_20, 0);
        lv_obj_align(port_icon, LV_ALIGN_TOP_LEFT, 0, 0);
        
        // 端口状态指示器
        m_statusIndicators[i] = lv_obj_create(port_card);
        lv_obj_set_size(m_statusIndicators[i], 12, 12);
        lv_obj_set_pos(m_statusIndicators[i], port_width - 25, 5);
        lv_obj_set_style_bg_color(m_statusIndicators[i], lv_color_white(), 0);
        lv_obj_set_style_border_width(m_statusIndicators[i], 0, 0);
        lv_obj_set_style_radius(m_statusIndicators[i], 6, 0);
        lv_obj_set_style_opa(m_statusIndicators[i], LV_OPA_70, 0);
        
        // 端口标签
        lv_obj_t* port_label = lv_label_create(port_card);
        lv_label_set_text_fmt(port_label, "PORT %d", i + 1);
        lv_obj_set_style_text_font(port_label, &lv_font_montserrat_10, 0);
        lv_obj_set_style_text_color(port_label, lv_color_white(), 0);
        lv_obj_align(port_label, LV_ALIGN_TOP_LEFT, 0, 25);
        
        // 端口功率标签
        m_powerLabels[0][i + 1] = lv_label_create(port_card);
        lv_label_set_text(m_powerLabels[0][i + 1], "0.0W");
        lv_obj_set_style_text_font(m_powerLabels[0][i + 1], &lv_font_montserrat_20, 0);
        lv_obj_set_style_text_color(m_powerLabels[0][i + 1], lv_color_white(), 0);
        lv_obj_align(m_powerLabels[0][i + 1], LV_ALIGN_BOTTOM_MID, 0, 0);
        
        // 添加点击事件
        lv_obj_add_event_cb(port_card, buttonEventCallback, LV_EVENT_CLICKED, (void*)(intptr_t)(PAGE_POWER_PORT1 + i));
    }
    
    // 滑动提示
    lv_obj_t* swipe_hint = lv_label_create(m_pages[PAGE_POWER_TOTAL]);
    lv_label_set_text(swipe_hint, "👈 SWIPE FOR PORT DETAILS 👉");
    lv_obj_set_style_text_font(swipe_hint, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(swipe_hint, lv_color_hex(0x74B9FF), 0);
    lv_obj_align(swipe_hint, LV_ALIGN_BOTTOM_MID, 0, -15);
}

void DisplayManager::createPortPowerPage(int port_index) {
    if (port_index < 1 || port_index > 4) return;
    
    DisplayPage page_id = (DisplayPage)(PAGE_POWER_PORT1 + port_index - 1);
    
    // 创建端口页面
    m_pages[page_id] = lv_obj_create(m_pageContainer);
    lv_obj_set_size(m_pages[page_id], LV_HOR_RES, LV_VER_RES - 60);
    lv_obj_set_style_bg_color(m_pages[page_id], lv_color_hex(0xF8F9FA), 0);
    lv_obj_set_style_border_width(m_pages[page_id], 0, 0);
    lv_obj_set_style_pad_all(m_pages[page_id], 15, 0);
    lv_obj_add_flag(m_pages[page_id], LV_OBJ_FLAG_HIDDEN);
    
    // 设置滑动事件
    setupPageSwipeEvents(m_pages[page_id]);
    
    // 页面标题容器
    lv_obj_t* title_container = lv_obj_create(m_pages[page_id]);
    lv_obj_set_size(title_container, LV_HOR_RES - 40, 50);
    lv_obj_set_pos(title_container, 20, 5);
    lv_obj_set_style_bg_color(title_container, lv_color_hex(0x2196F3), 0);
    lv_obj_set_style_border_width(title_container, 0, 0);
    lv_obj_set_style_radius(title_container, 10, 0);
    lv_obj_set_style_pad_all(title_container, 0, 0);
    
    // 页面标题
    lv_obj_t* title = lv_label_create(title_container);
    lv_label_set_text_fmt(title, "PORT %d", port_index);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(title, lv_color_white(), 0);
    lv_obj_center(title);
    
    // 状态指示器（圆点）
    m_portStatusDots[port_index - 1] = lv_obj_create(title_container);
    lv_obj_set_size(m_portStatusDots[port_index - 1], 12, 12);
    lv_obj_set_pos(m_portStatusDots[port_index - 1], LV_HOR_RES - 65, 19);
    lv_obj_set_style_bg_color(m_portStatusDots[port_index - 1], lv_color_hex(0xCCCCCC), 0);
    lv_obj_set_style_border_width(m_portStatusDots[port_index - 1], 0, 0);
    lv_obj_set_style_radius(m_portStatusDots[port_index - 1], 6, 0);
    
    // 主功率显示卡片
    lv_obj_t* power_card = lv_obj_create(m_pages[page_id]);
    lv_obj_set_size(power_card, LV_HOR_RES - 40, 100);
    lv_obj_set_pos(power_card, 20, 70);
    lv_obj_set_style_bg_color(power_card, lv_color_white(), 0);
    lv_obj_set_style_border_width(power_card, 2, 0);
    lv_obj_set_style_border_color(power_card, lv_color_hex(0x4CAF50), 0);
    lv_obj_set_style_radius(power_card, 15, 0);
    lv_obj_set_style_pad_all(power_card, 15, 0);
    lv_obj_set_style_shadow_width(power_card, 10, 0);
    lv_obj_set_style_shadow_color(power_card, lv_color_hex(0x000000), 0);
    lv_obj_set_style_shadow_opa(power_card, LV_OPA_20, 0);
    
    // 功率标题
    lv_obj_t* power_title = lv_label_create(power_card);
    lv_label_set_text(power_title, "POWER");
    lv_obj_set_style_text_font(power_title, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(power_title, lv_color_hex(0x666666), 0);
    lv_obj_align(power_title, LV_ALIGN_TOP_MID, 0, 5);
    
    // 功率数值显示
    m_powerLabels[port_index][0] = lv_label_create(power_card);
    lv_label_set_text(m_powerLabels[port_index][0], "0.00W");
    lv_obj_set_style_text_font(m_powerLabels[port_index][0], &lv_font_montserrat_48, 0);
    lv_obj_set_style_text_color(m_powerLabels[port_index][0], lv_color_hex(0x4CAF50), 0);
    lv_obj_align(m_powerLabels[port_index][0], LV_ALIGN_CENTER, 0, 5);
    
    // 电压卡片
    lv_obj_t* voltage_card = lv_obj_create(m_pages[page_id]);
    lv_obj_set_size(voltage_card, (LV_HOR_RES - 70) / 2, 80);
    lv_obj_set_pos(voltage_card, 20, 185);
    lv_obj_set_style_bg_color(voltage_card, lv_color_white(), 0);
    lv_obj_set_style_border_width(voltage_card, 1, 0);
    lv_obj_set_style_border_color(voltage_card, lv_color_hex(0xFF9800), 0);
    lv_obj_set_style_radius(voltage_card, 12, 0);
    lv_obj_set_style_pad_all(voltage_card, 10, 0);
    lv_obj_set_style_shadow_width(voltage_card, 8, 0);
    lv_obj_set_style_shadow_color(voltage_card, lv_color_hex(0x000000), 0);
    lv_obj_set_style_shadow_opa(voltage_card, LV_OPA_10, 0);
    
    // 电压图标和标题
    lv_obj_t* voltage_icon = lv_label_create(voltage_card);
    lv_label_set_text(voltage_icon, LV_SYMBOL_CHARGE);
    lv_obj_set_style_text_font(voltage_icon, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(voltage_icon, lv_color_hex(0xFF9800), 0);
    lv_obj_align(voltage_icon, LV_ALIGN_TOP_LEFT, 5, 5);
    
    lv_obj_t* voltage_title = lv_label_create(voltage_card);
    lv_label_set_text(voltage_title, "VOLTAGE");
    lv_obj_set_style_text_font(voltage_title, &lv_font_montserrat_10, 0);
    lv_obj_set_style_text_color(voltage_title, lv_color_hex(0x666666), 0);
    lv_obj_align(voltage_title, LV_ALIGN_TOP_LEFT, 25, 8);
    
    m_powerLabels[port_index][1] = lv_label_create(voltage_card);
    lv_label_set_text(m_powerLabels[port_index][1], "0.00V");
    lv_obj_set_style_text_font(m_powerLabels[port_index][1], &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(m_powerLabels[port_index][1], lv_color_hex(0xFF9800), 0);
    lv_obj_align(m_powerLabels[port_index][1], LV_ALIGN_BOTTOM_MID, 0, -5);
    
    // 电流卡片
    lv_obj_t* current_card = lv_obj_create(m_pages[page_id]);
    lv_obj_set_size(current_card, (LV_HOR_RES - 70) / 2, 80);
    lv_obj_set_pos(current_card, 35 + (LV_HOR_RES - 70) / 2, 185);
    lv_obj_set_style_bg_color(current_card, lv_color_white(), 0);
    lv_obj_set_style_border_width(current_card, 1, 0);
    lv_obj_set_style_border_color(current_card, lv_color_hex(0x2196F3), 0);
    lv_obj_set_style_radius(current_card, 12, 0);
    lv_obj_set_style_pad_all(current_card, 10, 0);
    lv_obj_set_style_shadow_width(current_card, 8, 0);
    lv_obj_set_style_shadow_color(current_card, lv_color_hex(0x000000), 0);
    lv_obj_set_style_shadow_opa(current_card, LV_OPA_10, 0);
    
    // 电流图标和标题
    lv_obj_t* current_icon = lv_label_create(current_card);
    lv_label_set_text(current_icon, LV_SYMBOL_LOOP);
    lv_obj_set_style_text_font(current_icon, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(current_icon, lv_color_hex(0x2196F3), 0);
    lv_obj_align(current_icon, LV_ALIGN_TOP_LEFT, 5, 5);
    
    lv_obj_t* current_title = lv_label_create(current_card);
    lv_label_set_text(current_title, "CURRENT");
    lv_obj_set_style_text_font(current_title, &lv_font_montserrat_10, 0);
    lv_obj_set_style_text_color(current_title, lv_color_hex(0x666666), 0);
    lv_obj_align(current_title, LV_ALIGN_TOP_LEFT, 25, 8);
    
    m_powerLabels[port_index][2] = lv_label_create(current_card);
    lv_label_set_text(m_powerLabels[port_index][2], "0.00A");
    lv_obj_set_style_text_font(m_powerLabels[port_index][2], &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(m_powerLabels[port_index][2], lv_color_hex(0x2196F3), 0);
    lv_obj_align(m_powerLabels[port_index][2], LV_ALIGN_BOTTOM_MID, 0, -5);
    
    // 协议信息卡片
    lv_obj_t* protocol_card = lv_obj_create(m_pages[page_id]);
    lv_obj_set_size(protocol_card, LV_HOR_RES - 40, 70);
    lv_obj_set_pos(protocol_card, 20, 280);
    lv_obj_set_style_bg_color(protocol_card, lv_color_white(), 0);
    lv_obj_set_style_border_width(protocol_card, 1, 0);
    lv_obj_set_style_border_color(protocol_card, lv_color_hex(0x9C27B0), 0);
    lv_obj_set_style_radius(protocol_card, 12, 0);
    lv_obj_set_style_pad_all(protocol_card, 15, 0);
    lv_obj_set_style_shadow_width(protocol_card, 8, 0);
    lv_obj_set_style_shadow_color(protocol_card, lv_color_hex(0x000000), 0);
    lv_obj_set_style_shadow_opa(protocol_card, LV_OPA_10, 0);
    
    // 协议图标和标题
    lv_obj_t* protocol_icon = lv_label_create(protocol_card);
    lv_label_set_text(protocol_icon, LV_SYMBOL_SETTINGS);
    lv_obj_set_style_text_font(protocol_icon, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(protocol_icon, lv_color_hex(0x9C27B0), 0);
    lv_obj_align(protocol_icon, LV_ALIGN_LEFT_MID, 10, 0);
    
    lv_obj_t* protocol_title_label = lv_label_create(protocol_card);
    lv_label_set_text(protocol_title_label, "CHARGING PROTOCOL");
    lv_obj_set_style_text_font(protocol_title_label, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(protocol_title_label, lv_color_hex(0x666666), 0);
    lv_obj_align(protocol_title_label, LV_ALIGN_TOP_LEFT, 40, 8);
    
    m_powerLabels[port_index][3] = lv_label_create(protocol_card);
    lv_label_set_text(m_powerLabels[port_index][3], "None");
    lv_obj_set_style_text_font(m_powerLabels[port_index][3], &lv_font_montserrat_18, 0);
    lv_obj_set_style_text_color(m_powerLabels[port_index][3], lv_color_hex(0x9C27B0), 0);
    lv_obj_align(m_powerLabels[port_index][3], LV_ALIGN_BOTTOM_LEFT, 40, -8);
    
    // 状态信息
    m_powerLabels[port_index][4] = lv_label_create(protocol_card);
    lv_label_set_text(m_powerLabels[port_index][4], "Inactive");
    lv_obj_set_style_text_font(m_powerLabels[port_index][4], &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(m_powerLabels[port_index][4], lv_color_hex(0x757575), 0);
    lv_obj_align(m_powerLabels[port_index][4], LV_ALIGN_BOTTOM_RIGHT, -10, -8);
    
    // 滑动提示
    lv_obj_t* swipe_hint = lv_label_create(m_pages[page_id]);
    lv_label_set_text(swipe_hint, "← Swipe between ports →");
    lv_obj_set_style_text_font(swipe_hint, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(swipe_hint, lv_color_hex(0x999999), 0);
    lv_obj_align(swipe_hint, LV_ALIGN_BOTTOM_MID, 0, -10);
}

void DisplayManager::updatePowerDisplay() {
    if (!m_powerData.valid) return;
    
    // 更新总功率页面
    if (m_powerLabels[0][0]) {
        lv_label_set_text_fmt(m_powerLabels[0][0], "%.1f", m_powerData.total_power / 1000.0f);
    }
    
    // 更新端口概览
    for (int i = 0; i < 4; i++) {
        if (m_powerLabels[0][i + 1]) {
            if (m_powerData.ports[i].valid && m_powerData.ports[i].state) {
                lv_label_set_text_fmt(m_powerLabels[0][i + 1], "%.1fW", 
                                     m_powerData.ports[i].power / 1000.0f);
                
                // 更新状态指示器为亮绿色（活跃）
                if (m_statusIndicators[i]) {
                    lv_obj_set_style_bg_color(m_statusIndicators[i], lv_color_hex(0x00E676), 0);
                    lv_obj_set_style_opa(m_statusIndicators[i], LV_OPA_100, 0);
                }
            } else {
                lv_label_set_text(m_powerLabels[0][i + 1], "0.0W");
                
                // 更新状态指示器为半透明白色（不活跃）
                if (m_statusIndicators[i]) {
                    lv_obj_set_style_bg_color(m_statusIndicators[i], lv_color_white(), 0);
                    lv_obj_set_style_opa(m_statusIndicators[i], LV_OPA_50, 0);
                }
            }
        }
    }
    
    // 更新各端口详细页面
    for (int i = 0; i < 4; i++) {
        if (m_powerData.ports[i].valid) {
            // 功率
            if (m_powerLabels[i + 1][0]) {
                lv_label_set_text_fmt(m_powerLabels[i + 1][0], "%.2fW", 
                                     m_powerData.ports[i].power / 1000.0f);
            }
            
            // 电压
            if (m_powerLabels[i + 1][1]) {
                lv_label_set_text_fmt(m_powerLabels[i + 1][1], "%.2fV", 
                                     m_powerData.ports[i].voltage / 1000.0f);
            }
            
            // 电流
            if (m_powerLabels[i + 1][2]) {
                lv_label_set_text_fmt(m_powerLabels[i + 1][2], "%.2fA", 
                                     m_powerData.ports[i].current / 1000.0f);
            }
            
            // 协议
            if (m_powerLabels[i + 1][3]) {
                lv_label_set_text(m_powerLabels[i + 1][3], m_powerData.ports[i].protocol_name);
            }
            
            // 状态
            if (m_powerLabels[i + 1][4]) {
                lv_label_set_text(m_powerLabels[i + 1][4], 
                                 m_powerData.ports[i].state ? "Active" : "Inactive");
                lv_obj_set_style_text_color(m_powerLabels[i + 1][4], 
                                           m_powerData.ports[i].state ? 
                                           lv_color_hex(0x4CAF50) : lv_color_hex(0x757575), 0);
            }
            
            // 更新端口页面的状态指示器
            if (m_portStatusDots[i]) {
                if (m_powerData.ports[i].state) {
                    lv_obj_set_style_bg_color(m_portStatusDots[i], lv_color_hex(0x4CAF50), 0);
                } else {
                    lv_obj_set_style_bg_color(m_portStatusDots[i], lv_color_hex(0xCCCCCC), 0);
                }
            }
        }
    }
}

void DisplayManager::setupPageSwipeEvents(lv_obj_t* page) {
    // 启用手势识别
    lv_obj_add_flag(page, LV_OBJ_FLAG_GESTURE_BUBBLE);
    
    // 添加手势事件回调
    lv_obj_add_event_cb(page, swipeEventCallback, LV_EVENT_GESTURE, this);
}

void DisplayManager::swipeEventCallback(lv_event_t* event) {
    DisplayManager* manager = (DisplayManager*)lv_event_get_user_data(event);
    lv_dir_t dir = lv_indev_get_gesture_dir(lv_indev_get_act());
    
    if (manager->m_currentPage >= PAGE_POWER_TOTAL && manager->m_currentPage <= PAGE_POWER_PORT4) {
        if (dir == LV_DIR_LEFT) {
            // 向左滑动，切换到下一个页面
            if (manager->m_currentPage < PAGE_POWER_PORT4) {
                manager->switchPage((DisplayPage)(manager->m_currentPage + 1));
            }
        } else if (dir == LV_DIR_RIGHT) {
            // 向右滑动，切换到上一个页面
            if (manager->m_currentPage > PAGE_POWER_TOTAL) {
                manager->switchPage((DisplayPage)(manager->m_currentPage - 1));
            }
        }
    }
}

/**
 * @brief 更新时间显示
 */
void DisplayManager::updateTimeDisplay() {
    if (!m_lvglDriver || !m_lvglDriver->lock(100)) {
        return;
    }
    
    // 获取当前时间
    time_t now = time(nullptr);
    struct tm* timeinfo = localtime(&now);
    
    // 更新时间标签
    if (ui_timeLabel) {
        char time_str[16];
        strftime(time_str, sizeof(time_str), "%H:%M:%S", timeinfo);
        lv_label_set_text(ui_timeLabel, time_str);
    }
    
    // 更新日期标签
    if (ui_dataLabel) {
        char date_str[16];
        strftime(date_str, sizeof(date_str), "%m-%d", timeinfo);
        lv_label_set_text(ui_dataLabel, date_str);
    }
    
    // 更新星期标签
    if (ui_weekLabel) {
        const char* weekdays[] = {"星期日", "星期一", "星期二", "星期三", "星期四", "星期五", "星期六"};
        lv_label_set_text(ui_weekLabel, weekdays[timeinfo->tm_wday]);
    }
    
    m_lvglDriver->unlock();
}

/**
 * @brief 更新功率数据显示
 */
void DisplayManager::updatePowerDataDisplay() {
    if (!m_lvglDriver || !m_lvglDriver->lock(100)) {
        return;
    }
    
    // 更新总功率显示
    if (ui_totalpowerlabel && m_powerData.valid) {
        char total_power_str[16];
        snprintf(total_power_str, sizeof(total_power_str), "%.1fW", m_powerData.total_power / 1000.0f);
        lv_label_set_text(ui_totalpowerlabel, total_power_str);
    }
    
    // 更新各端口功率显示
    if (m_powerData.valid) {
        // 端口1
        if (ui_port1power && m_powerData.ports[0].valid) {
            char port1_str[16];
            snprintf(port1_str, sizeof(port1_str), "%.2fW", m_powerData.ports[0].power / 1000.0f);
            lv_label_set_text(ui_port1power, port1_str);
        }
        
        // 端口2
        if (ui_port2power && m_powerData.ports[1].valid) {
            char port2_str[16];
            snprintf(port2_str, sizeof(port2_str), "%.2fW", m_powerData.ports[1].power / 1000.0f);
            lv_label_set_text(ui_port2power, port2_str);
        }
        
        // 端口3
        if (ui_port3power && m_powerData.ports[2].valid) {
            char port3_str[16];
            snprintf(port3_str, sizeof(port3_str), "%.2fW", m_powerData.ports[2].power / 1000.0f);
            lv_label_set_text(ui_port3power, port3_str);
        }
        
        // 端口4
        if (ui_port4power && m_powerData.ports[3].valid) {
            char port4_str[16];
            snprintf(port4_str, sizeof(port4_str), "%.2fW", m_powerData.ports[3].power / 1000.0f);
            lv_label_set_text(ui_port4power, port4_str);
        }
        
        // 更新功率条显示
        updatePowerBars();
    }
    
    // 更新端口功率页面的详细信息
    if (m_powerData.valid) {
        // 端口1功率页面
        if (ui_port1powerlabel && m_powerData.ports[0].valid) {
            char port1_power_str[16];
            snprintf(port1_power_str, sizeof(port1_power_str), "%.3fW", m_powerData.ports[0].power / 1000.0f);
            lv_label_set_text(ui_port1powerlabel, port1_power_str);
        }
        if (ui_port1voltage && m_powerData.ports[0].valid) {
            char port1_voltage_str[16];
            snprintf(port1_voltage_str, sizeof(port1_voltage_str), "%.3fV", m_powerData.ports[0].voltage / 1000.0f);
            lv_label_set_text(ui_port1voltage, port1_voltage_str);
        }
        if (ui_port1current && m_powerData.ports[0].valid) {
            char port1_current_str[16];
            snprintf(port1_current_str, sizeof(port1_current_str), "%.3fA", m_powerData.ports[0].current / 1000.0f);
            lv_label_set_text(ui_port1current, port1_current_str);
        }
        
        // 端口2功率页面
        if (ui_port2powerlabel && m_powerData.ports[1].valid) {
            char port2_power_str[16];
            snprintf(port2_power_str, sizeof(port2_power_str), "%.3fW", m_powerData.ports[1].power / 1000.0f);
            lv_label_set_text(ui_port2powerlabel, port2_power_str);
        }
        if (ui_port2voltage && m_powerData.ports[1].valid) {
            char port2_voltage_str[16];
            snprintf(port2_voltage_str, sizeof(port2_voltage_str), "%.3fV", m_powerData.ports[1].voltage / 1000.0f);
            lv_label_set_text(ui_port2voltage, port2_voltage_str);
        }
        if (ui_port2current && m_powerData.ports[1].valid) {
            char port2_current_str[16];
            snprintf(port2_current_str, sizeof(port2_current_str), "%.3fA", m_powerData.ports[1].current / 1000.0f);
            lv_label_set_text(ui_port2current, port2_current_str);
        }
        
        // 端口3功率页面
        if (ui_port3powerlabel && m_powerData.ports[2].valid) {
            char port3_power_str[16];
            snprintf(port3_power_str, sizeof(port3_power_str), "%.3fW", m_powerData.ports[2].power / 1000.0f);
            lv_label_set_text(ui_port3powerlabel, port3_power_str);
        }
        if (ui_port3voltage && m_powerData.ports[2].valid) {
            char port3_voltage_str[16];
            snprintf(port3_voltage_str, sizeof(port3_voltage_str), "%.3fV", m_powerData.ports[2].voltage / 1000.0f);
            lv_label_set_text(ui_port3voltage, port3_voltage_str);
        }
        if (ui_port3current && m_powerData.ports[2].valid) {
            char port3_current_str[16];
            snprintf(port3_current_str, sizeof(port3_current_str), "%.3fA", m_powerData.ports[2].current / 1000.0f);
            lv_label_set_text(ui_port3current, port3_current_str);
        }
        
        // 端口4功率页面
        if (ui_port4powerlabel && m_powerData.ports[3].valid) {
            char port4_power_str[16];
            snprintf(port4_power_str, sizeof(port4_power_str), "%.3fW", m_powerData.ports[3].power / 1000.0f);
            lv_label_set_text(ui_port4powerlabel, port4_power_str);
        }
        if (ui_port4voltage && m_powerData.ports[3].valid) {
            char port4_voltage_str[16];
            snprintf(port4_voltage_str, sizeof(port4_voltage_str), "%.3fV", m_powerData.ports[3].voltage / 1000.0f);
            lv_label_set_text(ui_port4voltage, port4_voltage_str);
        }
        if (ui_port4current && m_powerData.ports[3].valid) {
            char port4_current_str[16];
            snprintf(port4_current_str, sizeof(port4_current_str), "%.3fA", m_powerData.ports[3].current / 1000.0f);
            lv_label_set_text(ui_port4current, port4_current_str);
        }
    }
    
    m_lvglDriver->unlock();
}

/**
 * @brief 更新功率条显示
 */
void DisplayManager::updatePowerBars() {
    // 计算功率条的比例 (基于最大功率100W)
    const float MAX_POWER = 100000.0f; // 100W in mW
    
    if (ui_port1powerbar && m_powerData.ports[0].valid) {
        float ratio = m_powerData.ports[0].power / MAX_POWER;
        if (ratio > 1.0f) ratio = 1.0f;
        lv_bar_set_value(ui_port1powerbar, (int32_t)(ratio * 100), LV_ANIM_ON);
    }
    
    if (ui_port2powerbar && m_powerData.ports[1].valid) {
        float ratio = m_powerData.ports[1].power / MAX_POWER;
        if (ratio > 1.0f) ratio = 1.0f;
        lv_bar_set_value(ui_port2powerbar, (int32_t)(ratio * 100), LV_ANIM_ON);
    }
    
    if (ui_port3powerbar && m_powerData.ports[2].valid) {
        float ratio = m_powerData.ports[2].power / MAX_POWER;
        if (ratio > 1.0f) ratio = 1.0f;
        lv_bar_set_value(ui_port3powerbar, (int32_t)(ratio * 100), LV_ANIM_ON);
    }
    
    if (ui_port4powerbar && m_powerData.ports[3].valid) {
        float ratio = m_powerData.ports[3].power / MAX_POWER;
        if (ratio > 1.0f) ratio = 1.0f;
        lv_bar_set_value(ui_port4powerbar, (int32_t)(ratio * 100), LV_ANIM_ON);
    }
}

/**
 * @brief 更新端口详细信息显示
 */
void DisplayManager::updatePortDetailDisplay(int port_index) {
    if (!m_lvglDriver || !m_lvglDriver->lock(100)) {
        return;
    }
    
    if (port_index < 0 || port_index >= 4 || !m_powerData.ports[port_index].valid) {
        m_lvglDriver->unlock();
        return;
    }
    
    // 获取当前端口的数据
    const PortData& portData = m_powerData.ports[port_index];
    
    // 根据端口索引更新对应的详细屏幕标签
    lv_obj_t* state_label = nullptr;
    lv_obj_t* protocol_label = nullptr;
    lv_obj_t* manufacturer_label = nullptr;
    lv_obj_t* cable_label = nullptr;
    lv_obj_t* voltage_label = nullptr;
    lv_obj_t* current_label = nullptr;
    
    // 根据端口索引获取对应的UI组件
    switch (port_index) {
        case 0:
            state_label = ui_port1state;
            protocol_label = ui_port1protocol;
            manufacturer_label = ui_port1manufactuervid;
            cable_label = ui_port1cablevid;
            voltage_label = ui_port1maxvbusvoltage;
            current_label = ui_port1maxvbuscurrent;
            break;
        case 1:
            state_label = ui_port2state;
            protocol_label = ui_port2protocol;
            manufacturer_label = ui_port2manufactuervid;
            cable_label = ui_port2cablevid;
            voltage_label = ui_port2maxvbusvoltage;
            current_label = ui_port2maxvbuscurrent;
            break;
        case 2:
            state_label = ui_port3state;
            protocol_label = ui_port3protocol;
            manufacturer_label = ui_port3manufactuervid;
            cable_label = ui_port3cablevid;
            voltage_label = ui_port3maxvbusvoltage;
            current_label = ui_port3maxvbuscurrent;
            break;
        case 3:
            state_label = ui_port4state;
            protocol_label = ui_port4protocol;
            manufacturer_label = ui_port4manufactuervid;
            cable_label = ui_port4cablevid;
            voltage_label = ui_port4maxvbusvoltage;
            current_label = ui_port4maxvbuscurrent;
            break;
    }
    
    // 更新状态显示
    if (state_label) {
        const char* state_text = portData.state ? "已连接" : "未连接";
        lv_label_set_text(state_label, state_text);
    }
    
    // 更新协议显示
    if (protocol_label) {
        if (strlen(portData.protocol_name) > 0) {
            lv_label_set_text(protocol_label, portData.protocol_name);
        } else {
            lv_label_set_text(protocol_label, "未知");
        }
    }
    
    // 更新制造商VID显示
    if (manufacturer_label) {
        if (portData.manufacturer_vid > 0) {
            char manufacturer_str[16];
            snprintf(manufacturer_str, sizeof(manufacturer_str), "0x%04X", portData.manufacturer_vid);
            lv_label_set_text(manufacturer_label, manufacturer_str);
        } else {
            lv_label_set_text(manufacturer_label, "未知");
        }
    }
    
    // 更新线缆VID显示
    if (cable_label) {
        if (portData.cable_vid > 0) {
            char cable_str[16];
            snprintf(cable_str, sizeof(cable_str), "0x%04X", portData.cable_vid);
            lv_label_set_text(cable_label, cable_str);
        } else {
            lv_label_set_text(cable_label, "未知");
        }
    }
    
    // 更新最大电压显示
    if (voltage_label) {
        if (portData.cable_max_vbus_voltage > 0) {
            char voltage_str[16];
            snprintf(voltage_str, sizeof(voltage_str), "%.1fV", portData.cable_max_vbus_voltage / 1000.0f);
            lv_label_set_text(voltage_label, voltage_str);
        } else {
            lv_label_set_text(voltage_label, "未知");
        }
    }
    
    // 更新最大电流显示
    if (current_label) {
        if (portData.cable_max_vbus_current > 0) {
            char current_str[16];
            snprintf(current_str, sizeof(current_str), "%.2fA", portData.cable_max_vbus_current / 1000.0f);
            lv_label_set_text(current_label, current_str);
        } else {
            lv_label_set_text(current_label, "未知");
        }
    }
    
    m_lvglDriver->unlock();
}

/**
 * @brief 更新天气显示
 */
void DisplayManager::updateWeatherDisplay() {
    if (!m_lvglDriver || !m_lvglDriver->lock(100)) {
        return;
    }
    
    // 如果没有天气管理器，跳过更新
    if (!m_weatherManager) {
        m_lvglDriver->unlock();
        return;
    }
    
    // 获取当前天气数据
    auto currentWeather = m_weatherManager->getCurrentWeather();
    
    if (currentWeather.isValid) {
        // 更新温度显示
        if (ui_temperatureLabel) {
            char temp_str[16];
            snprintf(temp_str, sizeof(temp_str), "%s度", currentWeather.temperature.c_str());
            lv_label_set_text(ui_temperatureLabel, temp_str);
        }
        
        // 更新天气状况显示
        if (ui_weatherLabel) {
            lv_label_set_text(ui_weatherLabel, currentWeather.weather.c_str());
        }
        
        printf("[DisplayManager] 天气显示已更新：%s度 %s\n", 
               currentWeather.temperature.c_str(), 
               currentWeather.weather.c_str());
    } else {
        // 天气数据无效时显示默认值
        if (ui_temperatureLabel) {
            lv_label_set_text(ui_temperatureLabel, "--度");
        }
        
        if (ui_weatherLabel) {
            lv_label_set_text(ui_weatherLabel, "--");
        }
    }
    
    m_lvglDriver->unlock();
} 