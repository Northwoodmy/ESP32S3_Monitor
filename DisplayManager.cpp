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
#include <cmath>
#include <time.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

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
    , m_lastPageSwitchTime(0)
    , m_pendingTargetPage(PAGE_HOME)
    , m_pageChangeStartTime(0)
    , m_isPageChangePending(false)
    , m_fadingEnabled(true)
    , m_currentFadingBrightness(80)
    , m_targetFadingBrightness(80)
    , m_fadeStartTime(0)
    , m_fadeDuration(1000)
    , m_isFading(false)
    , m_fadeDirection(FADE_TO_ON)
    , m_otaScreen(nullptr)
    , m_otaProgressBar(nullptr)
    , m_otaStatusLabel(nullptr)
    , m_otaProgressLabel(nullptr)
    , m_otaSizeLabel(nullptr)
    , m_otaErrorLabel(nullptr)
    , m_otaDisplayActive(false)
    , m_otaInProgress(false)
    , m_previousPageForOTA(PAGE_HOME)
{
    // 设置全局实例指针
    s_instance = this;
    
    // 初始化功率数据
    memset(&m_powerData, 0, sizeof(m_powerData));
    m_powerData.port_count = 4;
    m_powerData.valid = false;
    
    // 初始化端口功率和状态历史
    for (int i = 0; i < 4; i++) {
        m_previousPortPower[i] = 0;
        strcpy(m_previousPortState[i], "UNKNOWN");
        m_lastAutoSwitchTime[i] = 0;
    }
    
    printf("[DisplayManager] 显示管理器已创建（新UI系统）\n");
}

/**
 * @brief 析构函数
 */
DisplayManager::~DisplayManager() {
    stop();
    
    // 清理OTA页面资源
    destroyOTAProgressPage();
    
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
        
        // 处理亮度渐变（更高频率更新以确保平滑渐变）
        if (m_isFading && m_fadingEnabled) {
            processFading();
        }
        
        // 渐变期间使用更短的延迟以获得更平滑的效果
        if (m_isFading && m_fadingEnabled) {
            vTaskDelay(pdMS_TO_TICKS(10)); // 渐变时10ms更新间隔
        } else {
            vTaskDelay(pdMS_TO_TICKS(50)); // 正常时50ms延迟
        }
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
            // 显示通知（暂时简化为控制台输出，后续可在UI1/UI2系统中重新实现）
            printf("[DisplayManager] 通知：%s\n", msg.data.notification.text);
            // TODO: 在UI1和UI2系统中实现通知显示功能
            break;
            
        case DisplayMessage::MSG_SCREEN_MODE_CHANGED:
            // 屏幕模式变更
            m_screenMode = msg.data.screen_mode.mode;
            m_screenStartHour = msg.data.screen_mode.startHour;
            m_screenStartMinute = msg.data.screen_mode.startMinute;
            m_screenEndHour = msg.data.screen_mode.endHour;
            m_screenEndMinute = msg.data.screen_mode.endMinute;
            m_screenTimeoutMinutes = msg.data.screen_mode.timeoutMinutes;
            
            // 设置自动旋转
            if (m_lvglDriver) {
                m_lvglDriver->setAutoRotationEnabled(msg.data.screen_mode.autoRotationEnabled);
                // 如果自动旋转被禁用，应用静态旋转角度
                if (!msg.data.screen_mode.autoRotationEnabled) {
                    m_lvglDriver->setScreenRotation((screen_rotation_t)msg.data.screen_mode.staticRotation);
                    printf("[DisplayManager] 应用静态旋转角度: %d度\n", msg.data.screen_mode.staticRotation * 90);
                }
                printf("[DisplayManager] 自动旋转已设置为: %s\n", 
                       msg.data.screen_mode.autoRotationEnabled ? "启用" : "禁用");
            }
            
            printf("[DisplayManager] 屏幕模式已更改: 模式=%d, 时间=%02d:%02d-%02d:%02d, 延时=%d分钟, 自动旋转=%s, 静态旋转=%d度\n",
                   m_screenMode, m_screenStartHour, m_screenStartMinute,
                   m_screenEndHour, m_screenEndMinute, m_screenTimeoutMinutes,
                   msg.data.screen_mode.autoRotationEnabled ? "启用" : "禁用", msg.data.screen_mode.staticRotation * 90);
            
            // 重新评估屏幕状态
            processScreenModeLogic();
            break;
            
        case DisplayMessage::MSG_TOUCH_ACTIVITY:
            // 触摸活动
            m_lastTouchTime = xTaskGetTickCount() * portTICK_PERIOD_MS;
            //printf("[DisplayManager] 触摸活动检测，延时计时器已重置\n");
            
            // 如果屏幕当前关闭，触摸活动应立即开启屏幕
            if (!m_screenOn) {
                //printf("[DisplayManager] 触摸唤醒：屏幕已关闭，立即开启屏幕\n");
                performScreenOn();
            } else {
                //printf("[DisplayManager] 触摸活动：屏幕已开启，重置延时计时器\n");
            }
            
            // 在功率控制模式或基于功率的延时模式下，触摸活动应该重置低功率状态
            if ((m_powerControlEnabled || !m_powerData.valid) && m_isInLowPowerMode) {
                printf("[DisplayManager] 触摸唤醒：重置低功率模式状态\n");
                m_isInLowPowerMode = false;
                m_lowPowerStartTime = 0;
            }
            break;
            
        case DisplayMessage::MSG_SCREEN_ON:
            // 强制开启屏幕
            performScreenOn();
            break;
            
        case DisplayMessage::MSG_SCREEN_OFF:
            // 强制关闭屏幕（OTA期间除外）
            if (m_otaInProgress) {
                printf("[DisplayManager] MSG_SCREEN_OFF：OTA升级进行中，跳过屏幕关闭操作\n");
            } else {
                performScreenOff();
            }
            break;
            
        case DisplayMessage::MSG_AUTO_SWITCH_PORT:
            // 自动切换到端口屏幕
            performAutoSwitchToPort(msg.data.auto_switch_port.port_index, 
                                  msg.data.auto_switch_port.duration_ms);
            break;
            
        case DisplayMessage::MSG_UPDATE_OTA_STATUS:
            // 更新OTA状态显示
            if (m_otaDisplayActive) {
                updateOTAProgressBar(msg.data.ota_status.progress);
                updateOTAStatusText(msg.data.ota_status.statusText);
                updateOTASizeInfo(msg.data.ota_status.totalSize, msg.data.ota_status.writtenSize);
                
                // 显示错误信息（如果有）
                if (strlen(msg.data.ota_status.errorMessage) > 0) {
                    showOTAError(msg.data.ota_status.errorMessage);
                }
            }
            break;
            
                 case DisplayMessage::MSG_OTA_START:
             // 开始OTA升级显示
             if (!m_otaDisplayActive) {
                 m_previousPageForOTA = m_currentPage;  // 保存当前页面
                 createOTAProgressPage();
                 m_otaDisplayActive = true;
                 printf("[DisplayManager] OTA upgrade display started\n");
             }
             break;
            
        case DisplayMessage::MSG_OTA_COMPLETE:
            // 完成OTA升级显示
            if (m_otaDisplayActive) {
                // 显示完成状态3秒后自动恢复到之前页面（如果不是成功需要重启）
                // 这里简化处理，直接恢复页面
                                 if (msg.data.ota_status.status != 4) {  // 不是成功状态
                     vTaskDelay(pdMS_TO_TICKS(3000));  // 等待3秒显示结果
                     destroyOTAProgressPage();
                     switchPage(m_previousPageForOTA);  // 恢复到之前页面
                     m_otaDisplayActive = false;
                     printf("[DisplayManager] OTA upgrade display completed, restoring previous page\n");
                 }
                // 成功状态通常会重启设备，不需要恢复页面
            }
            break;
    }
    
    m_lvglDriver->unlock();
}

