/*
 * SH8601 LCD控制器驱动程序
 * 
 * 版权所有: 2023 Espressif Systems (Shanghai) CO LTD
 * 许可证: Apache-2.0
 * 
 * 功能描述：
 * - 实现了SH8601 LCD控制器的完整驱动功能
 * - 支持SPI和QSPI两种接口模式
 * - 提供标准的LCD面板操作接口
 * - 支持多种颜色格式(RGB565/RGB666/RGB888)
 * - 实现硬件和软件复位功能
 * - 支持显示区域设置和位图绘制
 */

#include <stdlib.h>
#include <sys/cdefs.h>

// === FreeRTOS系统头文件 ===
#include "freertos/FreeRTOS.h"    // FreeRTOS实时操作系统
#include "freertos/task.h"        // 任务管理和延时函数

// === ESP32硬件驱动头文件 ===
#include "driver/gpio.h"          // GPIO控制
#include "esp_check.h"            // ESP错误检查宏
#include "esp_lcd_panel_interface.h" // LCD面板接口定义
#include "esp_lcd_panel_io.h"     // LCD面板IO操作
#include "esp_lcd_panel_vendor.h" // LCD厂商特定功能
#include "esp_lcd_panel_ops.h"    // LCD面板操作
#include "esp_lcd_panel_commands.h" // LCD标准命令定义
#include "esp_log.h"              // ESP日志系统

#include "esp_lcd_sh8601.h"       // SH8601控制器头文件

// === QSPI操作码定义 ===
#define LCD_OPCODE_WRITE_CMD        (0x02ULL)  // QSPI写命令操作码
#define LCD_OPCODE_READ_CMD         (0x03ULL)  // QSPI读命令操作码  
#define LCD_OPCODE_WRITE_COLOR      (0x32ULL)  // QSPI写颜色数据操作码

static const char *TAG = "sh8601";  // 日志标签

// === 函数声明 ===
static esp_err_t panel_sh8601_del(esp_lcd_panel_t *panel);
static esp_err_t panel_sh8601_reset(esp_lcd_panel_t *panel);
static esp_err_t panel_sh8601_init(esp_lcd_panel_t *panel);
static esp_err_t panel_sh8601_draw_bitmap(esp_lcd_panel_t *panel, int x_start, int y_start, int x_end, int y_end, const void *color_data);
static esp_err_t panel_sh8601_invert_color(esp_lcd_panel_t *panel, bool invert_color_data);
static esp_err_t panel_sh8601_mirror(esp_lcd_panel_t *panel, bool mirror_x, bool mirror_y);
static esp_err_t panel_sh8601_swap_xy(esp_lcd_panel_t *panel, bool swap_axes);
static esp_err_t panel_sh8601_set_gap(esp_lcd_panel_t *panel, int x_gap, int y_gap);
static esp_err_t panel_sh8601_disp_on_off(esp_lcd_panel_t *panel, bool off);

/**
 * @brief SH8601面板私有数据结构
 * 
 * 包含了SH8601控制器的所有配置信息和状态数据
 */
typedef struct {
    esp_lcd_panel_t base;                    // 基础LCD面板结构
    esp_lcd_panel_io_handle_t io;            // LCD面板IO句柄
    int reset_gpio_num;                      // 复位GPIO引脚号
    int x_gap;                               // X轴偏移量
    int y_gap;                               // Y轴偏移量
    uint8_t fb_bits_per_pixel;               // 帧缓冲区每像素位数
    uint8_t madctl_val;                      // MADCTL寄存器当前值（内存访问控制）
    uint8_t colmod_val;                      // COLMOD寄存器当前值（像素格式）
    const sh8601_lcd_init_cmd_t *init_cmds;  // 初始化命令数组指针
    uint16_t init_cmds_size;                 // 初始化命令数组大小
    struct {
        unsigned int use_qspi_interface: 1;  // 是否使用QSPI接口
        unsigned int reset_level: 1;         // 复位信号电平
    } flags;                                 // 标志位结构
} sh8601_panel_t;

/**
 * @brief 创建SH8601 LCD面板实例
 * 
 * 这是SH8601驱动的主要入口函数，负责初始化面板数据结构，
 * 配置GPIO，设置颜色格式，并注册所有的操作回调函数。
 * 
 * @param io LCD面板IO句柄
 * @param panel_dev_config 面板设备配置结构
 * @param ret_panel 返回的面板句柄指针
 * @return ESP_OK 成功，其他值表示错误
 */
