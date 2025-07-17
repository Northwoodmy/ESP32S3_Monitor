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

// 静态实例指针定义
DisplayManager* DisplayManager::s_instance = nullptr;

// UI系统助手头文件
#include "ui_helpers.h"
#include "ui2_helpers.h"

// 外部声明UI1系统的屏幕对象
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

// 外部声明UI2系统的屏幕对象
extern lv_obj_t * ui2_standbySCREEN;
extern lv_obj_t * ui2_totalpowerSCREEN;
extern lv_obj_t * ui2_port1SCREEN;
extern lv_obj_t * ui2_port2SCREEN;
extern lv_obj_t * ui2_port3SCREEN;
extern lv_obj_t * ui2_port4SCREEN;

// 外部声明UI1系统的标签对象
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

// 外部声明UI2系统的标签对象
extern lv_obj_t * ui2_timeLabel;
extern lv_obj_t * ui2_dataLabel;
extern lv_obj_t * ui2_weekLabel;
extern lv_obj_t * ui2_totalpowerlabel;
extern lv_obj_t * ui2_port1power;
extern lv_obj_t * ui2_port2power;
extern lv_obj_t * ui2_port3power;
extern lv_obj_t * ui2_port4power;
extern lv_obj_t * ui2_port1voltage;
extern lv_obj_t * ui2_port2voltage;
extern lv_obj_t * ui2_port3voltage;
extern lv_obj_t * ui2_port4voltage;
extern lv_obj_t * ui2_port1current;
extern lv_obj_t * ui2_port2current;
extern lv_obj_t * ui2_port3current;
extern lv_obj_t * ui2_port4current;

// 外部声明UI1系统天气相关组件
extern lv_obj_t * ui_temperatureLabel;
extern lv_obj_t * ui_weatherLabel;

// 外部声明UI2系统天气相关组件
extern lv_obj_t * ui2_temperatureLabel;
extern lv_obj_t * ui2_weatherLabel;

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

// 外部声明UI1系统端口详细信息组件
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

// 外部声明UI2系统端口详细信息组件
// 端口1详细信息
extern lv_obj_t * ui2_port1state;
extern lv_obj_t * ui2_port1protocol;
extern lv_obj_t * ui2_port1manufactuervid;
extern lv_obj_t * ui2_port1cablevid;
extern lv_obj_t * ui2_port1maxvbusvoltage;
extern lv_obj_t * ui2_port1maxvbuscurrent;
extern lv_obj_t * ui2_port1powerlabel;
extern lv_obj_t * ui2_port1active;
extern lv_obj_t * ui2_port1voltage1;
extern lv_obj_t * ui2_port1current1;

// 端口2详细信息
extern lv_obj_t * ui2_port2state;
extern lv_obj_t * ui2_port2protocol;
extern lv_obj_t * ui2_port2manufactuervid;
extern lv_obj_t * ui2_port2cablevid;
extern lv_obj_t * ui2_port2maxvbusvoltage;
extern lv_obj_t * ui2_port2maxvbuscurrent;
extern lv_obj_t * ui2_port2powerlabel;
extern lv_obj_t * ui2_port2powerlabel1;
extern lv_obj_t * ui2_port2voltage1;
extern lv_obj_t * ui2_port2current1;

// 端口3详细信息
extern lv_obj_t * ui2_port3state;
extern lv_obj_t * ui2_port3protocol;
extern lv_obj_t * ui2_port3manufactuervid;
extern lv_obj_t * ui2_port3cablevid;
extern lv_obj_t * ui2_port3maxvbusvoltage;
extern lv_obj_t * ui2_port3maxvbuscurrent;
extern lv_obj_t * ui2_port3powerlabel;
extern lv_obj_t * ui2_port3powerlabel1;
extern lv_obj_t * ui2_port3voltage1;
extern lv_obj_t * ui2_port3current1;