// === 旧款手动UI创建函数已全部删除，现使用SquareLine Studio生成的UI1和UI2系统 ===

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

// === 旧款手动UI更新和事件处理函数已全部删除，现使用SquareLine Studio生成的UI1和UI2系统 ===

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
        const char* state_text;
        if (strcmp(portData.state, "ATTACHED") == 0) {
            state_text = "已开启\n已连接";
        } else if (strcmp(portData.state, "ACTIVE") == 0) {
            state_text = "已开启\n未连接";
        } else {
            state_text = "未知状态";
        }
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
    bool autoRotationEnabled;
    int staticRotation;
    
    if (m_configStorage->loadScreenConfigAsync(mode, startHour, startMinute, 
                                             endHour, endMinute, timeoutMinutes, autoRotationEnabled, staticRotation, 3000)) {
        m_screenMode = mode;
        m_screenStartHour = startHour;
        m_screenStartMinute = startMinute;
        m_screenEndHour = endHour;
        m_screenEndMinute = endMinute;
        m_screenTimeoutMinutes = timeoutMinutes;
        
        // 设置自动旋转功能
        if (m_lvglDriver) {
            m_lvglDriver->setAutoRotationEnabled(autoRotationEnabled);
            // 如果自动旋转被禁用，应用静态旋转角度
            if (!autoRotationEnabled) {
                m_lvglDriver->setScreenRotation((screen_rotation_t)staticRotation);
                printf("[DisplayManager] 应用静态旋转角度: %d度\n", staticRotation * 90);
            }
            printf("[DisplayManager] 自动旋转配置已加载并应用: %s\n", 
                   autoRotationEnabled ? "启用" : "禁用");
        }
        
        printf("[DisplayManager] 屏幕模式配置加载成功：模式=%d, 时间=%02d:%02d-%02d:%02d, 延时=%d分钟, 自动旋转=%s, 静态旋转=%d度\n",
               m_screenMode, m_screenStartHour, m_screenStartMinute,
               m_screenEndHour, m_screenEndMinute, m_screenTimeoutMinutes,
               autoRotationEnabled ? "启用" : "禁用", staticRotation * 90);
        
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
                                 int endHour, int endMinute, int timeoutMinutes, bool autoRotationEnabled) {
    DisplayMessage msg;
    msg.type = DisplayMessage::MSG_SCREEN_MODE_CHANGED;
    msg.data.screen_mode.mode = mode;
    msg.data.screen_mode.startHour = startHour;
    msg.data.screen_mode.startMinute = startMinute;
    msg.data.screen_mode.endHour = endHour;
    msg.data.screen_mode.endMinute = endMinute;
    msg.data.screen_mode.timeoutMinutes = timeoutMinutes;
    msg.data.screen_mode.autoRotationEnabled = autoRotationEnabled;
    
    // 如果自动旋转被禁用，需要获取当前旋转角度并保存
    if (!autoRotationEnabled && m_lvglDriver) {
        msg.data.screen_mode.staticRotation = (int)m_lvglDriver->getScreenRotation();
        printf("[DisplayManager] 保存当前旋转角度: %d度\n", msg.data.screen_mode.staticRotation * 90);
    } else {
        msg.data.screen_mode.staticRotation = 0; // 默认为0度
    }
    
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
        BaseType_t result = xQueueSend(m_messageQueue, &msg, pdMS_TO_TICKS(100));
        if (result != pdTRUE) {
            printf("[DisplayManager] 警告：触摸活动消息发送失败\n");
        }
    } else {
        printf("[DisplayManager] 错误：消息队列为空\n");
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
            // OTA进行中时不允许关闭屏幕
            if (m_otaInProgress) {
                printf("[DisplayManager] 屏幕模式控制：OTA升级进行中，跳过屏幕关闭操作\n");
            } else {
                performScreenOff();
            }
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
    uint32_t currentTime = xTaskGetTickCount() * portTICK_PERIOD_MS;
    uint32_t timeoutMs = m_screenTimeoutMinutes * 60 * 1000;
    uint32_t timeSinceLastTouch = currentTime - m_lastTouchTime;
    
    // 如果没有有效的功率数据，使用传统的触摸延时逻辑
    if (!m_powerData.valid) {
        return timeSinceLastTouch > timeoutMs;
    }
    
    int currentPower = m_powerData.total_power;
    
    // 基于功率的延时逻辑：只有在低功率状态下才考虑延时关闭屏幕
    if (currentPower < 1000) { // 1W = 1000mW
        // 功率小于1W，检查从最后触摸时间开始的延时
        return timeSinceLastTouch > timeoutMs;
    } else {
        // 功率大于等于1W，不应该熄屏
        return false;
    }
}

/**
 * @brief 执行屏幕开启操作
 */