esp_err_t esp_lcd_new_panel_sh8601(const esp_lcd_panel_io_handle_t io, const esp_lcd_panel_dev_config_t *panel_dev_config, esp_lcd_panel_handle_t *ret_panel)
{
    ESP_RETURN_ON_FALSE(io && panel_dev_config && ret_panel, ESP_ERR_INVALID_ARG, TAG, "参数无效");

    esp_err_t ret = ESP_OK;
    sh8601_panel_t *sh8601 = NULL;
    
    // 分配SH8601面板数据结构内存
    sh8601 = calloc(1, sizeof(sh8601_panel_t));
    ESP_GOTO_ON_FALSE(sh8601, ESP_ERR_NO_MEM, err, TAG, "SH8601面板内存分配失败");

    // === 配置复位GPIO引脚 ===
    if (panel_dev_config->reset_gpio_num >= 0) {
        gpio_config_t io_conf = {
            .mode = GPIO_MODE_OUTPUT,
            .pin_bit_mask = 1ULL << panel_dev_config->reset_gpio_num,
        };
        ESP_GOTO_ON_ERROR(gpio_config(&io_conf), err, TAG, "复位引脚GPIO配置失败");
    }

    // === 配置RGB元素顺序 ===
    switch (panel_dev_config->rgb_ele_order) {
    case LCD_RGB_ELEMENT_ORDER_RGB:
        sh8601->madctl_val = 0;  // RGB顺序
        break;
    case LCD_RGB_ELEMENT_ORDER_BGR:
        sh8601->madctl_val |= LCD_CMD_BGR_BIT;  // BGR顺序
        break;
    default:
        ESP_GOTO_ON_FALSE(false, ESP_ERR_NOT_SUPPORTED, err, TAG, "不支持的颜色元素顺序");
        break;
    }

    // === 配置像素位深和颜色格式 ===
    uint8_t fb_bits_per_pixel = 0;
    switch (panel_dev_config->bits_per_pixel) {
    case 16: // RGB565格式
        sh8601->colmod_val = 0x55;    // 16位色深设置
        fb_bits_per_pixel = 16;
        break;
    case 18: // RGB666格式
        sh8601->colmod_val = 0x66;    // 18位色深设置
        // 每个颜色分量(R/G/B)占用字节的高6位，需要3个完整字节表示一个像素
        fb_bits_per_pixel = 18;
        break;
    case 24: // RGB888格式
        sh8601->colmod_val = 0x77;    // 24位色深设置
        fb_bits_per_pixel = 24;
        break;
    default:
        ESP_GOTO_ON_FALSE(false, ESP_ERR_NOT_SUPPORTED, err, TAG, "不支持的像素位宽");
        break;
    }

    // === 初始化SH8601结构体成员 ===
    sh8601->io = io;
    sh8601->reset_gpio_num = panel_dev_config->reset_gpio_num;
    sh8601->fb_bits_per_pixel = fb_bits_per_pixel;
    
    // 处理厂商特定配置
    sh8601_vendor_config_t *vendor_config = (sh8601_vendor_config_t *)panel_dev_config->vendor_config;
    if (vendor_config) {
        sh8601->init_cmds = vendor_config->init_cmds;
        sh8601->init_cmds_size = vendor_config->init_cmds_size;
        sh8601->flags.use_qspi_interface = vendor_config->flags.use_qspi_interface;
    }
    sh8601->flags.reset_level = panel_dev_config->flags.reset_active_high;
    
    // === 注册LCD面板操作函数 ===
    sh8601->base.del = panel_sh8601_del;                    // 删除面板
    sh8601->base.reset = panel_sh8601_reset;                // 复位面板
    sh8601->base.init = panel_sh8601_init;                  // 初始化面板
    sh8601->base.draw_bitmap = panel_sh8601_draw_bitmap;    // 绘制位图
    sh8601->base.invert_color = panel_sh8601_invert_color;  // 颜色反转
    sh8601->base.set_gap = panel_sh8601_set_gap;            // 设置偏移量
    sh8601->base.mirror = panel_sh8601_mirror;              // 镜像显示
    sh8601->base.swap_xy = panel_sh8601_swap_xy;            // XY轴交换
    sh8601->base.disp_on_off = panel_sh8601_disp_on_off;    // 显示开关

    *ret_panel = &(sh8601->base);
    ESP_LOGD(TAG, "成功创建SH8601面板 @%p", sh8601);

    return ESP_OK;

err:
    // 错误处理：清理已分配的资源
    if (sh8601) {
        if (panel_dev_config->reset_gpio_num >= 0) {
            gpio_reset_pin(panel_dev_config->reset_gpio_num);
        }
        free(sh8601);
    }
    return ret;
}