// 端口4详细信息
extern lv_obj_t * ui2_port4state;
extern lv_obj_t * ui2_port4protocol;
extern lv_obj_t * ui2_port4manufactuervid;
extern lv_obj_t * ui2_port4cablevid;
extern lv_obj_t * ui2_port4maxvbusvoltage;
extern lv_obj_t * ui2_port4maxvbuscurrent;
extern lv_obj_t * ui2_port4powerlabel;
extern lv_obj_t * ui2_port4powerlabel1;
extern lv_obj_t * ui2_port4voltage1;
extern lv_obj_t * ui2_port4current1;

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
    , m_currentTheme(THEME_UI1)
    , m_brightness(80)
    , m_uiSystemActive(false)
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
    , m_screenMode(SCREEN_MODE_ALWAYS_ON)
    , m_screenStartHour(8)
    , m_screenStartMinute(0)
    , m_screenEndHour(22)
    , m_screenEndMinute(0)
    , m_screenTimeoutMinutes(10)
    , m_screenOn(true)
    , m_lastTouchTime(0)
    , m_lastScreenModeCheck(0)
    , m_powerControlEnabled(false)
    , m_powerOffThreshold(1000)
    , m_powerOnThreshold(2000)
    , m_lowPowerStartTime(0)
    , m_isInLowPowerMode(false)
    , m_autoSwitchEnabled(true)
    , m_previousPage(PAGE_HOME)
    , m_autoSwitchStartTime(0)
    , m_autoSwitchDuration(10000)
    , m_isInAutoSwitchMode(false)
    , m_currentAutoSwitchPort(-1)
    , m_powerBasedAutoSwitchEnabled(true)
    , m_lowPowerThreshold(1000)
    , m_highPowerThreshold(2000)
    , m_isManualSwitch(false)
    , m_lastManualPage(PAGE_HOME)
    , m_lastPowerCheckTime(0)
    , m_powerCheckInterval(2000)
    , m_lastTotalPower(0)
{
    // 设置全局实例指针
    s_instance = this;
    
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
    
    // 初始化端口功率和状态历史
    for (int i = 0; i < 4; i++) {
        m_previousPortPower[i] = 0;
        m_previousPortState[i] = false;
        m_lastAutoSwitchTime[i] = 0;
    }
    
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
    
    // 首先检查要加载的主题
    DisplayTheme targetTheme = THEME_UI1; // 默认主题
    
    // 加载主题配置
    if (m_configStorage->hasThemeConfigAsync(3000)) {
        int savedTheme = m_configStorage->loadThemeConfigAsync(3000);
        if (savedTheme >= 0 && savedTheme <= 2) {
            targetTheme = (DisplayTheme)savedTheme;
            printf("[DisplayManager] 加载保存的主题: %d\n", targetTheme);
        } else {
            printf("[DisplayManager] 警告：保存的主题值无效(%d)，使用默认主题\n", savedTheme);
        }
    } else {
        printf("[DisplayManager] 使用默认主题: UI1\n");
    }
    
    // 获取LVGL锁并初始化对应的UI系统
    if (m_lvglDriver->lock(5000)) {
        // 根据目标主题初始化相应的UI系统
        if (targetTheme == THEME_UI1) {
            // 初始化UI1系统
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
                printf("[DisplayManager] 显示UI1默认页面：待机屏幕\n");
            }
            
            m_currentTheme = THEME_UI1;
            printf("[DisplayManager] UI1系统初始化完成\n");
            
        } else if (targetTheme == THEME_UI2) {
            // 直接初始化UI2系统
            ui2_init();
            
            // 获取主屏幕
            m_screen = lv_scr_act();
            if (!m_screen) {
                printf("[DisplayManager] 错误：获取主屏幕失败\n");
                m_lvglDriver->unlock();
                return false;
            }
            
            // 显示默认页面（待机屏幕）
            if (ui2_standbySCREEN) {
                lv_scr_load(ui2_standbySCREEN);
                printf("[DisplayManager] 显示UI2默认页面：待机屏幕\n");
            }
            
            m_currentTheme = THEME_UI2;
            printf("[DisplayManager] UI2系统初始化完成\n");
        }
        
        // 初始化时间和日期显示
        updateTimeDisplay();
        
        m_lvglDriver->unlock();
        
        printf("[DisplayManager] UI系统初始化完成\n");
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
    
    // 加载屏幕模式配置
    if (!loadScreenModeConfig()) {
        printf("[DisplayManager] 警告：加载屏幕模式配置失败，使用默认配置\n");
    }
    
    // 初始化触摸时间
    m_lastTouchTime = xTaskGetTickCount() * portTICK_PERIOD_MS;
    m_lastScreenModeCheck = m_lastTouchTime;
    
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
        
        // 定期更新时间显示和屏幕模式检查
        TickType_t currentTime = xTaskGetTickCount();
        if (currentTime - lastUpdateTime >= updateInterval) {
            // 更新时间显示
            updateTimeDisplay();
            
            // 更新天气显示
            updateWeatherDisplay();
            
            // 处理屏幕模式管理逻辑
            processScreenModeLogic();
            
            // 检查自动切换超时
            checkAutoSwitchTimeout();
            
            // 检查基于总功率的页面切换
            checkPowerBasedPageSwitch();
            
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
            //printf("[DisplayManager] 功率数据已更新：总功率=%d mW\n", m_powerData.total_power);
            break;
            
        case DisplayMessage::MSG_UPDATE_WEATHER_DATA:
            // 更新天气数据显示
            if (msg.data.weather_data.valid) {
                //printf("[DisplayManager] 天气数据已更新：温度=%s，天气=%s\n", 
                //       msg.data.weather_data.temperature, 
                //       msg.data.weather_data.weather);
            } else {
                printf("[DisplayManager] 天气数据无效\n");
            }
            break;
            
        case DisplayMessage::MSG_SWITCH_PAGE:
            // 切换页面
            if (msg.data.page_switch.page < PAGE_COUNT) {
                DisplayPage newPage = msg.data.page_switch.page;
                
                // 根据当前主题和页面类型执行实际的屏幕切换
                bool switchSuccess = false;
                
                if (m_currentTheme == THEME_UI1) {
                    // UI1系统的页面切换
                    switch (newPage) {
                        case PAGE_HOME:
                            if (ui_standbySCREEN) {
                                _ui_screen_change(&ui_standbySCREEN, LV_SCR_LOAD_ANIM_MOVE_RIGHT, 500, 0, &ui_standbySCREEN_screen_init);
                                m_currentPage = newPage;
                                switchSuccess = true;
                            }
                            break;
                        case PAGE_POWER_TOTAL:
                            if (ui_totalpowerSCREEN) {
                                _ui_screen_change(&ui_totalpowerSCREEN, LV_SCR_LOAD_ANIM_MOVE_RIGHT, 500, 0, &ui_totalpowerSCREEN_screen_init);
                                m_currentPage = newPage;
                                switchSuccess = true;
                            }
                            break;
                        case PAGE_POWER_PORT1:
                            if (ui_prot1SCREEN) {
                                _ui_screen_change(&ui_prot1SCREEN, LV_SCR_LOAD_ANIM_MOVE_RIGHT, 500, 0, &ui_prot1SCREEN_screen_init);
                                m_currentPage = newPage;
                                switchSuccess = true;
                            }
                            break;
                        case PAGE_POWER_PORT2:
                            if (ui_prot2SCREEN) {
                                _ui_screen_change(&ui_prot2SCREEN, LV_SCR_LOAD_ANIM_MOVE_RIGHT, 500, 0, &ui_prot2SCREEN_screen_init);
                                m_currentPage = newPage;
                                switchSuccess = true;
                            }
                            break;
                        case PAGE_POWER_PORT3:
                            if (ui_prot3SCREEN) {
                                _ui_screen_change(&ui_prot3SCREEN, LV_SCR_LOAD_ANIM_MOVE_RIGHT, 500, 0, &ui_prot3SCREEN_screen_init);
                                m_currentPage = newPage;
                                switchSuccess = true;
                            }
                            break;
                        case PAGE_POWER_PORT4:
                            if (ui_prot4SCREEN) {
                                _ui_screen_change(&ui_prot4SCREEN, LV_SCR_LOAD_ANIM_MOVE_RIGHT, 500, 0, &ui_prot4SCREEN_screen_init);
                                m_currentPage = newPage;
                                switchSuccess = true;
                            }
                            break;
                    }
                } else if (m_currentTheme == THEME_UI2) {
                    // UI2系统的页面切换
                    switch (newPage) {
                        case PAGE_HOME:
                            if (ui2_standbySCREEN) {
                                _ui2_screen_change(&ui2_standbySCREEN, LV_SCR_LOAD_ANIM_MOVE_RIGHT, 500, 0, &ui2_standbySCREEN_screen_init);
                                m_currentPage = newPage;
                                switchSuccess = true;
                            }
                            break;
                        case PAGE_POWER_TOTAL:
                            if (ui2_totalpowerSCREEN) {
                                _ui2_screen_change(&ui2_totalpowerSCREEN, LV_SCR_LOAD_ANIM_MOVE_RIGHT, 500, 0, &ui2_totalpowerSCREEN_screen_init);
                                m_currentPage = newPage;
                                switchSuccess = true;
                            }
                            break;
                        case PAGE_POWER_PORT1:
                            if (ui2_port1SCREEN) {
                                _ui2_screen_change(&ui2_port1SCREEN, LV_SCR_LOAD_ANIM_MOVE_RIGHT, 500, 0, &ui2_port1SCREEN_screen_init);
                                m_currentPage = newPage;
                                switchSuccess = true;
                            }
                            break;
                        case PAGE_POWER_PORT2:
                            if (ui2_port2SCREEN) {
                                _ui2_screen_change(&ui2_port2SCREEN, LV_SCR_LOAD_ANIM_MOVE_RIGHT, 500, 0, &ui2_port2SCREEN_screen_init);
                                m_currentPage = newPage;
                                switchSuccess = true;
                            }
                            break;
                        case PAGE_POWER_PORT3:
                            if (ui2_port3SCREEN) {
                                _ui2_screen_change(&ui2_port3SCREEN, LV_SCR_LOAD_ANIM_MOVE_RIGHT, 500, 0, &ui2_port3SCREEN_screen_init);
                                m_currentPage = newPage;
                                switchSuccess = true;
                            }
                            break;
                        case PAGE_POWER_PORT4:
                            if (ui2_port4SCREEN) {
                                _ui2_screen_change(&ui2_port4SCREEN, LV_SCR_LOAD_ANIM_MOVE_RIGHT, 500, 0, &ui2_port4SCREEN_screen_init);
                                m_currentPage = newPage;
                                switchSuccess = true;
                            }
                            break;
                    }
                }
                
                if (switchSuccess) {
                    // 如果是手动切换到端口页面，保持手动切换标志
                    if (m_isManualSwitch && isPortPage(newPage)) {
                        printf("[DisplayManager] 手动切换到端口页面：%d\n", m_currentPage);
                    } else if (m_isManualSwitch && !isPortPage(newPage)) {
                        // 手动切换到非端口页面，重置手动切换标志
                        m_isManualSwitch = false;
                        printf("[DisplayManager] 手动切换到非端口页面：%d，重置手动切换标志\n", m_currentPage);
                    } else {
                        printf("[DisplayManager] 切换到页面：%d\n", m_currentPage);
                    }
                } else {
                    printf("[DisplayManager] 警告：页面%d切换失败\n", newPage);
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
            // 主题切换改为重启系统的方式，此消息类型保留为兼容性
            printf("[DisplayManager] 主题切换消息（将通过系统重启应用）：%d\n", msg.data.theme.theme);
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
            
        case DisplayMessage::MSG_SCREEN_MODE_CHANGED:
            // 屏幕模式变更
            m_screenMode = msg.data.screen_mode.mode;
            m_screenStartHour = msg.data.screen_mode.startHour;
            m_screenStartMinute = msg.data.screen_mode.startMinute;
            m_screenEndHour = msg.data.screen_mode.endHour;
            m_screenEndMinute = msg.data.screen_mode.endMinute;
            m_screenTimeoutMinutes = msg.data.screen_mode.timeoutMinutes;
            
            printf("[DisplayManager] 屏幕模式已更改: 模式=%d, 时间=%02d:%02d-%02d:%02d, 延时=%d分钟\n",
                   m_screenMode, m_screenStartHour, m_screenStartMinute,
                   m_screenEndHour, m_screenEndMinute, m_screenTimeoutMinutes);
            
            // 重新评估屏幕状态
            processScreenModeLogic();
            break;
            
        case DisplayMessage::MSG_TOUCH_ACTIVITY:
            // 触摸活动
            m_lastTouchTime = xTaskGetTickCount() * portTICK_PERIOD_MS;
            printf("[DisplayManager] 触摸活动检测，延时计时器已重置\n");
            
            // 如果屏幕当前关闭，触摸活动应立即开启屏幕
            if (!m_screenOn) {
                performScreenOn();
            }
            break;
            
        case DisplayMessage::MSG_SCREEN_ON:
            // 强制开启屏幕
            performScreenOn();
            break;
            
        case DisplayMessage::MSG_SCREEN_OFF:
            // 强制关闭屏幕
            performScreenOff();
            break;
            
        case DisplayMessage::MSG_AUTO_SWITCH_PORT:
            // 自动切换到端口屏幕
            performAutoSwitchToPort(msg.data.auto_switch_port.port_index, 
                                  msg.data.auto_switch_port.duration_ms);
            break;
    }
    
    m_lvglDriver->unlock();
}

// 旧款手动UI创建函数已删除 - 现在使用SquareLine Studio生成的UI1和UI2系统

// 旧款手动UI创建函数已删除 - 现在使用SquareLine Studio生成的UI1和UI2系统

// 旧款手动UI创建和回调函数已删除 - 现在使用SquareLine Studio生成的UI1和UI2系统

// 公共接口实现
void DisplayManager::switchPage(DisplayPage page) {
    // 如果不是手动切换，则标记为自动切换
    if (!m_isManualSwitch) {
        printf("[DisplayManager] 自动切换到页面：%d\n", page);
    }
    
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
    // 主题切换改为重启系统的方式，此函数保留为兼容性接口
    // 实际的主题切换现在通过WebServerManager保存配置后重启系统完成
    printf("[DisplayManager] 主题切换请求：%d（将通过系统重启应用）\n", theme);
}

void DisplayManager::switchUISystem(DisplayTheme theme) {
    printf("[DisplayManager] 初始化UI系统到主题: %d\n", theme);
    
    // 获取LVGL锁
    if (!m_lvglDriver->lock(5000)) {
        printf("[DisplayManager] 错误：获取LVGL锁超时\n");
        return;
    }
    
    // 创建一个临时的空白屏幕作为活动屏幕，避免删除活动屏幕的错误
    lv_obj_t* temp_screen = lv_obj_create(NULL);
    if (temp_screen) {
        lv_scr_load(temp_screen);
        printf("[DisplayManager] 创建临时屏幕作为活动屏幕\n");
        
        // 短暂延迟，确保屏幕切换完成
        vTaskDelay(pdMS_TO_TICKS(100));
    }
    
    // 销毁当前UI系统
    if (m_currentTheme == THEME_UI1) {
        destroyUI1System();
    } else if (m_currentTheme == THEME_UI2) {
        destroyUI2System();
    }
    
    // 初始化新的UI系统
    if (theme == THEME_UI1) {
        initUI1System();
    } else if (theme == THEME_UI2) {
        initUI2System();
    }
    
    // 删除临时屏幕（如果成功创建的话）
    if (temp_screen) {
        lv_obj_del(temp_screen);
        printf("[DisplayManager] 删除临时屏幕\n");
    }
    
    // 更新当前主题
    m_currentTheme = theme;
    m_uiSystemActive = true;
    
    // 注意：不在此处保存配置，主题配置的保存由WebServerManager负责
    // 这个函数现在主要用于系统初始化时根据保存的配置加载对应的UI系统
    
    // 更新显示
    updateTimeDisplay();
    updatePowerDataDisplay();
    updateWeatherDisplay();
    
    m_lvglDriver->unlock();
    
    printf("[DisplayManager] UI系统初始化完成\n");
}

void DisplayManager::initUI1System() {
    printf("[DisplayManager] 初始化UI1系统\n");
    
    try {
        // 初始化UI1系统
        ui_init();
        
        // 短暂延迟，确保UI系统初始化完成
        vTaskDelay(pdMS_TO_TICKS(50));
        
        // 显示默认页面
        if (ui_standbySCREEN) {
            lv_scr_load(ui_standbySCREEN);
            printf("[DisplayManager] UI1待机屏幕已加载\n");
        } else {
            printf("[DisplayManager] 警告：UI1待机屏幕未创建\n");
        }
        
        printf("[DisplayManager] UI1系统初始化完成\n");
    } catch (...) {
        printf("[DisplayManager] 错误：UI1系统初始化失败\n");
    }
}

void DisplayManager::initUI2System() {
    printf("[DisplayManager] 初始化UI2系统\n");
    
    try {
        // 初始化UI2系统
        ui2_init();
        
        // 短暂延迟，确保UI系统初始化完成
        vTaskDelay(pdMS_TO_TICKS(50));
        
        // 显示默认页面
        if (ui2_standbySCREEN) {
            lv_scr_load(ui2_standbySCREEN);
            printf("[DisplayManager] UI2待机屏幕已加载\n");
        } else {
            printf("[DisplayManager] 警告：UI2待机屏幕未创建\n");
        }
        
        printf("[DisplayManager] UI2系统初始化完成\n");
    } catch (...) {
        printf("[DisplayManager] 错误：UI2系统初始化失败\n");
    }
}

void DisplayManager::destroyUI1System() {
    printf("[DisplayManager] 销毁UI1系统\n");
    
    // 检查UI1系统是否存在
    if (ui_standbySCREEN != NULL) {
        // 销毁UI1系统
        ui_destroy();
        printf("[DisplayManager] UI1系统销毁完成\n");
    } else {
        printf("[DisplayManager] UI1系统未初始化，跳过销毁\n");
    }
}

void DisplayManager::destroyUI2System() {
    printf("[DisplayManager] 销毁UI2系统\n");
    
    // 检查UI2系统是否存在
    if (ui2_standbySCREEN != NULL) {
        // 销毁UI2系统
        ui2_destroy();
        printf("[DisplayManager] UI2系统销毁完成\n");
    } else {
        printf("[DisplayManager] UI2系统未初始化，跳过销毁\n");
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
    
    // 检查端口功率变化并触发自动切换
    if (m_autoSwitchEnabled && m_powerData.valid) {
        checkPortPowerChange();
    }
    
    // 检查基于总功率的页面切换
    if (m_powerBasedAutoSwitchEnabled && m_powerData.valid) {
        // 更新总功率值
        m_lastTotalPower = m_powerData.total_power;
    }
    
    // 如果启用了功率控制且当前为延时模式，立即检查屏幕状态
    if (m_powerControlEnabled && m_powerData.valid && m_screenMode == SCREEN_MODE_TIMEOUT) {
        processPowerControlLogic();
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
        manualSwitchPage(target_page);
    }
}

const PowerMonitorData& DisplayManager::getCurrentPowerData() const {
    return m_powerData;
}

// 旧款手动UI创建函数已删除 - 现在使用SquareLine Studio生成的UI1和UI2系统

// 旧款手动UI创建函数已删除 - 现在使用SquareLine Studio生成的UI1和UI2系统

// 旧款手动UI更新和事件处理函数已删除 - 现在使用SquareLine Studio生成的UI1和UI2系统

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
    
    // 根据当前主题更新相应的时间标签
    if (m_currentTheme == THEME_UI1) {
        // UI1系统的时间标签
        if (ui_timeLabel) {
            char time_str[16];
            strftime(time_str, sizeof(time_str), "%H:%M:%S", timeinfo);
            lv_label_set_text(ui_timeLabel, time_str);
        }
        
        if (ui_dataLabel) {
            char date_str[16];
            strftime(date_str, sizeof(date_str), "%m-%d", timeinfo);
            lv_label_set_text(ui_dataLabel, date_str);
        }
        
        if (ui_weekLabel) {
            const char* weekdays[] = {"星期日", "星期一", "星期二", "星期三", "星期四", "星期五", "星期六"};
            lv_label_set_text(ui_weekLabel, weekdays[timeinfo->tm_wday]);
        }
    } else if (m_currentTheme == THEME_UI2) {
        // UI2系统的时间标签
        if (ui2_timeLabel) {
            char time_str[16];
            strftime(time_str, sizeof(time_str), "%H:%M:%S", timeinfo);
            lv_label_set_text(ui2_timeLabel, time_str);
        }
        
        if (ui2_dataLabel) {
            char date_str[16];
            strftime(date_str, sizeof(date_str), "%m-%d", timeinfo);
            lv_label_set_text(ui2_dataLabel, date_str);
        }
        
        if (ui2_weekLabel) {
            const char* weekdays[] = {"星期日", "星期一", "星期二", "星期三", "星期四", "星期五", "星期六"};
            lv_label_set_text(ui2_weekLabel, weekdays[timeinfo->tm_wday]);
        }
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
    
    // 根据当前主题更新相应的功率显示
    if (m_currentTheme == THEME_UI1) {
        // UI1系统的功率显示
        if (ui_totalpowerlabel && m_powerData.valid) {
            char total_power_str[16];
            snprintf(total_power_str, sizeof(total_power_str), "%.1fW", m_powerData.total_power / 1000.0f);
            lv_label_set_text(ui_totalpowerlabel, total_power_str);
        }
        
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
    } else if (m_currentTheme == THEME_UI2) {
        // UI2系统的功率显示
        if (ui2_totalpowerlabel && m_powerData.valid) {
            char total_power_str[16];
            snprintf(total_power_str, sizeof(total_power_str), "%.1fW", m_powerData.total_power / 1000.0f);
            lv_label_set_text(ui2_totalpowerlabel, total_power_str);
        }
        
        if (m_powerData.valid) {
            // UI2系统的端口功率显示
            if (ui2_port1power && m_powerData.ports[0].valid) {
                char port1_str[16];
                snprintf(port1_str, sizeof(port1_str), "%.2fW", m_powerData.ports[0].power / 1000.0f);
                lv_label_set_text(ui2_port1power, port1_str);
            }
            
            if (ui2_port2power && m_powerData.ports[1].valid) {
                char port2_str[16];
                snprintf(port2_str, sizeof(port2_str), "%.2fW", m_powerData.ports[1].power / 1000.0f);
                lv_label_set_text(ui2_port2power, port2_str);
            }
            
            if (ui2_port3power && m_powerData.ports[2].valid) {
                char port3_str[16];
                snprintf(port3_str, sizeof(port3_str), "%.2fW", m_powerData.ports[2].power / 1000.0f);
                lv_label_set_text(ui2_port3power, port3_str);
            }
            
            if (ui2_port4power && m_powerData.ports[3].valid) {
                char port4_str[16];
                snprintf(port4_str, sizeof(port4_str), "%.2fW", m_powerData.ports[3].power / 1000.0f);
                lv_label_set_text(ui2_port4power, port4_str);
            }
            
            // UI2系统的电压和电流显示
            if (ui2_port1voltage && m_powerData.ports[0].valid) {
                char voltage_str[16];
                snprintf(voltage_str, sizeof(voltage_str), "%.2fV", m_powerData.ports[0].voltage / 1000.0f);
                lv_label_set_text(ui2_port1voltage, voltage_str);
            }
            
            if (ui2_port1current && m_powerData.ports[0].valid) {
                char current_str[16];
                snprintf(current_str, sizeof(current_str), "%.2fA", m_powerData.ports[0].current / 1000.0f);
                lv_label_set_text(ui2_port1current, current_str);
            }
            
            if (ui2_port2voltage && m_powerData.ports[1].valid) {
                char voltage_str[16];
                snprintf(voltage_str, sizeof(voltage_str), "%.2fV", m_powerData.ports[1].voltage / 1000.0f);
                lv_label_set_text(ui2_port2voltage, voltage_str);
            }
            
            if (ui2_port2current && m_powerData.ports[1].valid) {
                char current_str[16];
                snprintf(current_str, sizeof(current_str), "%.2fA", m_powerData.ports[1].current / 1000.0f);
                lv_label_set_text(ui2_port2current, current_str);
            }
            
            if (ui2_port3voltage && m_powerData.ports[2].valid) {
                char voltage_str[16];
                snprintf(voltage_str, sizeof(voltage_str), "%.2fV", m_powerData.ports[2].voltage / 1000.0f);
                lv_label_set_text(ui2_port3voltage, voltage_str);
            }
            
            if (ui2_port3current && m_powerData.ports[2].valid) {
                char current_str[16];
                snprintf(current_str, sizeof(current_str), "%.2fA", m_powerData.ports[2].current / 1000.0f);
                lv_label_set_text(ui2_port3current, current_str);
            }
            
            if (ui2_port4voltage && m_powerData.ports[3].valid) {
                char voltage_str[16];
                snprintf(voltage_str, sizeof(voltage_str), "%.2fV", m_powerData.ports[3].voltage / 1000.0f);
                lv_label_set_text(ui2_port4voltage, voltage_str);
            }
            
            if (ui2_port4current && m_powerData.ports[3].valid) {
                char current_str[16];
                snprintf(current_str, sizeof(current_str), "%.2fA", m_powerData.ports[3].current / 1000.0f);
                lv_label_set_text(ui2_port4current, current_str);
            }
            
            // UI2系统端口详细页面的数据更新
            updateUI2PortDetailPages();
        }
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
        // 根据当前主题更新相应的天气显示
        if (m_currentTheme == THEME_UI1) {
            // UI1系统的天气显示
            if (ui_temperatureLabel) {
                char temp_str[16];
                snprintf(temp_str, sizeof(temp_str), "%s度", currentWeather.temperature.c_str());
                lv_label_set_text(ui_temperatureLabel, temp_str);
            }
            
            if (ui_weatherLabel) {
                lv_label_set_text(ui_weatherLabel, currentWeather.weather.c_str());
            }
        } else if (m_currentTheme == THEME_UI2) {
            // UI2系统的天气显示
            if (ui2_temperatureLabel) {
                char temp_str[16];
                snprintf(temp_str, sizeof(temp_str), "%s度", currentWeather.temperature.c_str());
                lv_label_set_text(ui2_temperatureLabel, temp_str);
            }
            
            if (ui2_weatherLabel) {
                lv_label_set_text(ui2_weatherLabel, currentWeather.weather.c_str());
            }
        }
        
        //printf("[DisplayManager] 天气显示已更新：%s度 %s\n", 
        //       currentWeather.temperature.c_str(), 
        //       currentWeather.weather.c_str());
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

// === 屏幕模式管理功能实现 ===

/**
 * @brief 加载屏幕模式配置
 */
bool DisplayManager::loadScreenModeConfig() {
    if (!m_configStorage) {
        printf("[DisplayManager] 错误：配置存储未初始化\n");
        return false;
    }
    
    if (!m_configStorage->hasScreenConfigAsync(3000)) {
        printf("[DisplayManager] 未找到屏幕模式配置，使用默认配置\n");
        return false;
    }
    
    ScreenMode mode;
    int startHour, startMinute, endHour, endMinute, timeoutMinutes;
    
    if (m_configStorage->loadScreenConfigAsync(mode, startHour, startMinute, 
                                             endHour, endMinute, timeoutMinutes, 3000)) {
        m_screenMode = mode;
        m_screenStartHour = startHour;
        m_screenStartMinute = startMinute;
        m_screenEndHour = endHour;
        m_screenEndMinute = endMinute;
        m_screenTimeoutMinutes = timeoutMinutes;
        
        printf("[DisplayManager] 屏幕模式配置加载成功：模式=%d, 时间=%02d:%02d-%02d:%02d, 延时=%d分钟\n",
               m_screenMode, m_screenStartHour, m_screenStartMinute,
               m_screenEndHour, m_screenEndMinute, m_screenTimeoutMinutes);
        
        return true;
    } else {
        printf("[DisplayManager] 屏幕模式配置加载失败\n");
        return false;
    }
}

/**
 * @brief 设置屏幕模式
 */
void DisplayManager::setScreenMode(ScreenMode mode, int startHour, int startMinute, 
                                 int endHour, int endMinute, int timeoutMinutes) {
    DisplayMessage msg;
    msg.type = DisplayMessage::MSG_SCREEN_MODE_CHANGED;
    msg.data.screen_mode.mode = mode;
    msg.data.screen_mode.startHour = startHour;
    msg.data.screen_mode.startMinute = startMinute;
    msg.data.screen_mode.endHour = endHour;
    msg.data.screen_mode.endMinute = endMinute;
    msg.data.screen_mode.timeoutMinutes = timeoutMinutes;
    
    if (m_messageQueue) {
        xQueueSend(m_messageQueue, &msg, pdMS_TO_TICKS(100));
    }
}

/**
 * @brief 获取当前屏幕模式
 */
ScreenMode DisplayManager::getScreenMode() const {
    return m_screenMode;
}

/**
 * @brief 检查屏幕是否应该开启
 */
bool DisplayManager::shouldScreenBeOn() const {
    switch (m_screenMode) {
        case SCREEN_MODE_ALWAYS_ON:
            return true;
            
        case SCREEN_MODE_ALWAYS_OFF:
            return false;
            
        case SCREEN_MODE_SCHEDULED:
            return isInScheduledTime();
            
        case SCREEN_MODE_TIMEOUT:
            return !isTimeoutExpired();
            
        default:
            return true;
    }
}

/**
 * @brief 强制开启屏幕
 */
void DisplayManager::forceScreenOn() {
    DisplayMessage msg;
    msg.type = DisplayMessage::MSG_SCREEN_ON;
    
    if (m_messageQueue) {
        xQueueSend(m_messageQueue, &msg, pdMS_TO_TICKS(100));
    }
}

/**
 * @brief 强制关闭屏幕
 */
void DisplayManager::forceScreenOff() {
    DisplayMessage msg;
    msg.type = DisplayMessage::MSG_SCREEN_OFF;
    
    if (m_messageQueue) {
        xQueueSend(m_messageQueue, &msg, pdMS_TO_TICKS(100));
    }
}

/**
 * @brief 通知触摸活动
 */
void DisplayManager::notifyTouchActivity() {
    DisplayMessage msg;
    msg.type = DisplayMessage::MSG_TOUCH_ACTIVITY;
    
    if (m_messageQueue) {
        xQueueSend(m_messageQueue, &msg, pdMS_TO_TICKS(100));
    }
}

/**
 * @brief 检查屏幕是否开启
 */
bool DisplayManager::isScreenOn() const {
    return m_screenOn;
}

/**
 * @brief 处理屏幕模式逻辑
 */
void DisplayManager::processScreenModeLogic() {
    uint32_t currentTime = xTaskGetTickCount() * portTICK_PERIOD_MS;
    
    // 检查是否到了处理时间
    if (currentTime - m_lastScreenModeCheck < SCREEN_MODE_CHECK_INTERVAL) {
        return;
    }
    
    m_lastScreenModeCheck = currentTime;
    
    // 如果启用了功率控制且当前为延时模式，优先处理功率控制逻辑
    if (m_powerControlEnabled && m_screenMode == SCREEN_MODE_TIMEOUT) {
        processPowerControlLogic();
    } else if (m_screenMode == SCREEN_MODE_TIMEOUT) {
        // 延时模式下处理基于功率的延时逻辑
        processPowerBasedTimeoutLogic();
    } else {
        // 根据屏幕模式决定屏幕状态
        bool shouldBeOn = shouldScreenBeOn();
        
        if (shouldBeOn && !m_screenOn) {
            performScreenOn();
        } else if (!shouldBeOn && m_screenOn) {
            performScreenOff();
        }
    }
}

/**
 * @brief 检查定时模式是否应该开启屏幕
 */
bool DisplayManager::isInScheduledTime() const {
    // 获取当前时间
    time_t now;
    time(&now);
    struct tm* timeinfo = localtime(&now);
    
    int currentHour = timeinfo->tm_hour;
    int currentMinute = timeinfo->tm_min;
    
    // 计算当前时间的分钟数（从00:00开始）
    int currentMinutes = currentHour * 60 + currentMinute;
    int startMinutes = m_screenStartHour * 60 + m_screenStartMinute;
    int endMinutes = m_screenEndHour * 60 + m_screenEndMinute;
    
    // 处理跨天的情况
    if (startMinutes <= endMinutes) {
        // 同一天内的时间段
        return currentMinutes >= startMinutes && currentMinutes <= endMinutes;
    } else {
        // 跨天的时间段（例如：22:00 - 08:00）
        return currentMinutes >= startMinutes || currentMinutes <= endMinutes;
    }
}

/**
 * @brief 检查延时模式是否应该关闭屏幕
 */
bool DisplayManager::isTimeoutExpired() const {
    // 如果没有有效的功率数据，使用传统的触摸延时逻辑
    if (!m_powerData.valid) {
        uint32_t currentTime = xTaskGetTickCount() * portTICK_PERIOD_MS;
        uint32_t timeoutMs = m_screenTimeoutMinutes * 60 * 1000;
        return (currentTime - m_lastTouchTime) > timeoutMs;
    }
    
    uint32_t currentTime = xTaskGetTickCount() * portTICK_PERIOD_MS;
    uint32_t timeoutMs = m_screenTimeoutMinutes * 60 * 1000; // 转换为毫秒
    int currentPower = m_powerData.total_power;
    
    // 新的延时逻辑：只有当功率小于1W时才开始计算延时
    if (currentPower < 1000) { // 1W = 1000mW
        // 功率小于1W，检查是否已经开始计算延时
        if (m_isInLowPowerMode && m_lowPowerStartTime > 0) {
            // 已经在低功率模式，检查延时时间是否到达
            bool timeExpired = (currentTime - m_lowPowerStartTime) > timeoutMs;
            if (timeExpired) {
                printf("[DisplayManager] 延时模式：低功率%.1fW持续%.1f分钟，延时时间已到\n", 
                       currentPower / 1000.0f, (currentTime - m_lowPowerStartTime) / 60000.0f);
            }
            return timeExpired;
        } else {
            // 还没有开始计算延时，不应该熄屏
            return false;
        }
    } else {
        // 功率大于等于1W，不应该熄屏
        return false;
    }
}

/**
 * @brief 执行屏幕开启操作
 */
void DisplayManager::performScreenOn() {
    if (m_screenOn) {
        return; // 已经开启
    }
    
    printf("[DisplayManager] 开启屏幕\n");
    
    // 恢复屏幕亮度
    if (m_lvglDriver) {
        m_lvglDriver->setBrightness(m_brightness);
    }
    
    // 显示屏幕内容
    if (m_lvglDriver && m_lvglDriver->lock(1000)) {
        // 可以在这里添加屏幕开启时的UI更新逻辑
        // 例如：显示待机屏幕或恢复上次的屏幕内容
        m_lvglDriver->unlock();
    }
    
    m_screenOn = true;
    printf("[DisplayManager] 屏幕已开启\n");
}

/**
 * @brief 执行屏幕关闭操作
 */
void DisplayManager::performScreenOff() {
    if (!m_screenOn) {
        return; // 已经关闭
    }
    
    printf("[DisplayManager] 关闭屏幕\n");
    
    // 设置屏幕亮度为0（关闭背光）
    if (m_lvglDriver) {
        m_lvglDriver->setBrightness(0);
    }
    
    // 可以在这里添加屏幕关闭时的UI更新逻辑
    // 例如：显示黑屏或省电模式界面
    if (m_lvglDriver && m_lvglDriver->lock(1000)) {
        // 可以创建一个黑屏或省电模式的界面
        m_lvglDriver->unlock();
    }
    
    m_screenOn = false;
    printf("[DisplayManager] 屏幕已关闭\n");
}

/**
 * @brief 重置延时计时器
 */
void DisplayManager::resetTimeoutTimer() {
    m_lastTouchTime = xTaskGetTickCount() * portTICK_PERIOD_MS;
}

// === 功率控制屏幕功能实现 ===

/**
 * @brief 设置功率控制阈值
 */
void DisplayManager::setPowerControlThresholds(bool enablePowerControl, int powerOffThreshold, int powerOnThreshold) {
    m_powerControlEnabled = enablePowerControl;
    m_powerOffThreshold = powerOffThreshold;
    m_powerOnThreshold = powerOnThreshold;
    
    // 重置低功率状态
    m_isInLowPowerMode = false;
    m_lowPowerStartTime = 0;
    
    printf("[DisplayManager] 功率控制已%s：关闭阈值=%.1fW，开启阈值=%.1fW\n", 
           enablePowerControl ? "启用" : "禁用",
           powerOffThreshold / 1000.0f, 
           powerOnThreshold / 1000.0f);
    
    if (enablePowerControl) {
        printf("[DisplayManager] 注意：功率控制仅在延时模式(SCREEN_MODE_TIMEOUT)下生效\n");
    }
}

/**
 * @brief 获取功率控制启用状态
 */
bool DisplayManager::isPowerControlEnabled() const {
    return m_powerControlEnabled;
}

/**
 * @brief 获取当前功率状态
 */
int DisplayManager::getCurrentPower() const {
    if (m_powerData.valid) {
        return m_powerData.total_power;
    }
    return 0;
}

/**
 * @brief 更新UI2系统端口详细页面数据
 */
void DisplayManager::updateUI2PortDetailPages() {
    if (m_currentTheme != THEME_UI2 || !m_powerData.valid) {
        return;
    }
    
    // 更新端口1详细页面
    if (m_powerData.ports[0].valid) {
        // 电压显示
        if (ui2_port1voltage1) {
            char voltage_str[16];
            snprintf(voltage_str, sizeof(voltage_str), "%.3fV", m_powerData.ports[0].voltage / 1000.0f);
            lv_label_set_text(ui2_port1voltage1, voltage_str);
        }
        
        // 电流显示
        if (ui2_port1current1) {
            char current_str[16];
            snprintf(current_str, sizeof(current_str), "%.3fA", m_powerData.ports[0].current / 1000.0f);
            lv_label_set_text(ui2_port1current1, current_str);
        }
        
        // 功率显示
        if (ui2_port1powerlabel) {
            char power_str[16];
            snprintf(power_str, sizeof(power_str), "%.3fW", m_powerData.ports[0].power / 1000.0f);
            lv_label_set_text(ui2_port1powerlabel, power_str);
        }
        
        // 活动状态显示
        if (ui2_port1active) {
            lv_label_set_text(ui2_port1active, m_powerData.ports[0].state ? "已开启" : "未开启");
        }
        
        // 连接状态显示
        if (ui2_port1state) {
            lv_label_set_text(ui2_port1state, m_powerData.ports[0].state ? "已连接" : "未连接");
        }
        
        // 协议显示
        if (ui2_port1protocol) {
            lv_label_set_text(ui2_port1protocol, strlen(m_powerData.ports[0].protocol_name) > 0 ? m_powerData.ports[0].protocol_name : "未知");
        }
        
        // 制造商VID
        if (ui2_port1manufactuervid) {
            if (m_powerData.ports[0].manufacturer_vid > 0) {
                char vid_str[16];
                snprintf(vid_str, sizeof(vid_str), "0x%04X", m_powerData.ports[0].manufacturer_vid);
                lv_label_set_text(ui2_port1manufactuervid, vid_str);
            } else {
                lv_label_set_text(ui2_port1manufactuervid, "未知");
            }
        }
        
        // 线缆VID
        if (ui2_port1cablevid) {
            if (m_powerData.ports[0].cable_vid > 0) {
                char vid_str[16];
                snprintf(vid_str, sizeof(vid_str), "0x%04X", m_powerData.ports[0].cable_vid);
                lv_label_set_text(ui2_port1cablevid, vid_str);
            } else {
                lv_label_set_text(ui2_port1cablevid, "未知");
            }
        }
        
        // 最大电压
        if (ui2_port1maxvbusvoltage) {
            if (m_powerData.ports[0].cable_max_vbus_voltage > 0) {
                char voltage_str[16];
                snprintf(voltage_str, sizeof(voltage_str), "%.1fV", m_powerData.ports[0].cable_max_vbus_voltage / 1000.0f);
                lv_label_set_text(ui2_port1maxvbusvoltage, voltage_str);
            } else {
                lv_label_set_text(ui2_port1maxvbusvoltage, "未知");
            }
        }
        
        // 最大电流
        if (ui2_port1maxvbuscurrent) {
            if (m_powerData.ports[0].cable_max_vbus_current > 0) {
                char current_str[16];
                snprintf(current_str, sizeof(current_str), "%.2fA", m_powerData.ports[0].cable_max_vbus_current / 1000.0f);
                lv_label_set_text(ui2_port1maxvbuscurrent, current_str);
            } else {
                lv_label_set_text(ui2_port1maxvbuscurrent, "未知");
            }
        }
    }
    
    // 更新端口2详细页面
    if (m_powerData.ports[1].valid) {
        // 电压显示
        if (ui2_port2voltage1) {
            char voltage_str[16];
            snprintf(voltage_str, sizeof(voltage_str), "%.3fV", m_powerData.ports[1].voltage / 1000.0f);
            lv_label_set_text(ui2_port2voltage1, voltage_str);
        }
        
        // 电流显示
        if (ui2_port2current1) {
            char current_str[16];
            snprintf(current_str, sizeof(current_str), "%.3fA", m_powerData.ports[1].current / 1000.0f);
            lv_label_set_text(ui2_port2current1, current_str);
        }
        
        // 活动状态显示（powerlabel实际上是状态标签）
        if (ui2_port2powerlabel) {
            lv_label_set_text(ui2_port2powerlabel, m_powerData.ports[1].state ? "已开启" : "未开启");
        }
        
        // 功率显示（powerlabel1才是真正的功率标签）
        if (ui2_port2powerlabel1) {
            char power_str[16];
            snprintf(power_str, sizeof(power_str), "%.3fW", m_powerData.ports[1].power / 1000.0f);
            lv_label_set_text(ui2_port2powerlabel1, power_str);
        }
        
        // 状态显示
        if (ui2_port2state) {
            lv_label_set_text(ui2_port2state, m_powerData.ports[1].state ? "已连接" : "未连接");
        }
        
        // 协议显示
        if (ui2_port2protocol) {
            lv_label_set_text(ui2_port2protocol, strlen(m_powerData.ports[1].protocol_name) > 0 ? m_powerData.ports[1].protocol_name : "未知");
        }
        
        // 制造商VID
        if (ui2_port2manufactuervid) {
            if (m_powerData.ports[1].manufacturer_vid > 0) {
                char vid_str[16];
                snprintf(vid_str, sizeof(vid_str), "0x%04X", m_powerData.ports[1].manufacturer_vid);
                lv_label_set_text(ui2_port2manufactuervid, vid_str);
            } else {
                lv_label_set_text(ui2_port2manufactuervid, "未知");
            }
        }
        
        // 线缆VID
        if (ui2_port2cablevid) {
            if (m_powerData.ports[1].cable_vid > 0) {
                char vid_str[16];
                snprintf(vid_str, sizeof(vid_str), "0x%04X", m_powerData.ports[1].cable_vid);
                lv_label_set_text(ui2_port2cablevid, vid_str);
            } else {
                lv_label_set_text(ui2_port2cablevid, "未知");
            }
        }
        
        // 最大电压
        if (ui2_port2maxvbusvoltage) {
            if (m_powerData.ports[1].cable_max_vbus_voltage > 0) {
                char voltage_str[16];
                snprintf(voltage_str, sizeof(voltage_str), "%.1fV", m_powerData.ports[1].cable_max_vbus_voltage / 1000.0f);
                lv_label_set_text(ui2_port2maxvbusvoltage, voltage_str);
            } else {
                lv_label_set_text(ui2_port2maxvbusvoltage, "未知");
            }
        }
        
        // 最大电流
        if (ui2_port2maxvbuscurrent) {
            if (m_powerData.ports[1].cable_max_vbus_current > 0) {
                char current_str[16];
                snprintf(current_str, sizeof(current_str), "%.2fA", m_powerData.ports[1].cable_max_vbus_current / 1000.0f);
                lv_label_set_text(ui2_port2maxvbuscurrent, current_str);
            } else {
                lv_label_set_text(ui2_port2maxvbuscurrent, "未知");
            }
        }
    }
    
    // 更新端口3详细页面
    if (m_powerData.ports[2].valid) {
        // 电压显示
        if (ui2_port3voltage1) {
            char voltage_str[16];
            snprintf(voltage_str, sizeof(voltage_str), "%.3fV", m_powerData.ports[2].voltage / 1000.0f);
            lv_label_set_text(ui2_port3voltage1, voltage_str);
        }
        
        // 电流显示
        if (ui2_port3current1) {
            char current_str[16];
            snprintf(current_str, sizeof(current_str), "%.3fA", m_powerData.ports[2].current / 1000.0f);
            lv_label_set_text(ui2_port3current1, current_str);
        }
        
        // 活动状态显示（powerlabel实际上是状态标签）
        if (ui2_port3powerlabel) {
            lv_label_set_text(ui2_port3powerlabel, m_powerData.ports[2].state ? "已开启" : "未开启");
        }
        
        // 功率显示（powerlabel1才是真正的功率标签）
        if (ui2_port3powerlabel1) {
            char power_str[16];
            snprintf(power_str, sizeof(power_str), "%.3fW", m_powerData.ports[2].power / 1000.0f);
            lv_label_set_text(ui2_port3powerlabel1, power_str);
        }
        
        // 状态显示
        if (ui2_port3state) {
            lv_label_set_text(ui2_port3state, m_powerData.ports[2].state ? "已连接" : "未连接");
        }
        
        // 协议显示
        if (ui2_port3protocol) {
            lv_label_set_text(ui2_port3protocol, strlen(m_powerData.ports[2].protocol_name) > 0 ? m_powerData.ports[2].protocol_name : "未知");
        }
        
        // 制造商VID
        if (ui2_port3manufactuervid) {
            if (m_powerData.ports[2].manufacturer_vid > 0) {
                char vid_str[16];
                snprintf(vid_str, sizeof(vid_str), "0x%04X", m_powerData.ports[2].manufacturer_vid);
                lv_label_set_text(ui2_port3manufactuervid, vid_str);
            } else {
                lv_label_set_text(ui2_port3manufactuervid, "未知");
            }
        }
        
        // 线缆VID
        if (ui2_port3cablevid) {
            if (m_powerData.ports[2].cable_vid > 0) {
                char vid_str[16];
                snprintf(vid_str, sizeof(vid_str), "0x%04X", m_powerData.ports[2].cable_vid);
                lv_label_set_text(ui2_port3cablevid, vid_str);
            } else {
                lv_label_set_text(ui2_port3cablevid, "未知");
            }
        }
        
        // 最大电压
        if (ui2_port3maxvbusvoltage) {
            if (m_powerData.ports[2].cable_max_vbus_voltage > 0) {
                char voltage_str[16];
                snprintf(voltage_str, sizeof(voltage_str), "%.1fV", m_powerData.ports[2].cable_max_vbus_voltage / 1000.0f);
                lv_label_set_text(ui2_port3maxvbusvoltage, voltage_str);
            } else {
                lv_label_set_text(ui2_port3maxvbusvoltage, "未知");
            }
        }
        
        // 最大电流
        if (ui2_port3maxvbuscurrent) {
            if (m_powerData.ports[2].cable_max_vbus_current > 0) {
                char current_str[16];
                snprintf(current_str, sizeof(current_str), "%.2fA", m_powerData.ports[2].cable_max_vbus_current / 1000.0f);
                lv_label_set_text(ui2_port3maxvbuscurrent, current_str);
            } else {
                lv_label_set_text(ui2_port3maxvbuscurrent, "未知");
            }
        }
    }
    
    // 更新端口4详细页面
    if (m_powerData.ports[3].valid) {
        // 电压显示
        if (ui2_port4voltage1) {
            char voltage_str[16];
            snprintf(voltage_str, sizeof(voltage_str), "%.3fV", m_powerData.ports[3].voltage / 1000.0f);
            lv_label_set_text(ui2_port4voltage1, voltage_str);
        }
        
        // 电流显示
        if (ui2_port4current1) {
            char current_str[16];
            snprintf(current_str, sizeof(current_str), "%.3fA", m_powerData.ports[3].current / 1000.0f);
            lv_label_set_text(ui2_port4current1, current_str);
        }
        
        // 活动状态显示（powerlabel实际上是状态标签）
        if (ui2_port4powerlabel) {
            lv_label_set_text(ui2_port4powerlabel, m_powerData.ports[3].state ? "已开启" : "未开启");
        }
        
        // 功率显示（powerlabel1才是真正的功率标签）
        if (ui2_port4powerlabel1) {
            char power_str[16];
            snprintf(power_str, sizeof(power_str), "%.3fW", m_powerData.ports[3].power / 1000.0f);
            lv_label_set_text(ui2_port4powerlabel1, power_str);
        }
        
        // 状态显示
        if (ui2_port4state) {
            lv_label_set_text(ui2_port4state, m_powerData.ports[3].state ? "已连接" : "未连接");
        }
        
        // 协议显示
        if (ui2_port4protocol) {
            lv_label_set_text(ui2_port4protocol, strlen(m_powerData.ports[3].protocol_name) > 0 ? m_powerData.ports[3].protocol_name : "未知");
        }
        
        // 制造商VID
        if (ui2_port4manufactuervid) {
            if (m_powerData.ports[3].manufacturer_vid > 0) {
                char vid_str[16];
                snprintf(vid_str, sizeof(vid_str), "0x%04X", m_powerData.ports[3].manufacturer_vid);
                lv_label_set_text(ui2_port4manufactuervid, vid_str);
            } else {
                lv_label_set_text(ui2_port4manufactuervid, "未知");
            }
        }
        
        // 线缆VID
        if (ui2_port4cablevid) {
            if (m_powerData.ports[3].cable_vid > 0) {
                char vid_str[16];
                snprintf(vid_str, sizeof(vid_str), "0x%04X", m_powerData.ports[3].cable_vid);
                lv_label_set_text(ui2_port4cablevid, vid_str);
            } else {
                lv_label_set_text(ui2_port4cablevid, "未知");
            }
        }
        
        // 最大电压
        if (ui2_port4maxvbusvoltage) {
            if (m_powerData.ports[3].cable_max_vbus_voltage > 0) {
                char voltage_str[16];
                snprintf(voltage_str, sizeof(voltage_str), "%.1fV", m_powerData.ports[3].cable_max_vbus_voltage / 1000.0f);
                lv_label_set_text(ui2_port4maxvbusvoltage, voltage_str);
            } else {
                lv_label_set_text(ui2_port4maxvbusvoltage, "未知");
            }
        }
        
        // 最大电流
        if (ui2_port4maxvbuscurrent) {
            if (m_powerData.ports[3].cable_max_vbus_current > 0) {
                char current_str[16];
                snprintf(current_str, sizeof(current_str), "%.2fA", m_powerData.ports[3].cable_max_vbus_current / 1000.0f);
                lv_label_set_text(ui2_port4maxvbuscurrent, current_str);
            } else {
                lv_label_set_text(ui2_port4maxvbuscurrent, "未知");
            }
        }
    }
}

/**
 * @brief 检查功率状态并管理屏幕
 */
void DisplayManager::processPowerControlLogic() {
    if (!m_powerData.valid) {
        return;
    }
    
    // 功率控制只在延时模式下生效
    if (m_screenMode != SCREEN_MODE_TIMEOUT) {
        return;
    }
    
    uint32_t currentTime = xTaskGetTickCount() * portTICK_PERIOD_MS;
    int currentPower = m_powerData.total_power;
    
    // 检查是否应该立即开启屏幕（功率大于开启阈值）
    if (currentPower > m_powerOnThreshold) {
        if (!m_screenOn) {
            printf("[DisplayManager] 功率控制：检测到高功率 %.1fW > %.1fW，立即开启屏幕\n", 
                   currentPower / 1000.0f, m_powerOnThreshold / 1000.0f);
            performScreenOn();
        }
        
        // 重置低功率状态
        m_isInLowPowerMode = false;
        m_lowPowerStartTime = 0;
        
        // 同时重置触摸延时计时器，保持屏幕活跃
        m_lastTouchTime = currentTime;
        return;
    }
    
    // 检查是否应该延时关闭屏幕（功率小于关闭阈值）
    if (currentPower < m_powerOffThreshold) {
        if (!m_isInLowPowerMode) {
            // 刚进入低功率模式，开始计时
            m_isInLowPowerMode = true;
            m_lowPowerStartTime = currentTime;
            printf("[DisplayManager] 功率控制：检测到低功率 %.1fW < %.1fW，开始延时关闭计时\n", 
                   currentPower / 1000.0f, m_powerOffThreshold / 1000.0f);
        } else {
            // 已经在低功率模式，检查是否到达延时时间
            uint32_t lowPowerDuration = currentTime - m_lowPowerStartTime;
            uint32_t timeoutMs = m_screenTimeoutMinutes * 60 * 1000;
            
            if (lowPowerDuration >= timeoutMs && m_screenOn) {
                printf("[DisplayManager] 功率控制：低功率持续 %.1f分钟，延时关闭屏幕\n", 
                       lowPowerDuration / 60000.0f);
                performScreenOff();
            }
        }
    } else {
        // 功率在阈值之间，保持当前状态但重置低功率计时器
        if (m_isInLowPowerMode) {
            m_isInLowPowerMode = false;
            m_lowPowerStartTime = 0;
            printf("[DisplayManager] 功率控制：功率恢复到 %.1fW，取消延时关闭\n", 
                   currentPower / 1000.0f);
        }
    }
    
    // 如果不在低功率模式，继续检查正常的屏幕模式逻辑
    if (!m_isInLowPowerMode) {
        bool shouldBeOn = shouldScreenBeOn();
        
        if (shouldBeOn && !m_screenOn) {
            performScreenOn();
        } else if (!shouldBeOn && m_screenOn) {
            performScreenOff();
        }
    }
}

/**
 * @brief 检查是否应该基于功率开启屏幕
 */
bool DisplayManager::shouldScreenBeOnBasedOnPower() const {
    if (!m_powerData.valid) {
        return true; // 没有功率数据时默认开启
    }
    
    // 功率控制只在延时模式下生效
    if (m_screenMode != SCREEN_MODE_TIMEOUT) {
        return shouldScreenBeOn(); // 非延时模式时使用正常逻辑
    }
    
    int currentPower = m_powerData.total_power;
    
    // 高功率时立即开启
    if (currentPower > m_powerOnThreshold) {
        return true;
    }
    
    // 低功率时检查是否在延时时间内
    if (currentPower < m_powerOffThreshold && m_isInLowPowerMode) {
        uint32_t currentTime = xTaskGetTickCount() * portTICK_PERIOD_MS;
        uint32_t lowPowerDuration = currentTime - m_lowPowerStartTime;
        uint32_t timeoutMs = m_screenTimeoutMinutes * 60 * 1000;
        
        // 在延时时间内保持开启
        return lowPowerDuration < timeoutMs;
    }
    
    // 其他情况根据正常模式决定
    return shouldScreenBeOn();
}

/**
 * @brief 处理基于功率的延时逻辑
 */
void DisplayManager::processPowerBasedTimeoutLogic() {
    if (!m_powerData.valid) {
        // 没有功率数据时，使用传统的触摸延时逻辑
        bool shouldBeOn = shouldScreenBeOn();
        
        if (shouldBeOn && !m_screenOn) {
            performScreenOn();
        } else if (!shouldBeOn && m_screenOn) {
            performScreenOff();
        }
        return;
    }
    
    uint32_t currentTime = xTaskGetTickCount() * portTICK_PERIOD_MS;
    int currentPower = m_powerData.total_power;
    
    // 检查功率状态并管理延时计算
    if (currentPower < 1000) { // 1W = 1000mW
        // 功率小于1W，检查是否需要开始延时计算
        if (!m_isInLowPowerMode) {
            // 开始低功率延时计算
            m_isInLowPowerMode = true;
            m_lowPowerStartTime = currentTime;
            printf("[DisplayManager] 延时模式：检测到低功率 %.1fW < 1.0W，开始延时计算\n", 
                   currentPower / 1000.0f);
        }
        
        // 检查延时时间是否到达
        uint32_t timeoutMs = m_screenTimeoutMinutes * 60 * 1000;
        uint32_t lowPowerDuration = currentTime - m_lowPowerStartTime;
        
        if (lowPowerDuration >= timeoutMs && m_screenOn) {
            printf("[DisplayManager] 延时模式：低功率 %.1fW 持续 %.1f分钟，关闭屏幕\n", 
                   currentPower / 1000.0f, lowPowerDuration / 60000.0f);
            performScreenOff();
        } else if (!m_screenOn && lowPowerDuration < timeoutMs) {
            // 如果屏幕关闭但延时时间未到，重新开启（可能是由于其他原因关闭的）
            performScreenOn();
        }
    } else {
        // 功率大于等于1W，重置延时计算
        if (m_isInLowPowerMode) {
            m_isInLowPowerMode = false;
            m_lowPowerStartTime = 0;
            printf("[DisplayManager] 延时模式：功率恢复到 %.1fW >= 1.0W，重置延时计算\n", 
                   currentPower / 1000.0f);
        }
        
        // 功率充足时确保屏幕开启
        if (!m_screenOn) {
            performScreenOn();
        }
        
        // 重置触摸延时计时器，保持屏幕活跃
        m_lastTouchTime = currentTime;
    }
}

// === 自动切换端口功能实现 ===

/**
 * @brief 启用或禁用自动切换端口功能
 */
void DisplayManager::setAutoSwitchEnabled(bool enabled) {
    m_autoSwitchEnabled = enabled;
    printf("[DisplayManager] 自动切换端口功能已%s\n", enabled ? "启用" : "禁用");
}

/**
 * @brief 获取自动切换端口功能状态
 */
bool DisplayManager::isAutoSwitchEnabled() const {
    return m_autoSwitchEnabled;
}

/**
 * @brief 检查端口功率变化并触发自动切换
 */
void DisplayManager::checkPortPowerChange() {
    if (!m_powerData.valid || m_isInAutoSwitchMode) {
        return; // 数据无效或已在自动切换模式中
    }
    
    for (int i = 0; i < 4; i++) {
        if (m_powerData.ports[i].valid) {
            bool currentState = m_powerData.ports[i].state;
            int currentPower = m_powerData.ports[i].power;
            
            // 检查端口状态从无功率变为有功率，并且距离上次自动切换至少5秒
            uint32_t currentTime = xTaskGetTickCount() * portTICK_PERIOD_MS;
            uint32_t cooldownTime = 5000; // 5秒冷却时间
            
            if (currentState && currentPower > 0 && 
                (!m_previousPortState[i] || m_previousPortPower[i] == 0) &&
                (currentTime - m_lastAutoSwitchTime[i]) >= cooldownTime) {
                
                printf("[DisplayManager] 检测到端口%d功率变化：0W -> %.2fW，触发自动切换\n", 
                       i + 1, currentPower / 1000.0f);
                
                // 立即更新历史数据和冷却时间，防止重复触发
                m_previousPortState[i] = currentState;
                m_previousPortPower[i] = currentPower;
                m_lastAutoSwitchTime[i] = currentTime;
                
                // 发送自动切换消息
                DisplayMessage msg;
                msg.type = DisplayMessage::MSG_AUTO_SWITCH_PORT;
                msg.data.auto_switch_port.port_index = i;
                msg.data.auto_switch_port.duration_ms = 10000; // 10秒
                
                if (m_messageQueue) {
                    xQueueSend(m_messageQueue, &msg, pdMS_TO_TICKS(100));
                }
                
                break; // 只处理第一个检测到的端口变化
            } else {
                // 只有在没有触发自动切换时才更新历史数据
                m_previousPortState[i] = currentState;
                m_previousPortPower[i] = currentPower;
            }
        }
    }
}

/**
 * @brief 执行自动切换到端口屏幕
 */
void DisplayManager::performAutoSwitchToPort(int port_index, uint32_t duration_ms) {
    if (port_index < 0 || port_index >= 4 || m_isInAutoSwitchMode) {
        return;
    }
    
    printf("[DisplayManager] 开始自动切换到端口%d屏幕，持续%d毫秒\n", port_index + 1, duration_ms);
    
    // 保存当前页面
    m_previousPage = m_currentPage;
    m_autoSwitchStartTime = xTaskGetTickCount() * portTICK_PERIOD_MS;
    m_autoSwitchDuration = duration_ms;
    m_isInAutoSwitchMode = true;
    m_currentAutoSwitchPort = port_index;
    
    // 切换到端口屏幕
    switchToPortScreen(port_index);
}

/**
 * @brief 检查自动切换是否应该结束
 */
void DisplayManager::checkAutoSwitchTimeout() {
    if (!m_isInAutoSwitchMode) {
        return;
    }
    
    uint32_t currentTime = xTaskGetTickCount() * portTICK_PERIOD_MS;
    uint32_t elapsedTime = currentTime - m_autoSwitchStartTime;
    
    if (elapsedTime >= m_autoSwitchDuration) {
        printf("[DisplayManager] 自动切换端口%d超时，恢复到页面%d\n", 
               m_currentAutoSwitchPort + 1, m_previousPage);
        
        restorePreviousPage();
    }
}

/**
 * @brief 恢复到切换前的页面
 */
void DisplayManager::restorePreviousPage() {
    if (!m_isInAutoSwitchMode) {
        return;
    }
    
    printf("[DisplayManager] 开始恢复到页面%d\n", m_previousPage);
    
    // 重置自动切换状态
    m_isInAutoSwitchMode = false;
    m_currentAutoSwitchPort = -1;
    
    // 使用switchPage方法恢复到之前的页面
    switchPage(m_previousPage);
    
    printf("[DisplayManager] 已请求恢复到页面%d\n", m_previousPage);
}

/**
 * @brief 根据当前主题切换到对应的端口屏幕
 */
void DisplayManager::switchToPortScreen(int port_index) {
    if (port_index < 0 || port_index >= 4) {
        return;
    }
    
    // 使用手动切换方法，这会通过消息队列处理实际的屏幕切换
    DisplayPage targetPage = (DisplayPage)(PAGE_POWER_PORT1 + port_index);
    manualSwitchPage(targetPage);
    
    printf("[DisplayManager] 请求切换到端口%d屏幕\n", port_index + 1);
}

/**
 * @brief 设置全局实例指针（供UI系统回调使用）
 */
void DisplayManager::setInstance(DisplayManager* instance) {
    s_instance = instance;
}

/**
 * @brief 获取全局实例指针
 */
DisplayManager* DisplayManager::getInstance() {
    return s_instance;
}

/**
 * @brief 更新当前页面状态（供UI系统回调使用）
 */
void DisplayManager::updateCurrentPage(DisplayPage page) {
    if (m_currentPage != page) {
        printf("[DisplayManager] 页面状态更新：%d -> %d\n", m_currentPage, page);
        m_currentPage = page;
    }
}

/**
 * @brief 根据屏幕对象更新当前页面状态
 */
void DisplayManager::updateCurrentPageByScreen(lv_obj_t* screen) {
    if (!screen) return;
    
    // 根据当前主题和屏幕对象确定页面类型
    if (m_currentTheme == THEME_UI1) {
        if (screen == ui_standbySCREEN) {
            updateCurrentPage(PAGE_HOME);
        } else if (screen == ui_totalpowerSCREEN) {
            updateCurrentPage(PAGE_POWER_TOTAL);
        } else if (screen == ui_prot1SCREEN) {
            updateCurrentPage(PAGE_POWER_PORT1);
        } else if (screen == ui_prot2SCREEN) {
            updateCurrentPage(PAGE_POWER_PORT2);
        } else if (screen == ui_prot3SCREEN) {
            updateCurrentPage(PAGE_POWER_PORT3);
        } else if (screen == ui_prot4SCREEN) {
            updateCurrentPage(PAGE_POWER_PORT4);
        }
    } else if (m_currentTheme == THEME_UI2) {
        if (screen == ui2_standbySCREEN) {
            updateCurrentPage(PAGE_HOME);
        } else if (screen == ui2_totalpowerSCREEN) {
            updateCurrentPage(PAGE_POWER_TOTAL);
        } else if (screen == ui2_port1SCREEN) {
            updateCurrentPage(PAGE_POWER_PORT1);
        } else if (screen == ui2_port2SCREEN) {
            updateCurrentPage(PAGE_POWER_PORT2);
        } else if (screen == ui2_port3SCREEN) {
            updateCurrentPage(PAGE_POWER_PORT3);
        } else if (screen == ui2_port4SCREEN) {
            updateCurrentPage(PAGE_POWER_PORT4);
        }
    }
}

// C风格包装函数，供UI系统调用
extern "C" void updateDisplayManagerCurrentPage(void* screen) {
    DisplayManager* instance = DisplayManager::getInstance();
    if (instance) {
        instance->updateCurrentPageByScreen((lv_obj_t*)screen);
    }
}

// === 基于总功率的自动页面切换功能实现 ===

/**
 * @brief 设置基于总功率的自动页面切换功能
 */
void DisplayManager::setPowerBasedAutoSwitch(bool enabled, int lowPowerThreshold, int highPowerThreshold, uint32_t checkInterval) {
    m_powerBasedAutoSwitchEnabled = enabled;
    m_lowPowerThreshold = lowPowerThreshold;
    m_highPowerThreshold = highPowerThreshold;
    m_powerCheckInterval = checkInterval;
    
    printf("[DisplayManager] 基于总功率的自动页面切换已%s：低功率阈值=%.1fW，高功率阈值=%.1fW，检查间隔=%d毫秒\n", 
           enabled ? "启用" : "禁用",
           lowPowerThreshold / 1000.0f, 
           highPowerThreshold / 1000.0f,
           checkInterval);
}

/**
 * @brief 获取基于总功率的自动页面切换功能状态
 */
bool DisplayManager::isPowerBasedAutoSwitchEnabled() const {
    return m_powerBasedAutoSwitchEnabled;
}

/**
 * @brief 手动切换页面（会标记为手动切换）
 */
void DisplayManager::manualSwitchPage(DisplayPage page) {
    printf("[DisplayManager] 手动切换到页面：%d\n", page);
    
    // 标记为手动切换
    m_isManualSwitch = true;
    m_lastManualPage = page;
    
    // 执行页面切换
    switchPage(page);
}

/**
 * @brief 检查总功率变化并执行自动页面切换
 */
void DisplayManager::checkPowerBasedPageSwitch() {
    if (!m_powerBasedAutoSwitchEnabled || !m_powerData.valid) {
        return;
    }
    
    uint32_t currentTime = xTaskGetTickCount() * portTICK_PERIOD_MS;
    
    // 检查是否到了检查时间
    if (currentTime - m_lastPowerCheckTime < m_powerCheckInterval) {
        return;
    }
    
    m_lastPowerCheckTime = currentTime;
    
    // 如果当前是手动切换且是端口页面，不执行自动切换
    if (m_isManualSwitch && isPortPage(m_currentPage)) {
        printf("[DisplayManager] 当前为手动切换的端口页面，跳过自动切换\n");
        return;
    }
    
    // 如果当前是手动切换且不是端口页面，重置手动切换标志
    if (m_isManualSwitch && !isPortPage(m_currentPage)) {
        printf("[DisplayManager] 手动切换到非端口页面，重置手动切换标志\n");
        m_isManualSwitch = false;
    }
    
    // 如果当前是端口页面（无论是手动还是自动切换的），不执行基于功率的自动切换
    if (isPortPage(m_currentPage)) {
        printf("[DisplayManager] 当前为端口页面，跳过基于功率的自动切换\n");
        return;
    }
    
    // 检查是否应该执行自动切换
    if (!shouldExecuteAutoSwitch()) {
        return;
    }
    
    // 根据总功率决定目标页面
    DisplayPage targetPage = getTargetPageByPower();
    
    // 如果目标页面与当前页面不同，执行切换
    if (targetPage != m_currentPage) {
        printf("[DisplayManager] 基于总功率%.1fW自动切换页面：%d -> %d\n", 
               m_powerData.total_power / 1000.0f, m_currentPage, targetPage);
        
        switchPage(targetPage);
    }
}

/**
 * @brief 根据总功率决定应该显示的页面
 */
DisplayPage DisplayManager::getTargetPageByPower() const {
    if (!m_powerData.valid) {
        return PAGE_HOME; // 没有功率数据时显示待机页面
    }
    
    int totalPower = m_powerData.total_power;
    
    if (totalPower < m_lowPowerThreshold) {
        // 总功率小于低功率阈值，显示待机页面
        return PAGE_HOME;
    } else if (totalPower > m_highPowerThreshold) {
        // 总功率大于高功率阈值，显示总功率页面
        return PAGE_POWER_TOTAL;
    } else {
        // 功率在阈值之间，保持当前页面
        return m_currentPage;
    }
}

/**
 * @brief 检查是否为端口页面
 */
bool DisplayManager::isPortPage(DisplayPage page) const {
    return (page == PAGE_POWER_PORT1 || 
            page == PAGE_POWER_PORT2 || 
            page == PAGE_POWER_PORT3 || 
            page == PAGE_POWER_PORT4);
}

/**
 * @brief 检查是否应该执行自动切换
 */
bool DisplayManager::shouldExecuteAutoSwitch() const {
    // 如果当前在自动切换端口模式中，不执行基于功率的自动切换
    if (m_isInAutoSwitchMode) {
        return false;
    }
    
    // 如果当前是手动切换的端口页面，不执行自动切换
    if (m_isManualSwitch && isPortPage(m_currentPage)) {
        return false;
    }
    
    // 如果当前是端口页面（无论是手动还是自动切换的），不执行基于功率的自动切换
    if (isPortPage(m_currentPage)) {
        return false;
    }
    
    // 如果功率数据无效，不执行自动切换
    if (!m_powerData.valid) {
        return false;
    }
    
    return true;
}