void DisplayManager::performScreenOn() {
    if (m_screenOn && (!m_isFading || m_fadeDirection == FADE_TO_ON)) {
        return; // 已经开启且不在关闭渐变中
    }
    
    // 如果正在渐变关闭，立即停止并开始开启渐变
    if (m_isFading && m_fadeDirection == FADE_TO_OFF) {
        stopFading();
    }
    
    printf("[DisplayManager] 开启屏幕\n");
    
    // 如果启用了渐变功能，使用渐变开启
    if (m_fadingEnabled) {
        printf("[DisplayManager] 使用亮度渐变开启屏幕（目标亮度：%d%%）\n", m_brightness);
        startFading(m_brightness, FADE_TO_ON);
    } else {
        // 不使用渐变，直接开启
        performScreenOnImmediate();
    }
}

/**
 * @brief 执行屏幕关闭操作
 */
void DisplayManager::performScreenOff() {
    if (!m_screenOn) {
        return; // 已经关闭
    }
    
    // 如果已经在渐变关闭过程中，不要重复启动
    if (m_isFading && m_fadeDirection == FADE_TO_OFF) {
        return; // 已经在渐变关闭中，避免重复调用
    }
    
    printf("[DisplayManager] 关闭屏幕（延时模式生效）\n");
    printf("[DisplayManager] 💡 屏幕已进入省电模式，触摸屏幕可立即唤醒\n");
    
    // 如果启用了渐变功能，使用渐变关闭
    if (m_fadingEnabled) {
        printf("[DisplayManager] 使用亮度渐变关闭屏幕\n");
        startFading(0, FADE_TO_OFF);
    } else {
        // 不使用渐变，直接关闭
        performScreenOffImmediate();
    }
}

/**
 * @brief 重置延时计时器
 */
void DisplayManager::resetTimeoutTimer() {
    m_lastTouchTime = xTaskGetTickCount() * portTICK_PERIOD_MS;
    
    // 如果当前处于低功率模式，重置低功率状态
    if (m_isInLowPowerMode) {
        m_isInLowPowerMode = false;
        m_lowPowerStartTime = 0;
    }
    
    // 如果屏幕关闭，立即开启
    if (!m_screenOn) {
        performScreenOn();
    }
}

/**
 * @brief 检查触摸唤醒功能是否可用
 */
bool DisplayManager::isTouchWakeupEnabled() const {
    // 触摸唤醒功能在延时模式下可用
    return (m_screenMode == SCREEN_MODE_TIMEOUT);
}

/**
 * @brief 获取触摸活动状态信息
 */