/**
 * @brief 发送参数命令到LCD控制器
 * 
 * 根据接口类型（SPI或QSPI）格式化命令并发送参数数据
 * 
 * @param sh8601 SH8601面板结构指针
 * @param io LCD面板IO句柄
 * @param lcd_cmd LCD命令
 * @param param 参数数据指针
 * @param param_size 参数数据大小
 * @return ESP_OK 成功，其他值表示错误
 */
static esp_err_t tx_param(sh8601_panel_t *sh8601, esp_lcd_panel_io_handle_t io, int lcd_cmd, const void *param, size_t param_size)
{
    if (sh8601->flags.use_qspi_interface) {
        // QSPI模式：重新格式化命令字
        lcd_cmd &= 0xff;                           // 保留命令的低8位
        lcd_cmd <<= 8;                             // 命令左移8位
        lcd_cmd |= LCD_OPCODE_WRITE_CMD << 24;     // 添加QSPI写命令操作码
    }
    return esp_lcd_panel_io_tx_param(io, lcd_cmd, param, param_size);
}

/**
 * @brief 发送颜色数据到LCD控制器
 * 
 * 根据接口类型（SPI或QSPI）格式化命令并发送颜色数据
 * 
 * @param sh8601 SH8601面板结构指针
 * @param io LCD面板IO句柄
 * @param lcd_cmd LCD命令
 * @param param 颜色数据指针
 * @param param_size 颜色数据大小
 * @return ESP_OK 成功，其他值表示错误
 */
static esp_err_t tx_color(sh8601_panel_t *sh8601, esp_lcd_panel_io_handle_t io, int lcd_cmd, const void *param, size_t param_size)
{
    if (sh8601->flags.use_qspi_interface) {
        // QSPI模式：重新格式化命令字
        lcd_cmd &= 0xff;                           // 保留命令的低8位
        lcd_cmd <<= 8;                             // 命令左移8位
        lcd_cmd |= LCD_OPCODE_WRITE_COLOR << 24;   // 添加QSPI写颜色操作码
    }
    return esp_lcd_panel_io_tx_color(io, lcd_cmd, param, param_size);
}

/**
 * @brief 删除SH8601面板实例
 * 
 * 清理GPIO资源并释放内存
 * 
 * @param panel LCD面板句柄
 * @return ESP_OK 成功
 */
static esp_err_t panel_sh8601_del(esp_lcd_panel_t *panel)
{
    sh8601_panel_t *sh8601 = __containerof(panel, sh8601_panel_t, base);

    // 复位GPIO引脚状态
    if (sh8601->reset_gpio_num >= 0) {
        gpio_reset_pin(sh8601->reset_gpio_num);
    }
    ESP_LOGD(TAG, "删除SH8601面板 @%p", sh8601);
    free(sh8601);  // 释放内存
    return ESP_OK;
}

/**
 * @brief 复位SH8601 LCD面板
 * 
 * 执行硬件复位或软件复位操作。硬件复位通过GPIO引脚控制，
 * 软件复位通过发送SWRESET命令实现。
 * 
 * @param panel LCD面板句柄
 * @return ESP_OK 成功，其他值表示错误
 */
static esp_err_t panel_sh8601_reset(esp_lcd_panel_t *panel)
{
    sh8601_panel_t *sh8601 = __containerof(panel, sh8601_panel_t, base);
    esp_lcd_panel_io_handle_t io = sh8601->io;

    // 执行硬件复位
    if (sh8601->reset_gpio_num >= 0) {
        gpio_set_level(sh8601->reset_gpio_num, sh8601->flags.reset_level);    // 置复位信号为有效电平
        vTaskDelay(pdMS_TO_TICKS(10));                                        // 保持10ms
        gpio_set_level(sh8601->reset_gpio_num, !sh8601->flags.reset_level);   // 释放复位信号
        vTaskDelay(pdMS_TO_TICKS(150));                                       // 等待150ms复位完成
    } else { 
        // 执行软件复位
        ESP_RETURN_ON_ERROR(tx_param(sh8601, io, LCD_CMD_SWRESET, NULL, 0), TAG, "发送复位命令失败");
        vTaskDelay(pdMS_TO_TICKS(80));  // 等待80ms复位完成
    }

    return ESP_OK;
}

