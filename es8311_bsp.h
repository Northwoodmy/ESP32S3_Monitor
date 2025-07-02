/*
 * SPDX-FileCopyrightText: 2015-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file es8311.h
 * @brief ES8311音频编解码器驱动程序头文件
 * @version 1.1.0
 * @date 2024-01-01
 * @author Espressif Systems & 系统管理员
 * 
 * @description
 * ES8311是一款高性能的立体声音频编解码器芯片，集成了ADC、DAC、
 * 前置放大器和耳机驱动器。本驱动程序提供了完整的API接口。
 * 
 * @features
 * - 支持8kHz到96kHz采样率
 * - 支持16/18/20/24/32位音频分辨率
 * - 内置可编程增益放大器(PGA)
 * - 支持模拟和数字麦克风
 * - 硬件音量控制和静音功能
 * - 可配置的淡入淡出效果
 * - I2C控制接口
 * - I2S音频数据接口
 * 
 * @hardware_connection
 * I2C接口：
 * - SDA: I2C数据线
 * - SCL: I2C时钟线
 * - 地址：0x18(CE引脚低电平) 或 0x19(CE引脚高电平)
 * 
 * I2S接口：
 * - MCLK: 主时钟输入
 * - SCLK/BCLK: 串行时钟
 * - LRCK/WS: 左右声道选择时钟
 * - SDIN: 串行数据输入(DAC)
 * - SDOUT: 串行数据输出(ADC)
 * 
 * @changelog
 * v1.1.0 - 添加详细中文注释和使用说明
 * v1.0.0 - Espressif原始驱动版本
 */

#pragma once

#include "esp_types.h"
#include "esp_err.h"

// === I2C总线管理器支持 ===
// 注意：ES8311驱动依赖统一的I2C总线管理器
// 使用前必须先调用 I2CBus_Init() 初始化I2C总线

// === ES8311设备I2C地址定义 ===
/**
 * ES8311芯片I2C地址配置
 * 地址由CE引脚电平状态决定：
 * - CE引脚接地(低电平)：地址为0x18
 * - CE引脚接VCC(高电平)：地址为0x19
 */
#define ES8311_ADDRRES_0 0x18u // 保持向后兼容的旧定义
#define ES8311_ADDRESS_0 0x18u // CE引脚低电平时的I2C地址
#define ES8311_ADDRESS_1 0x19u // CE引脚高电平时的I2C地址

