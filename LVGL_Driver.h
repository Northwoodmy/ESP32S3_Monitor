#ifndef LVGL_DRIVER_H
#define LVGL_DRIVER_H

#include <lvgl.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// ====================================================================
// LVGL驱动类声明
// ====================================================================
class LVGLDriver {
public:
    // 构造函数和析构函数
    LVGLDriver();
    ~LVGLDriver();
    
    // 初始化和运行函数
    void init();
    void loop();
    void start();
    void stop();
    
    // 显示功能函数
    void createSimpleButton();
    void updateButtonText(const char* text);
    
    // 系统控制函数
    void setBrightness(int brightness);
    int getBrightness();
    
    // 状态获取函数
    bool isBrightnessChanged();
    void clearBrightnessFlag();
    bool isRunning();
    
private:
    // 内部变量
    int brightness;
    bool brightnessChanged;
    lv_obj_t* buttonObj;
    lv_obj_t* buttonLabel;
    
    // FreeRTOS任务相关
    TaskHandle_t lvglTaskHandle;
    bool taskRunning;
    
    // 静态回调函数
    static void buttonEventCallback(lv_event_t* e);
    static void lvglTask(void* parameter);
};

// ====================================================================
// 全局函数声明（兼容C风格调用）
// ====================================================================
extern "C" {
    void Lvgl_Init(void);
    void Lvgl_Loop(void);
    
    // LVGL核心功能函数
    void Pure_LVGL_Init(void);
    void Pure_LVGL_Loop(void);
    void Create_Test_UI(void);
}

// ====================================================================
// 全局变量声明
// ====================================================================
extern int brightness_flag;
extern LVGLDriver* lvglDriver;

#endif // LVGL_DRIVER_H 