/**
 * @brief SH8601厂商特定默认初始化命令序列
 * 
 * 这些命令是SH8601控制器的基本配置命令，包括：
 * - 撕裂效果控制
 * - 显示控制模式
 */
static const sh8601_lcd_init_cmd_t vendor_specific_init_default[] = {
//  {命令, {数据}, 数据长度, 延迟时间(ms)}
    {0x44, (uint8_t []){0x01, 0xD1}, 2, 0},    // 设置撕裂效果扫描线
    {0x35, (uint8_t []){0x00}, 0, 0},          // 启用撕裂效果输出信号
    {0x53, (uint8_t []){0x20}, 1, 25},         // 显示控制模式设置
};

/**
 * @brief 初始化SH8601 LCD面板
 * 
 * 发送初始化命令序列到LCD控制器，包括：
 * 1. 设置内存访问控制（MADCTL）
 * 2. 设置像素格式（COLMOD）
 * 3. 执行厂商特定初始化命令
 * 
 * @param panel LCD面板句柄
 * @return ESP_OK 成功，其他值表示错误
 */
static esp_err_t panel_sh8601_init(esp_lcd_panel_t *panel)
{
    sh8601_panel_t *sh8601 = __containerof(panel, sh8601_panel_t, base);
    esp_lcd_panel_io_handle_t io = sh8601->io;
    const sh8601_lcd_init_cmd_t *init_cmds = NULL;
    uint16_t init_cmds_size = 0;
    bool is_cmd_overwritten = false;

    // === 1. 设置内存访问控制寄存器 ===
    ESP_RETURN_ON_ERROR(tx_param(sh8601, io, LCD_CMD_MADCTL, (uint8_t[]) {
        sh8601->madctl_val,  // 内存访问控制值（包含RGB顺序、扫描方向等）
    }, 1), TAG, "发送MADCTL命令失败");
    
    // === 2. 设置像素格式寄存器 ===
    ESP_RETURN_ON_ERROR(tx_param(sh8601, io, LCD_CMD_COLMOD, (uint8_t[]) {
        sh8601->colmod_val,  // 像素格式值（16/18/24位色深）
    }, 1), TAG, "发送COLMOD命令失败");

    // === 3. 执行厂商特定初始化命令 ===
    // 确定要使用的初始化命令序列
    if (sh8601->init_cmds) {
        // 使用用户提供的初始化命令
        init_cmds = sh8601->init_cmds;
        init_cmds_size = sh8601->init_cmds_size;
    } else {
        // 使用默认的厂商初始化命令
        init_cmds = vendor_specific_init_default;
        init_cmds_size = sizeof(vendor_specific_init_default) / sizeof(sh8601_lcd_init_cmd_t);
    }

    // 逐个执行初始化命令
    for (int i = 0; i < init_cmds_size; i++) {
        // 检查命令是否与内部命令冲突
        switch (init_cmds[i].cmd) {
        case LCD_CMD_MADCTL:
            is_cmd_overwritten = true;
            sh8601->madctl_val = ((uint8_t *)init_cmds[i].data)[0];  // 更新MADCTL值
            break;
        case LCD_CMD_COLMOD:
            is_cmd_overwritten = true;
            sh8601->colmod_val = ((uint8_t *)init_cmds[i].data)[0];  // 更新COLMOD值
            break;
        default:
            is_cmd_overwritten = false;
            break;
        }

        if (is_cmd_overwritten) {
            ESP_LOGW(TAG, "命令0x%02X已被使用，将被外部初始化序列覆盖", init_cmds[i].cmd);
        }

        // 发送初始化命令
        ESP_RETURN_ON_ERROR(tx_param(sh8601, io, init_cmds[i].cmd, init_cmds[i].data, init_cmds[i].data_bytes), TAG,
                            "发送初始化命令失败");
        vTaskDelay(pdMS_TO_TICKS(init_cmds[i].delay_ms));  // 按要求延迟
    }
    ESP_LOGD(TAG, "发送初始化命令成功");

    return ESP_OK;
}