#ifdef __cplusplus
extern "C" {
#endif

// === 类型定义 ===
/**
 * @brief ES8311设备句柄类型
 * 
 * 不透明句柄，用于标识特定的ES8311设备实例。
 * 通过es8311_create()函数创建，es8311_delete()函数销毁。
 */
typedef void *es8311_handle_t;

/**
 * @brief 麦克风增益枚举类型
 * 
 * 定义了麦克风输入的可选增益级别。
 * 增益越高，麦克风灵敏度越高，但噪声也可能增加。
 * 
 * @note 增益设置会影响录音质量和动态范围
 */
typedef enum {
    ES8311_MIC_GAIN_MIN = -1,  // 无效值，用于边界检查
    ES8311_MIC_GAIN_0DB,       // 0dB增益（无放大）
    ES8311_MIC_GAIN_6DB,       // 6dB增益（2倍放大）
    ES8311_MIC_GAIN_12DB,      // 12dB增益（4倍放大）
    ES8311_MIC_GAIN_18DB,      // 18dB增益（8倍放大）
    ES8311_MIC_GAIN_24DB,      // 24dB增益（16倍放大）
    ES8311_MIC_GAIN_30DB,      // 30dB增益（32倍放大）
    ES8311_MIC_GAIN_36DB,      // 36dB增益（64倍放大）
    ES8311_MIC_GAIN_42DB,      // 42dB增益（128倍放大）
    ES8311_MIC_GAIN_MAX        // 无效值，用于边界检查
} es8311_mic_gain_t;

/**
 * @brief 音频淡入淡出配置枚举
 * 
 * 定义音频渐变的速率，用于避免播放/录音时的爆音。
 * LRCK是左右声道时钟，数值越大渐变越慢。
 * 
 * @note 
 * - 适用于音频播放的开始和结束
 * - 可以减少音频切换时的爆音和咔嗒声
 * - 数值表示每多少个LRCK周期改变0.25dB
 */
typedef enum {
    ES8311_FADE_OFF = 0,       // 关闭淡入淡出功能
    ES8311_FADE_4LRCK,         // 每4个LRCK周期渐变0.25dB（最快）
    ES8311_FADE_8LRCK,         // 每8个LRCK周期渐变0.25dB
    ES8311_FADE_16LRCK,        // 每16个LRCK周期渐变0.25dB
    ES8311_FADE_32LRCK,        // 每32个LRCK周期渐变0.25dB
    ES8311_FADE_64LRCK,        // 每64个LRCK周期渐变0.25dB
    ES8311_FADE_128LRCK,       // 每128个LRCK周期渐变0.25dB
    ES8311_FADE_256LRCK,       // 每256个LRCK周期渐变0.25dB
    ES8311_FADE_512LRCK,       // 每512个LRCK周期渐变0.25dB
    ES8311_FADE_1024LRCK,      // 每1024个LRCK周期渐变0.25dB
    ES8311_FADE_2048LRCK,      // 每2048个LRCK周期渐变0.25dB
    ES8311_FADE_4096LRCK,      // 每4096个LRCK周期渐变0.25dB
    ES8311_FADE_8192LRCK,      // 每8192个LRCK周期渐变0.25dB
    ES8311_FADE_16384LRCK,     // 每16384个LRCK周期渐变0.25dB
    ES8311_FADE_32768LRCK,     // 每32768个LRCK周期渐变0.25dB
    ES8311_FADE_65536LRCK      // 每65536个LRCK周期渐变0.25dB（最慢）
} es8311_fade_t;

/**
 * @brief 音频分辨率枚举类型
 * 
 * 定义音频数据的位深度。更高的分辨率提供更好的音质，
 * 但同时需要更多的存储空间和处理能力。
 * 
 * @note 
 * - 16位：CD音质，适合大多数应用
 * - 24位：专业音频制作
 * - 32位：高端音频处理
 */
typedef enum es8311_resolution_t {
    ES8311_RESOLUTION_16 = 16, // 16位分辨率（65536个级别）
    ES8311_RESOLUTION_18 = 18, // 18位分辨率（262144个级别）
    ES8311_RESOLUTION_20 = 20, // 20位分辨率（1048576个级别）
    ES8311_RESOLUTION_24 = 24, // 24位分辨率（16777216个级别）
    ES8311_RESOLUTION_32 = 32  // 32位分辨率（4294967296个级别）
} es8311_resolution_t;

/**
 * @brief ES8311时钟配置结构体
 * 
 * 包含ES8311芯片工作所需的所有时钟参数。
 * 正确的时钟配置是音频质量的关键。
 * 
 * @note 
 * 主时钟(MCLK)可以来自两个源：
 * 1. 专用MCLK引脚 - 灵活性高，支持任意频率
 * 2. SCLK引脚 - 简化连接，频率受限
 */
typedef struct es8311_clock_config_t {
    bool mclk_inverted;      // MCLK时钟极性：true=反相, false=正常
    bool sclk_inverted;      // SCLK时钟极性：true=反相, false=正常
    bool mclk_from_mclk_pin; // MCLK源选择：true=来自MCLK引脚, false=来自SCLK引脚
    int  mclk_frequency;     // MCLK频率(Hz)，当mclk_from_mclk_pin=false时忽略
    int  sample_frequency;   // 采样频率(Hz)，如44100、48000等
} es8311_clock_config_t;

// === 函数声明 ===

/**
 * @brief 初始化ES8311音频编解码器
 * 
 * 完成ES8311芯片的完整初始化，包括时钟配置、分辨率设置等。
 * 
 * 主时钟(MCLK)提供方式：
 * 1. 从MCLK引脚获取：
 *    - 适用于灵活的应用场景
 *    - I2S主设备将时钟信号路由到MCLK引脚
 *    - 必须在clk_cfg->mclk_frequency参数中定义频率
 * 
 * 2. 从SCLK引脚获取：
 *    - 适用于简化连接的场景  
 *    - ES8311从SCK引脚获取时钟，无需连接MCLK引脚
 *    - 此模式下res_in必须等于res_out
 *    - clk_cfg->mclk_frequency参数被忽略
 *    - MCLK = clk_cfg->sample_frequency × res_out × 2
 *    - 不支持所有采样频率
 *
 * @param dev ES8311设备句柄
 * @param[in] clk_cfg 时钟配置参数
 * @param[in] res_in  输入串行端口分辨率
 * @param[in] res_out 输出串行端口分辨率
 * @return
 *     - ESP_OK 成功
 *     - ESP_ERR_INVALID_ARG 采样频率或分辨率无效
 *     - 其他值 失败
 */
esp_err_t es8311_init(es8311_handle_t dev, const es8311_clock_config_t *const clk_cfg, const es8311_resolution_t res_in,
                      const es8311_resolution_t res_out);

/**
 * @brief 设置输出音量
 * 
 * 控制ES8311的输出音量大小。超出<0, 100>范围的值会被截断。
 *
 * @param dev ES8311设备句柄
 * @param[in] volume 设置音量 (0 ~ 100)
 * @param[out] volume_set 实际设置的音量值。与volume相同，除非volume超出<0, 100>范围。
 *                        如果不需要此信息，可以设置为NULL。
 *
 * @return
 *     - ESP_OK 成功
 *     - 其他值 失败
 */
esp_err_t es8311_voice_volume_set(es8311_handle_t dev, int volume, int *volume_set);

/**
 * @brief 获取当前输出音量
 *
 * @param dev ES8311设备句柄
 * @param[out] volume 获取的音量值 (0 ~ 100)
 *
 * @return
 *     - ESP_OK 成功
 *     - 其他值 失败
 */
esp_err_t es8311_voice_volume_get(es8311_handle_t dev, int *volume);

/**
 * @brief 打印ES8311寄存器内容
 * 
 * 用于调试目的，将ES8311所有寄存器的值输出到串口。
 * 有助于分析芯片配置状态和故障排查。
 *
 * @param dev ES8311设备句柄
 */
void es8311_register_dump(es8311_handle_t dev);

/**
 * @brief 静音ES8311输出
 * 
 * 控制音频输出的静音状态，不影响音量设置。
 *
 * @param dev ES8311设备句柄
 * @param[in] enable true=静音, false=取消静音
 * @return
 *     - ESP_OK 成功
 *     - 其他值 失败
 */
esp_err_t es8311_voice_mute(es8311_handle_t dev, bool enable);

/**
 * @brief 设置麦克风增益
 * 
 * 调整麦克风输入的放大倍数。更高的增益可以提高灵敏度，
 * 但可能会引入更多噪声。
 *
 * @param dev ES8311设备句柄
 * @param[in] gain_db 麦克风增益级别
 * @return
 *     - ESP_OK 成功
 *     - 其他值 失败
 */
esp_err_t es8311_microphone_gain_set(es8311_handle_t dev, es8311_mic_gain_t gain_db);

/**
 * @brief 配置麦克风类型
 * 
 * 设置麦克风接口类型，支持模拟和数字麦克风。
 *
 * @param dev ES8311设备句柄
 * @param[in] digital_mic true=数字麦克风, false=模拟麦克风
 * @return
 *     - ESP_OK 成功
 *     - 其他值 失败
 */
esp_err_t es8311_microphone_config(es8311_handle_t dev, bool digital_mic);

/**
 * @brief 配置采样频率
 * 
 * 运行时动态改变采样频率。通常由es8311_init()调用，
 * 只有在需要运行时改变采样频率时才需要显式调用。
 * 
 * @note 此函数会重新计算和配置内部时钟分频器
 * 
 * @param dev ES8311设备句柄
 * @param[in] mclk_frequency   MCLK频率(Hz) (来自MCLK或SCLK引脚，取决于寄存器01[7]位)
 * @param[in] sample_frequency 所需采样频率(Hz)，如44100、22050等
 * @return
 *     - ESP_OK 成功
 *     - ESP_ERR_INVALID_ARG 无法为给定的MCLK和采样频率设置时钟分频器
 *     - 其他值 I2C读写错误
 */
esp_err_t es8311_sample_frequency_config(es8311_handle_t dev, int mclk_frequency, int sample_frequency);

/**
 * @brief 配置ADC音量淡入淡出
 * 
 * 为录音(ADC)配置渐变效果，避免录音开始和结束时的突变。
 *
 * @param dev ES8311设备句柄
 * @param[in] fade 淡入淡出斜率
 * @return
 *     - ESP_OK 成功
 *     - 其他值 I2C读写错误
 */
esp_err_t es8311_voice_fade(es8311_handle_t dev, const es8311_fade_t fade);

/**
 * @brief 配置DAC音量淡入淡出
 * 
 * 为播放(DAC)配置渐变效果，避免播放开始和结束时的突变。
 *
 * @param dev ES8311设备句柄
 * @param[in] fade 淡入淡出斜率
 * @return
 *     - ESP_OK 成功
 *     - 其他值 I2C读写错误
 */
esp_err_t es8311_microphone_fade(es8311_handle_t dev, const es8311_fade_t fade);

/**
 * @brief 创建ES8311设备句柄
 * 
 * 分配内存并初始化ES8311设备句柄。必须与es8311_delete()配对使用。
 * 使用统一的I2C总线管理器进行设备通信。
 *
 * @param[in] dev_addr 设备I2C地址 (0x18或0x19)
 * @return ES8311设备句柄，失败时返回NULL
 * 
 * @note 
 * - 调用前必须先初始化I2C总线管理器 (I2CBus_Init())
 * - 函数会自动验证设备是否存在于I2C总线上
 */
es8311_handle_t es8311_create(const uint8_t dev_addr);

/**
 * @brief 删除ES8311设备句柄
 * 
 * 释放由es8311_create()分配的内存。
 *
 * @param dev ES8311设备句柄
 */
void es8311_delete(es8311_handle_t dev);

#ifdef __cplusplus
}
#endif