void DisplayManager::getTouchWakeupStatus(uint32_t& lastTouchTime, uint32_t& timeSinceLastTouch, bool& isInLowPower) const {
    lastTouchTime = m_lastTouchTime;
    timeSinceLastTouch = (xTaskGetTickCount() * portTICK_PERIOD_MS) - m_lastTouchTime;
    isInLowPower = m_isInLowPowerMode;
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
    
    // 优先检查触摸活动：如果最近有触摸活动，屏幕应该保持开启
    uint32_t timeSinceLastTouch = currentTime - m_lastTouchTime;
    uint32_t touchTimeoutMs = m_screenTimeoutMinutes * 60 * 1000;
    
    if (timeSinceLastTouch < touchTimeoutMs) {
        // 触摸延时时间内，确保屏幕开启
        if (!m_screenOn) {
            printf("[DisplayManager] 功率控制：触摸延时时间内，强制开启屏幕\n");
            performScreenOn();
        }
        return; // 触摸活动优先级最高，跳过功率控制逻辑
    }
    
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
                // OTA进行中时不允许关闭屏幕
                if (m_otaInProgress) {
                    printf("[DisplayManager] 功率控制：OTA升级进行中，跳过屏幕关闭操作\n");
                } else {
                    printf("[DisplayManager] 功率控制：低功率持续 %.1f分钟，延时关闭屏幕\n", 
                           lowPowerDuration / 60000.0f);
                    performScreenOff();
                }
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
    uint32_t timeSinceLastTouch = currentTime - m_lastTouchTime;
    uint32_t timeoutMs = m_screenTimeoutMinutes * 60 * 1000;
    
    // 新的延时逻辑：统一从最后触摸时间开始计算延时
    if (currentPower < 1000) { // 1W = 1000mW
        // 功率小于1W，检查是否应该延时关闭屏幕
        if (timeSinceLastTouch >= timeoutMs && m_screenOn) {
            // OTA进行中时不允许关闭屏幕
            if (m_otaInProgress) {
                printf("[DisplayManager] 延时模式：OTA升级进行中，跳过屏幕关闭操作\n");
            } else {
                printf("[DisplayManager] 延时模式：低功率 %.1fW，距上次触摸 %.1f分钟，关闭屏幕\n", 
                       currentPower / 1000.0f, timeSinceLastTouch / 60000.0f);
                performScreenOff();
            }
        } else if (!m_screenOn && timeSinceLastTouch < timeoutMs) {
            // 如果屏幕关闭但延时时间未到，重新开启（可能是由于其他原因关闭的）
            printf("[DisplayManager] 延时模式：低功率 %.1fW，但延时未到，开启屏幕\n", 
                   currentPower / 1000.0f);
            performScreenOn();
        }
        
        // 标记处于低功率模式（用于状态追踪）
        if (!m_isInLowPowerMode) {
            m_isInLowPowerMode = true;
            m_lowPowerStartTime = currentTime;
            printf("[DisplayManager] 延时模式：检测到低功率 %.1fW < 1.0W，进入低功率模式\n", 
                   currentPower / 1000.0f);
        }
    } else {
        // 功率大于等于1W，重置低功率状态并确保屏幕开启
        if (m_isInLowPowerMode) {
            m_isInLowPowerMode = false;
            m_lowPowerStartTime = 0;
            printf("[DisplayManager] 延时模式：功率恢复到 %.1fW >= 1.0W，退出低功率模式\n", 
                   currentPower / 1000.0f);
        }
        
        // 功率充足时确保屏幕开启并重置触摸时间
        if (!m_screenOn) {
            printf("[DisplayManager] 延时模式：高功率 %.1fW，强制开启屏幕\n", 
                   currentPower / 1000.0f);
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
            const char* currentState = m_powerData.ports[i].state;
            int currentPower = m_powerData.ports[i].power;
            
            // 检查端口功率从0变为非0（简化逻辑）
            // 触发条件：当前功率 > 0 且 历史功率 == 0
            bool currentHasPower = currentPower > 0;
            bool previousNoPower = m_previousPortPower[i] == 0;
            
            bool conditionMet = currentHasPower && previousNoPower;
            
            if (conditionMet) {
                printf("[DisplayManager] 检测到端口%d功率变化：0W -> %.1fW，立即触发自动切换\n", 
                       i + 1, currentPower / 1000.0f);
                
                // 立即更新历史数据，防止重复触发
                strncpy(m_previousPortState[i], currentState, 15);
                m_previousPortState[i][15] = '\0';
                m_previousPortPower[i] = currentPower;
                
                // 发送自动切换消息
                DisplayMessage msg;
                msg.type = DisplayMessage::MSG_AUTO_SWITCH_PORT;
                msg.data.auto_switch_port.port_index = i;
                msg.data.auto_switch_port.duration_ms = 20000; // 20秒
                
                if (m_messageQueue) {
                    xQueueSend(m_messageQueue, &msg, pdMS_TO_TICKS(100));
                }
                
                break; // 只处理第一个检测到的端口变化
            } else {
                // 更新历史数据
                strncpy(m_previousPortState[i], currentState, 15);
                m_previousPortState[i][15] = '\0';
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
 * @brief 恢复到切换前的页面（根据总功率条件智能决定）
 */
void DisplayManager::restorePreviousPage() {
    if (!m_isInAutoSwitchMode) {
        return;
    }
    
    // 重置自动切换状态
    m_isInAutoSwitchMode = false;
    m_currentAutoSwitchPort = -1;
    
    // 根据当前总功率条件决定切换到哪个页面
    DisplayPage targetPage;
    if (m_powerData.valid && m_powerData.total_power > 2000) {
        // 总功率大于2W，切换到总功率页面
        targetPage = PAGE_POWER_TOTAL;
        printf("[DisplayManager] 端口自动切换结束，总功率%.1fW > 2W，切换到总功率页面\n", 
               m_powerData.total_power / 1000.0f);
    } else {
        // 总功率小于等于2W，切换到待机页面
        targetPage = PAGE_HOME;
        printf("[DisplayManager] 端口自动切换结束，总功率%.1fW ≤ 2W，切换到待机页面\n", 
               m_powerData.valid ? m_powerData.total_power / 1000.0f : 0.0f);
    }
    
    // 执行页面切换
    switchPage(targetPage);
    
    printf("[DisplayManager] 已请求恢复到页面%d\n", m_previousPage);
}

/**
 * @brief 根据当前主题切换到对应的端口屏幕
 */
void DisplayManager::switchToPortScreen(int port_index) {
    if (port_index < 0 || port_index >= 4) {
        return;
    }
    
    // 使用普通切换方法，不标记为手动切换（因为这是自动切换触发的）
    DisplayPage targetPage = (DisplayPage)(PAGE_POWER_PORT1 + port_index);
    switchPage(targetPage);
    
    printf("[DisplayManager] 自动切换到端口%d屏幕\n", port_index + 1);
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

// === 亮度渐变功能实现 ===

/**
 * @brief 启用或禁用亮度渐变功能
 */
void DisplayManager::setFadingEnabled(bool enabled, uint32_t fadeDurationMs) {
    m_fadingEnabled = enabled;
    m_fadeDuration = fadeDurationMs;
    
    // 如果禁用渐变且当前正在渐变，停止渐变
    if (!enabled && m_isFading) {
        stopFading();
    }
    
    printf("[DisplayManager] 亮度渐变功能已%s，渐变时长: %d毫秒\n", 
           enabled ? "启用" : "禁用", fadeDurationMs);
}

/**
 * @brief 获取亮度渐变功能状态
 */
bool DisplayManager::isFadingEnabled() const {
    return m_fadingEnabled;
}

/**
 * @brief 检查是否正在执行亮度渐变
 */
bool DisplayManager::isFading() const {
    return m_isFading;
}

/**
 * @brief 设置渐变持续时间
 */
void DisplayManager::setFadeDuration(uint32_t fadeDurationMs) {
    m_fadeDuration = fadeDurationMs;
    printf("[DisplayManager] 渐变持续时间已设置为: %d毫秒\n", fadeDurationMs);
}

/**
 * @brief 获取当前渐变亮度值
 */
uint8_t DisplayManager::getCurrentFadingBrightness() const {
    return m_currentFadingBrightness;
}

/**
 * @brief 启动亮度渐变
 */
void DisplayManager::startFading(uint8_t targetBrightness, FadeDirection direction) {
    // 如果渐变功能未启用，直接设置目标亮度
    if (!m_fadingEnabled) {
        if (direction == FADE_TO_ON) {
            performScreenOnImmediate();
        } else {
            performScreenOffImmediate();
        }
        return;
    }
    
    // 停止当前渐变（如果正在进行）
    if (m_isFading) {
        stopFading();
    }
    
    // 获取当前实际亮度作为起始亮度
    if (direction == FADE_TO_ON) {
        // 开启时从0开始渐变
        m_currentFadingBrightness = 0;
        if (m_lvglDriver) {
            m_lvglDriver->setBrightness(0);
        }
        // 立即标记屏幕为开启状态，但亮度从0开始
        m_screenOn = true;
    } else {
        // 关闭时从当前亮度开始渐变
        m_currentFadingBrightness = m_brightness;
    }
    
    // 设置渐变参数
    m_targetFadingBrightness = targetBrightness;
    m_fadeDirection = direction;
    m_fadeStartTime = xTaskGetTickCount() * portTICK_PERIOD_MS;
    m_isFading = true;
    
    printf("[DisplayManager] 开始亮度渐变: %d%% -> %d%%, 方向: %s, 持续时间: %d毫秒\n",
           m_currentFadingBrightness, 
           m_targetFadingBrightness,
           direction == FADE_TO_ON ? "亮起" : "变暗",
           m_fadeDuration);
}

/**
 * @brief 处理亮度渐变逻辑
 */
void DisplayManager::processFading() {
    if (!m_isFading || !m_fadingEnabled) {
        return;
    }
    
    uint32_t currentTime = xTaskGetTickCount() * portTICK_PERIOD_MS;
    uint32_t elapsedTime = currentTime - m_fadeStartTime;
    
    // 检查渐变是否完成
    if (elapsedTime >= m_fadeDuration) {
        // 渐变完成，设置最终亮度
        m_currentFadingBrightness = m_targetFadingBrightness;
        
        if (m_lvglDriver) {
            m_lvglDriver->setBrightness(m_currentFadingBrightness);
        }
        
        // 更新屏幕状态
        if (m_fadeDirection == FADE_TO_OFF) {
            m_screenOn = false;
            printf("[DisplayManager] 渐变关闭完成，屏幕背光已关闭，触摸系统保持活跃状态\n");
        } else {
            m_screenOn = true;
            printf("[DisplayManager] 渐变开启完成，屏幕亮度已恢复至 %d%%\n", m_currentFadingBrightness);
        }
        
        // 停止渐变
        m_isFading = false;
        return;
    }
    
    // 计算渐变进度 (0.0 到 1.0)
    float linearProgress = (float)elapsedTime / (float)m_fadeDuration;
    
    // 使用更平滑的缓动曲线 - 组合多种缓动函数
    // 1. 三次贝塞尔曲线：3t²-2t³
    float cubicProgress = linearProgress * linearProgress * (3.0f - 2.0f * linearProgress);
    
    // 2. 正弦波缓动：sin(t*π/2) 用于更自然的变化
    float sineProgress = sin(linearProgress * M_PI * 0.5f);
    
    // 3. 组合缓动：前半段使用正弦波，后半段使用三次贝塞尔，过渡更自然
    float smoothProgress;
    if (linearProgress < 0.5f) {
        // 前半段：使用修正的正弦波缓动，起步更平缓
        float t = linearProgress * 2.0f;
        smoothProgress = 0.5f * (1.0f - cos(t * M_PI * 0.5f));
    } else {
        // 后半段：使用修正的三次贝塞尔曲线，结束更平缓
        float t = (linearProgress - 0.5f) * 2.0f;
        float cubicPart = t * t * (3.0f - 2.0f * t);
        smoothProgress = 0.5f + 0.5f * cubicPart;
    }
    
    // 进一步平滑处理：使用五次多项式进行最终平滑
    // f(t) = 6t⁵ - 15t⁴ + 10t³ （Ken Perlin's smoothstep的扩展版本）
    smoothProgress = smoothProgress * smoothProgress * smoothProgress * 
                    (smoothProgress * (smoothProgress * 6.0f - 15.0f) + 10.0f);
    
    uint8_t startBrightness, endBrightness;
    if (m_fadeDirection == FADE_TO_ON) {
        startBrightness = 0;
        endBrightness = m_targetFadingBrightness;
    } else {
        startBrightness = m_brightness; // 从设定亮度开始
        endBrightness = 0;
    }
    
    // 计算亮度范围和总步数
    int brightnessRange = abs(endBrightness - startBrightness);
    
    // 根据亮度范围和渐变时间计算理想步数，确保足够平滑
    int idealSteps = (brightnessRange * m_fadeDuration) / 100; // 每100ms每1%亮度一步
    if (idealSteps < brightnessRange * 2) idealSteps = brightnessRange * 2; // 最少每0.5%一步
    if (idealSteps > brightnessRange * 10) idealSteps = brightnessRange * 10; // 最多每0.1%一步
    
    // 使用更精确的浮点计算，支持小数点后一位精度
    float currentBrightnessFloat = (float)startBrightness + 
                                  ((float)(endBrightness - startBrightness) * smoothProgress);
    
    // 将亮度值精确到0.1%，然后四舍五入到整数
    float precisionBrightness = roundf(currentBrightnessFloat * 10.0f) / 10.0f;
    uint8_t newBrightness = (uint8_t)(precisionBrightness + 0.5f);
    
    // 确保亮度值在有效范围内
    if (newBrightness > 100) newBrightness = 100;
    
    // 对于平滑渐变，允许每一个亮度单位的变化
    // 在渐变过程中不设置变化阈值，让每个计算出的值都能更新
    bool shouldUpdate = false;
    
    // 如果亮度值发生任何变化就更新（提高平滑度）
    if (newBrightness != m_currentFadingBrightness) {
        shouldUpdate = true;
    }
    
    // 在渐变接近完成时强制更新，确保到达目标值
    if (elapsedTime >= m_fadeDuration - 50) {
        shouldUpdate = true;
    }
    
    // 更新硬件亮度
    if (shouldUpdate) {
        m_currentFadingBrightness = newBrightness;
        
        if (m_lvglDriver) {
            m_lvglDriver->setBrightness(m_currentFadingBrightness);
        }
        
        // 可选：输出详细渐变进度（调试用，正常使用时可注释掉）
        // printf("[DisplayManager] 渐变进度: %.1f%%, 平滑进度: %.2f, 当前亮度: %d%%, 精确值: %.1f%%\n", 
        //        linearProgress * 100, smoothProgress, m_currentFadingBrightness, precisionBrightness);
    }
}

/**
 * @brief 停止当前的亮度渐变
 */
void DisplayManager::stopFading() {
    if (!m_isFading) {
        return;
    }
    
    printf("[DisplayManager] 停止亮度渐变\n");
    m_isFading = false;
    
    // 设置为目标亮度（完成渐变）
    m_currentFadingBrightness = m_targetFadingBrightness;
    
    if (m_lvglDriver) {
        m_lvglDriver->setBrightness(m_currentFadingBrightness);
    }
    
    // 更新屏幕状态
    if (m_fadeDirection == FADE_TO_OFF) {
        m_screenOn = false;
    } else {
        m_screenOn = true;
    }
}

/**
 * @brief 执行即时屏幕开启操作（不使用渐变）
 */
void DisplayManager::performScreenOnImmediate() {
    if (m_screenOn) {
        return; // 已经开启
    }
    
    printf("[DisplayManager] 即时开启屏幕（无渐变）\n");
    
    // 恢复屏幕亮度
    if (m_lvglDriver) {
        m_lvglDriver->setBrightness(m_brightness);
    }
    
    // 更新当前渐变亮度值
    m_currentFadingBrightness = m_brightness;
    
    // 显示屏幕内容
    if (m_lvglDriver && m_lvglDriver->lock(1000)) {
        // 可以在这里添加屏幕开启时的UI更新逻辑
        // 例如：显示待机屏幕或恢复上次的屏幕内容
        m_lvglDriver->unlock();
    }
    
    m_screenOn = true;
    printf("[DisplayManager] 屏幕已开启，亮度已恢复至 %d%%\n", m_brightness);
}

/**
 * @brief 执行即时屏幕关闭操作（不使用渐变）
 */
void DisplayManager::performScreenOffImmediate() {
    if (!m_screenOn) {
        return; // 已经关闭
    }
    
    printf("[DisplayManager] 即时关闭屏幕（无渐变）\n");
    
    // 设置屏幕亮度为0（关闭背光）
    if (m_lvglDriver) {
        m_lvglDriver->setBrightness(0);
    }
    
    // 更新当前渐变亮度值
    m_currentFadingBrightness = 0;
    
    // 显示屏幕内容保持不变，只是关闭背光
    // LVGL任务继续运行，触摸检测继续工作
    if (m_lvglDriver && m_lvglDriver->lock(1000)) {
        // 不对UI内容进行任何修改，保持当前界面
        m_lvglDriver->unlock();
    } else {
        printf("[DisplayManager] 警告：LVGL锁获取失败，可能影响触摸响应\n");
    }
    
    m_screenOn = false;
    printf("[DisplayManager] 屏幕背光已关闭，触摸系统保持活跃状态\n");
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
    
    // 更新最后切换时间，启动冷却保护
    uint32_t currentTime = xTaskGetTickCount() * portTICK_PERIOD_MS;
    m_lastPageSwitchTime = currentTime;
    
    // 清除待处理的页面变化
    if (m_isPageChangePending) {
        m_isPageChangePending = false;
        printf("[DisplayManager] 手动切换页面，清除待处理的页面变化\n");
    }
    
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
    
    // 如果当前是端口页面或在端口自动切换模式中，不执行基于功率的自动切换
    if (isPortPage(m_currentPage) || m_isInAutoSwitchMode) {
        // 清除待处理的页面变化
        if (m_isPageChangePending) {
            m_isPageChangePending = false;
            printf("[DisplayManager] 当前为端口页面或端口自动切换模式，清除待处理的页面变化\n");
        }
        return;
    }
    
    // 检查是否应该执行自动切换
    if (!shouldExecuteAutoSwitch()) {
        return;
    }
    
    int totalPower = m_powerData.total_power;
    
    // 根据当前页面和功率条件决定切换逻辑
    if (m_currentPage == PAGE_POWER_TOTAL && totalPower < 1000) {
        // 在总功率页面，总功率小于1W，需要5秒持续检查后切换到待机页面
        if (!m_isPageChangePending || m_pendingTargetPage != PAGE_HOME) {
            // 开始5秒持续检查
            m_isPageChangePending = true;
            m_pendingTargetPage = PAGE_HOME;
            m_pageChangeStartTime = currentTime;
            
            printf("[DisplayManager] 在总功率页面检测到低功率%.1fW < 1W，开始5秒持续时间检查\n", 
                   totalPower / 1000.0f);
        } else {
            // 检查是否持续超过5秒
            uint32_t changeDuration = currentTime - m_pageChangeStartTime;
            if (changeDuration >= 5000) {
                printf("[DisplayManager] 低功率持续 %.1f秒，切换到待机页面\n", 
                       changeDuration / 1000.0f);
                
                switchPage(PAGE_HOME);
                m_lastPageSwitchTime = currentTime;
                m_isPageChangePending = false;
            }
        }
    } else if (m_currentPage == PAGE_HOME && totalPower > 2000) {
        // 在待机页面，总功率大于2W，检查是否有端口功率减少（避免在用户拔掉设备时切换）
        bool hasPortPowerDecrease = checkPortPowerDecrease();
        
        if (!hasPortPowerDecrease) {
            // 没有端口功率减少，立即切换到总功率页面
            printf("[DisplayManager] 在待机页面检测到高功率%.1fW > 2W且无端口功率减少，立即切换到总功率页面\n", 
                   totalPower / 1000.0f);
            
            switchPage(PAGE_POWER_TOTAL);
            m_lastPageSwitchTime = currentTime;
            
            // 清除可能存在的待处理状态
            if (m_isPageChangePending) {
                m_isPageChangePending = false;
            }
        } else {
            printf("[DisplayManager] 检测到端口功率减少，跳过切换到总功率页面（可能是用户拔掉设备）\n");
        }
    } else {
        // 其他情况，清除待处理的页面变化
        if (m_isPageChangePending) {
            m_isPageChangePending = false;
            printf("[DisplayManager] 功率条件不满足切换要求，取消页面变化 (功率%.1fW)\n", 
                   totalPower / 1000.0f);
        }
    }
}

/**
 * @brief 检查是否有端口功率减少（用于判断是否用户拔掉了设备）
 */
bool DisplayManager::checkPortPowerDecrease() const {
    if (!m_powerData.valid) {
        return false;
    }
    
    for (int i = 0; i < 4; i++) {
        if (m_powerData.ports[i].valid) {
            int currentPower = m_powerData.ports[i].power;
            int previousPower = m_previousPortPower[i];
            
            // 如果当前功率比之前功率显著减少（减少超过5W），认为是功率减少
            if (previousPower > 0 && currentPower < previousPower && 
                (previousPower - currentPower) > 5000) {
                printf("[DisplayManager] 检测到端口%d功率减少：%.1fW -> %.1fW\n", 
                       i + 1, previousPower / 1000.0f, currentPower / 1000.0f);
                return true;
            }
        }
    }
    
    return false;
}

// === OTA升级进度显示功能实现 ===

/**
 * @brief 开始OTA升级显示
 */
void DisplayManager::startOTADisplay(bool isServerOTA) {
    // 设置OTA进行中标志，防止屏幕被其他流程熄灭
    m_otaInProgress = true;
    
    // 强制唤醒屏幕以显示OTA进度
    printf("[DisplayManager] OTA升级开始，强制唤醒屏幕并保持常亮\n");
    forceScreenOn();
    
    // 发送OTA开始消息
    DisplayMessage msg;
    msg.type = DisplayMessage::MSG_OTA_START;
    msg.data.ota_status.isServerOTA = isServerOTA;
    
    if (m_messageQueue && xQueueSend(m_messageQueue, &msg, pdMS_TO_TICKS(100)) != pdTRUE) {
        printf("[DisplayManager] 警告：发送OTA开始消息失败\n");
    }
}

/**
 * @brief 更新OTA升级状态
 */
void DisplayManager::updateOTAStatus(int status, float progress, uint32_t totalSize, uint32_t writtenSize, 
                                    const char* statusText, const char* errorMessage) {
    DisplayMessage msg;
    msg.type = DisplayMessage::MSG_UPDATE_OTA_STATUS;
    msg.data.ota_status.status = status;
    msg.data.ota_status.progress = progress;
    msg.data.ota_status.totalSize = totalSize;
    msg.data.ota_status.writtenSize = writtenSize;
    
    // 复制状态文本（确保不超出缓冲区）
    strncpy(msg.data.ota_status.statusText, statusText ? statusText : "", sizeof(msg.data.ota_status.statusText) - 1);
    msg.data.ota_status.statusText[sizeof(msg.data.ota_status.statusText) - 1] = '\0';
    
    // 复制错误信息（确保不超出缓冲区）
    strncpy(msg.data.ota_status.errorMessage, errorMessage ? errorMessage : "", sizeof(msg.data.ota_status.errorMessage) - 1);
    msg.data.ota_status.errorMessage[sizeof(msg.data.ota_status.errorMessage) - 1] = '\0';
    
    if (m_messageQueue && xQueueSend(m_messageQueue, &msg, pdMS_TO_TICKS(100)) != pdTRUE) {
        printf("[DisplayManager] 警告：发送OTA状态更新消息失败\n");
    }
}

/**
 * @brief 完成OTA升级显示
 */
void DisplayManager::completeOTADisplay(bool success, const char* message) {
    // 清除OTA进行中标志，恢复正常屏幕控制
    m_otaInProgress = false;
    printf("[DisplayManager] OTA升级完成，恢复正常屏幕控制模式\n");
    
    DisplayMessage msg;
    msg.type = DisplayMessage::MSG_OTA_COMPLETE;
    msg.data.ota_status.status = success ? 4 : 5;  // 4=success, 5=failed
    
    // 复制完成消息
    strncpy(msg.data.ota_status.statusText, message ? message : "", sizeof(msg.data.ota_status.statusText) - 1);
    msg.data.ota_status.statusText[sizeof(msg.data.ota_status.statusText) - 1] = '\0';
    
    if (m_messageQueue && xQueueSend(m_messageQueue, &msg, pdMS_TO_TICKS(100)) != pdTRUE) {
        printf("[DisplayManager] 警告：发送OTA完成消息失败\n");
    }
}

/**
 * @brief 创建OTA进度显示页面
 */
void DisplayManager::createOTAProgressPage() {
    if (m_otaScreen) {
        return;  // 已经创建
    }
    
    // 创建OTA进度屏幕
    m_otaScreen = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(m_otaScreen, lv_color_hex(0x000000), LV_PART_MAIN);  // 黑色背景
    lv_obj_set_style_pad_all(m_otaScreen, 20, LV_PART_MAIN);  // 设置内边距
    
    // 创建主容器，使用flex布局
    lv_obj_t* mainContainer = lv_obj_create(m_otaScreen);
    lv_obj_set_size(mainContainer, lv_pct(100), lv_pct(100));
    lv_obj_set_style_bg_opa(mainContainer, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(mainContainer, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(mainContainer, 0, LV_PART_MAIN);
    lv_obj_set_flex_flow(mainContainer, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(mainContainer, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_center(mainContainer);
    
    // 创建主标题
    lv_obj_t* titleLabel = lv_label_create(mainContainer);
    lv_label_set_text(titleLabel, "Firmware Update");
    lv_obj_set_style_text_font(titleLabel, &lv_font_montserrat_24, LV_PART_MAIN);
    lv_obj_set_style_text_color(titleLabel, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
    lv_obj_set_style_text_align(titleLabel, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_set_style_pad_bottom(titleLabel, 30, LV_PART_MAIN);
    
    // 创建状态标签
    m_otaStatusLabel = lv_label_create(mainContainer);
    lv_label_set_text(m_otaStatusLabel, "Initializing...");
    lv_obj_set_style_text_font(m_otaStatusLabel, &lv_font_montserrat_18, LV_PART_MAIN);
    lv_obj_set_style_text_color(m_otaStatusLabel, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
    lv_obj_set_style_text_align(m_otaStatusLabel, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_set_style_pad_bottom(m_otaStatusLabel, 20, LV_PART_MAIN);
    
    // 创建进度条容器
    lv_obj_t* progressContainer = lv_obj_create(mainContainer);
    lv_obj_set_size(progressContainer, lv_pct(85), LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(progressContainer, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(progressContainer, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(progressContainer, 10, LV_PART_MAIN);
    lv_obj_set_flex_flow(progressContainer, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(progressContainer, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_bottom(progressContainer, 20, LV_PART_MAIN);
    
    // 创建进度条
    m_otaProgressBar = lv_bar_create(progressContainer);
    lv_obj_set_size(m_otaProgressBar, lv_pct(100), LV_SIZE_CONTENT);
    lv_obj_set_height(m_otaProgressBar, 25);  // 设置固定高度
    lv_obj_set_style_bg_color(m_otaProgressBar, lv_color_hex(0x333333), LV_PART_MAIN);
    lv_obj_set_style_bg_color(m_otaProgressBar, lv_color_hex(0x00DD00), LV_PART_INDICATOR);
    lv_obj_set_style_radius(m_otaProgressBar, 12, LV_PART_MAIN);
    lv_obj_set_style_radius(m_otaProgressBar, 10, LV_PART_INDICATOR);
    lv_obj_set_style_pad_bottom(m_otaProgressBar, 15, LV_PART_MAIN);
    lv_obj_set_style_min_width(m_otaProgressBar, 200, LV_PART_MAIN);  // 设置最小宽度
    lv_obj_set_style_max_width(m_otaProgressBar, 400, LV_PART_MAIN);  // 设置最大宽度
    lv_bar_set_range(m_otaProgressBar, 0, 100);
    lv_bar_set_value(m_otaProgressBar, 0, LV_ANIM_OFF);
    
    // 创建进度百分比标签
    m_otaProgressLabel = lv_label_create(progressContainer);
    lv_label_set_text(m_otaProgressLabel, "0%");
    lv_obj_set_style_text_font(m_otaProgressLabel, &lv_font_montserrat_20, LV_PART_MAIN);
    lv_obj_set_style_text_color(m_otaProgressLabel, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
    lv_obj_set_style_text_align(m_otaProgressLabel, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_set_style_pad_bottom(m_otaProgressLabel, 10, LV_PART_MAIN);
    
    // 创建文件大小标签
    m_otaSizeLabel = lv_label_create(progressContainer);
    lv_label_set_text(m_otaSizeLabel, "0 / 0 bytes");
    lv_obj_set_style_text_font(m_otaSizeLabel, &lv_font_montserrat_14, LV_PART_MAIN);
    lv_obj_set_style_text_color(m_otaSizeLabel, lv_color_hex(0xAAAAAA), LV_PART_MAIN);
    lv_obj_set_style_text_align(m_otaSizeLabel, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    
    // 创建错误信息标签（初始隐藏）
    m_otaErrorLabel = lv_label_create(mainContainer);
    lv_label_set_text(m_otaErrorLabel, "");
    lv_obj_set_style_text_font(m_otaErrorLabel, &lv_font_montserrat_14, LV_PART_MAIN);
    lv_obj_set_style_text_color(m_otaErrorLabel, lv_color_hex(0xFF4444), LV_PART_MAIN);
    lv_obj_set_style_text_align(m_otaErrorLabel, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_set_width(m_otaErrorLabel, lv_pct(90));
    lv_label_set_long_mode(m_otaErrorLabel, LV_LABEL_LONG_WRAP);
    lv_obj_add_flag(m_otaErrorLabel, LV_OBJ_FLAG_HIDDEN);  // 初始隐藏
    
    // 切换到OTA进度屏幕
    lv_scr_load_anim(m_otaScreen, LV_SCR_LOAD_ANIM_MOVE_LEFT, 300, 0, false);
    
    printf("[DisplayManager] OTA progress page created with responsive layout\n");
}

/**
 * @brief 销毁OTA进度显示页面
 */
void DisplayManager::destroyOTAProgressPage() {
    if (m_otaScreen) {
        lv_obj_del(m_otaScreen);
        m_otaScreen = nullptr;
        m_otaProgressBar = nullptr;
        m_otaStatusLabel = nullptr;
        m_otaProgressLabel = nullptr;
        m_otaSizeLabel = nullptr;
        m_otaErrorLabel = nullptr;
        printf("[DisplayManager] OTA progress page destroyed\n");
    }
}

/**
 * @brief 更新OTA进度条显示
 */
void DisplayManager::updateOTAProgressBar(float progress) {
    if (m_otaProgressBar) {
        // 确保进度在有效范围内
        int progressInt = (int)progress;
        if (progressInt < 0) progressInt = 0;
        if (progressInt > 100) progressInt = 100;
        
        lv_bar_set_value(m_otaProgressBar, progressInt, LV_ANIM_ON);
    }
    
    if (m_otaProgressLabel) {
        char progressText[16];
        snprintf(progressText, sizeof(progressText), "%.1f%%", progress);
        lv_label_set_text(m_otaProgressLabel, progressText);
    }
}

/**
 * @brief 更新OTA状态文本显示
 */
void DisplayManager::updateOTAStatusText(const char* statusText) {
    if (m_otaStatusLabel && statusText) {
        lv_label_set_text(m_otaStatusLabel, statusText);
    }
}

/**
 * @brief 更新OTA大小信息显示
 */
void DisplayManager::updateOTASizeInfo(uint32_t totalSize, uint32_t writtenSize) {
    if (m_otaSizeLabel) {
        char sizeText[64];
        
        if (totalSize > 0) {
            // 格式化大小信息
            if (totalSize >= 1024 * 1024) {
                // MB格式
                float totalMB = (float)totalSize / (1024.0f * 1024.0f);
                float writtenMB = (float)writtenSize / (1024.0f * 1024.0f);
                snprintf(sizeText, sizeof(sizeText), "%.2f / %.2f MB", writtenMB, totalMB);
            } else if (totalSize >= 1024) {
                // KB格式
                float totalKB = (float)totalSize / 1024.0f;
                float writtenKB = (float)writtenSize / 1024.0f;
                snprintf(sizeText, sizeof(sizeText), "%.1f / %.1f KB", writtenKB, totalKB);
            } else {
                // 字节格式
                snprintf(sizeText, sizeof(sizeText), "%u / %u bytes", writtenSize, totalSize);
            }
        } else {
            // 动态大小模式
            if (writtenSize >= 1024 * 1024) {
                float writtenMB = (float)writtenSize / (1024.0f * 1024.0f);
                snprintf(sizeText, sizeof(sizeText), "%.2f MB (dynamic)", writtenMB);
            } else if (writtenSize >= 1024) {
                float writtenKB = (float)writtenSize / 1024.0f;
                snprintf(sizeText, sizeof(sizeText), "%.1f KB (dynamic)", writtenKB);
            } else {
                snprintf(sizeText, sizeof(sizeText), "%u bytes (dynamic)", writtenSize);
            }
        }
        
        lv_label_set_text(m_otaSizeLabel, sizeText);
    }
}

 /**
  * @brief 显示OTA错误信息
  */
 void DisplayManager::showOTAError(const char* errorMessage) {
     if (m_otaErrorLabel && errorMessage && strlen(errorMessage) > 0) {
         lv_label_set_text(m_otaErrorLabel, errorMessage);
         lv_obj_clear_flag(m_otaErrorLabel, LV_OBJ_FLAG_HIDDEN);  // 显示错误标签
     }
 }
 
 /**
  * @brief 重新布局OTA进度页面（适应屏幕旋转）
  */
 void DisplayManager::relayoutOTAProgressPage() {
     if (!m_otaScreen || !m_otaDisplayActive) {
         return;  // OTA页面未激活
     }
     
     // 获取当前屏幕尺寸
     lv_coord_t screen_width = lv_obj_get_width(lv_scr_act());
     lv_coord_t screen_height = lv_obj_get_height(lv_scr_act());
     
     printf("[DisplayManager] Screen orientation changed: %dx%d, relayouting OTA page\n", 
            screen_width, screen_height);
     
     // 由于使用了flex布局和百分比尺寸，LVGL会自动重新布局
     // 这里我们可以做一些额外的调整，比如根据屏幕方向调整字体大小
     
     if (screen_width > screen_height) {
         // 横屏模式 - 可以使用较大的字体和进度条
         if (m_otaProgressBar) {
             lv_obj_set_height(m_otaProgressBar, 30);  // 更厚的进度条
         }
     } else {
         // 竖屏模式 - 使用标准尺寸
         if (m_otaProgressBar) {
             lv_obj_set_height(m_otaProgressBar, 25);  // 标准进度条
         }
     }
     
     // 强制重新计算布局
     lv_obj_invalidate(m_otaScreen);
     lv_refr_now(NULL);
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

/**
 * @brief 获取LVGL驱动实例
 */
LVGLDriver* DisplayManager::getLVGLDriver() const {
    return m_lvglDriver;
}