/**
 * @brief 在LCD面板上绘制位图
 * 
 * 该函数将像素数据写入LCD的指定区域。步骤包括：
 * 1. 设置列地址范围（CASET命令）
 * 2. 设置行地址范围（RASET命令）
 * 3. 写入像素数据（RAMWR命令）
 * 
 * @param panel LCD面板句柄
 * @param x_start 起始X坐标
 * @param y_start 起始Y坐标
 * @param x_end 结束X坐标
 * @param y_end 结束Y坐标
 * @param color_data 颜色数据指针
 * @return ESP_OK 成功，其他值表示错误
 */
static esp_err_t panel_sh8601_draw_bitmap(esp_lcd_panel_t *panel, int x_start, int y_start, int x_end, int y_end, const void *color_data)
{
    sh8601_panel_t *sh8601 = __containerof(panel, sh8601_panel_t, base);
    assert((x_start < x_end) && (y_start < y_end) && "起始位置必须小于结束位置");
    esp_lcd_panel_io_handle_t io = sh8601->io;

    // 应用显示偏移量
    x_start += sh8601->x_gap;
    x_end += sh8601->x_gap;
    y_start += sh8601->y_gap;
    y_end += sh8601->y_gap;

    // === 1. 设置列地址范围（X坐标范围）===
    ESP_RETURN_ON_ERROR(tx_param(sh8601, io, LCD_CMD_CASET, (uint8_t[]) {
        (x_start >> 8) & 0xFF,    // X起始坐标高字节
        x_start & 0xFF,           // X起始坐标低字节
        ((x_end - 1) >> 8) & 0xFF, // X结束坐标高字节
        (x_end - 1) & 0xFF,       // X结束坐标低字节
    }, 4), TAG, "发送列地址设置命令失败");
    
    // === 2. 设置行地址范围（Y坐标范围）===
    ESP_RETURN_ON_ERROR(tx_param(sh8601, io, LCD_CMD_RASET, (uint8_t[]) {
        (y_start >> 8) & 0xFF,    // Y起始坐标高字节
        y_start & 0xFF,           // Y起始坐标低字节
        ((y_end - 1) >> 8) & 0xFF, // Y结束坐标高字节
        (y_end - 1) & 0xFF,       // Y结束坐标低字节
    }, 4), TAG, "发送行地址设置命令失败");
    
    // === 3. 传输帧缓冲区数据 ===
    size_t len = (x_end - x_start) * (y_end - y_start) * sh8601->fb_bits_per_pixel / 8;
    tx_color(sh8601, io, LCD_CMD_RAMWR, color_data, len);

    return ESP_OK;
}

/**
 * @brief 反转LCD面板颜色
 * 
 * 启用或禁用颜色反转功能。颜色反转时，白色变黑色，黑色变白色。
 * 
 * @param panel LCD面板句柄
 * @param invert_color_data true:启用颜色反转，false:禁用颜色反转
 * @return ESP_OK 成功，其他值表示错误
 */
static esp_err_t panel_sh8601_invert_color(esp_lcd_panel_t *panel, bool invert_color_data)
{
    sh8601_panel_t *sh8601 = __containerof(panel, sh8601_panel_t, base);
    esp_lcd_panel_io_handle_t io = sh8601->io;
    int command = 0;
    
    if (invert_color_data) {
        command = LCD_CMD_INVON;   // 启用颜色反转命令
    } else {
        command = LCD_CMD_INVOFF;  // 禁用颜色反转命令
    }
    ESP_RETURN_ON_ERROR(tx_param(sh8601, io, command, NULL, 0), TAG, "发送颜色反转命令失败");
    return ESP_OK;
}

/**
 * @brief 设置LCD面板镜像显示
 * 
 * 控制显示的镜像效果。注意：SH8601只支持X轴镜像，不支持Y轴镜像。
 * 
 * @param panel LCD面板句柄
 * @param mirror_x true:启用X轴镜像，false:禁用X轴镜像
 * @param mirror_y true:启用Y轴镜像，false:禁用Y轴镜像（不支持）
 * @return ESP_OK 成功，ESP_ERR_NOT_SUPPORTED Y轴镜像不支持
 */
static esp_err_t panel_sh8601_mirror(esp_lcd_panel_t *panel, bool mirror_x, bool mirror_y)
{
    sh8601_panel_t *sh8601 = __containerof(panel, sh8601_panel_t, base);
    esp_lcd_panel_io_handle_t io = sh8601->io;
    esp_err_t ret = ESP_OK;

    // 设置X轴镜像
    if (mirror_x) {
        sh8601->madctl_val |= BIT(6);  // 设置X轴镜像位
    } else {
        sh8601->madctl_val &= ~BIT(6); // 清除X轴镜像位
    }
    
    // 检查Y轴镜像（不支持）
    if (mirror_y) {
        ESP_LOGE(TAG, "该面板不支持Y轴镜像");
        ret = ESP_ERR_NOT_SUPPORTED;
    }
    
    // 更新MADCTL寄存器
    ESP_RETURN_ON_ERROR(tx_param(sh8601, io, LCD_CMD_MADCTL, (uint8_t[]) {
        sh8601->madctl_val  // 更新后的内存访问控制值
    }, 1), TAG, "发送MADCTL命令失败");
    return ret;
}

/**
 * @brief 交换LCD面板的XY轴
 * 
 * SH8601控制器不支持XY轴交换功能
 * 
 * @param panel LCD面板句柄
 * @param swap_axes true:交换XY轴，false:不交换XY轴
 * @return ESP_ERR_NOT_SUPPORTED 不支持XY轴交换
 */
static esp_err_t panel_sh8601_swap_xy(esp_lcd_panel_t *panel, bool swap_axes)
{
    ESP_LOGE(TAG, "该面板不支持XY轴交换");
    return ESP_ERR_NOT_SUPPORTED;
}

/**
 * @brief 设置LCD面板显示偏移量
 * 
 * 设置显示区域相对于面板原点的偏移量，用于适配不同的面板布局
 * 
 * @param panel LCD面板句柄
 * @param x_gap X轴偏移量（像素）
 * @param y_gap Y轴偏移量（像素）
 * @return ESP_OK 成功
 */
static esp_err_t panel_sh8601_set_gap(esp_lcd_panel_t *panel, int x_gap, int y_gap)
{
    sh8601_panel_t *sh8601 = __containerof(panel, sh8601_panel_t, base);
    sh8601->x_gap = x_gap;  // 保存X轴偏移量
    sh8601->y_gap = y_gap;  // 保存Y轴偏移量
    return ESP_OK;
}

/**
 * @brief 控制LCD面板显示开关
 * 
 * 启用或禁用LCD面板的显示输出
 * 
 * @param panel LCD面板句柄
 * @param on_off true:开启显示，false:关闭显示
 * @return ESP_OK 成功，其他值表示错误
 */
static esp_err_t panel_sh8601_disp_on_off(esp_lcd_panel_t *panel, bool on_off)
{
    sh8601_panel_t *sh8601 = __containerof(panel, sh8601_panel_t, base);
    esp_lcd_panel_io_handle_t io = sh8601->io;
    int command = 0;

    if (on_off) {
        command = LCD_CMD_DISPON;   // 显示开启命令
    } else {
        command = LCD_CMD_DISPOFF;  // 显示关闭命令
    }
    ESP_RETURN_ON_ERROR(tx_param(sh8601, io, command, NULL, 0), TAG, "发送显示控制命令失败");
    return ESP_OK;
}

/**
 * @brief 设置SH8601 LCD面板亮度
 * 
 * 通过发送0x51命令来控制SH8601 AMOLED显示屏的背光亮度
 * 
 * @param panel LCD面板句柄
 * @param brightness 亮度值 (0-255, 0为最暗，255为最亮)
 * @return ESP_OK 成功，其他值表示错误
 */
esp_err_t esp_lcd_sh8601_set_brightness(esp_lcd_panel_handle_t panel, uint8_t brightness)
{
    ESP_RETURN_ON_FALSE(panel, ESP_ERR_INVALID_ARG, TAG, "面板句柄无效");
    
    sh8601_panel_t *sh8601 = __containerof(panel, sh8601_panel_t, base);
    esp_lcd_panel_io_handle_t io = sh8601->io;
    
    // 发送亮度控制命令 0x51，参数为亮度值
    ESP_RETURN_ON_ERROR(tx_param(sh8601, io, 0x51, (uint8_t[]){brightness}, 1), TAG, "发送亮度控制命令失败");
    
    ESP_LOGD(TAG, "成功设置面板亮度为: %d", brightness);
    return ESP_OK;
}
