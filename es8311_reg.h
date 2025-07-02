/*
 * SPDX-FileCopyrightText: 2015-2021 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file es8311_reg.h
 * @brief ES8311音频编解码器寄存器地址定义文件
 * @version 1.1.0
 * @date 2024-01-01
 * @author Espressif Systems & 系统管理员
 * 
 * @description
 * 本文件定义了ES8311音频编解码器芯片的所有寄存器地址。
 * ES8311通过I2C接口进行配置，每个寄存器控制芯片的不同功能。
 * 
 * @register_layout
 * 寄存器分组：
 * - 0x00: 复位控制
 * - 0x01-0x08: 时钟管理
 * - 0x09-0x0A: 串行数据端口
 * - 0x0B-0x14: 系统控制
 * - 0x15-0x1C: ADC控制
 * - 0x31-0x37: DAC控制
 * - 0x44-0x45: GPIO控制
 * - 0xFD-0xFF: 芯片ID和版本
 * 
 * @changelog
 * v1.1.0 - 添加详细中文注释和功能说明
 * v1.0.0 - Espressif原始版本
 */

#pragma once

// === 复位控制寄存器 ===
/**
 * @brief 复位寄存器 (0x00)
 * 
 * 控制数字电路、时钟管理器等模块的复位状态
 * 写入特定值可复位相应的功能模块
 */
#define ES8311_RESET_REG00              0x00

// === 时钟系统寄存器组 (0x01-0x08) ===
/**
 * @brief 时钟管理寄存器1 (0x01)
 * 
 * 功能：
 * - 选择MCLK时钟源
 * - 使能编解码器时钟
 * - 控制时钟输出
 */
#define ES8311_CLK_MANAGER_REG01        0x01

/**
 * @brief 时钟管理寄存器2 (0x02) 
 * 
 * 功能：
 * - 时钟分频器配置
 * - 时钟倍频器配置
 * - 预分频设置
 */
#define ES8311_CLK_MANAGER_REG02        0x02

/**
 * @brief 时钟管理寄存器3 (0x03)
 * 
 * 功能：
 * - ADC快速/慢速模式选择
 * - ADC过采样率(OSR)配置
 */
#define ES8311_CLK_MANAGER_REG03        0x03

/**
 * @brief 时钟管理寄存器4 (0x04)
 * 
 * 功能：
 * - DAC过采样率(OSR)配置
 * - DAC时钟频率控制
 */
#define ES8311_CLK_MANAGER_REG04        0x04

/**
 * @brief 时钟管理寄存器5 (0x05)
 * 
 * 功能：
 * - ADC时钟分频器
 * - DAC时钟分频器
 */
#define ES8311_CLK_MANAGER_REG05        0x05

/**
 * @brief 时钟管理寄存器6 (0x06)
 * 
 * 功能：
 * - BCLK(位时钟)反相控制
 * - BCLK分频器设置
 */
#define ES8311_CLK_MANAGER_REG06        0x06

/**
 * @brief 时钟管理寄存器7 (0x07)
 * 
 * 功能：
 * - 三态控制
 * - LRCK(左右声道时钟)分频器高位
 */
#define ES8311_CLK_MANAGER_REG07        0x07

/**
 * @brief 时钟管理寄存器8 (0x08)
 * 
 * 功能：
 * - LRCK分频器低位
 * - 完整的LRCK分频比设置
 */
#define ES8311_CLK_MANAGER_REG08        0x08

// === 串行数据端口寄存器组 (0x09-0x0A) ===
/**
 * @brief 串行数据输入端口寄存器 (0x09)
 * 
 * 功能：
 * - DAC串行数字端口配置
 * - 数据格式设置(I2S/左对齐/右对齐)
 * - 字长设置
 */
#define ES8311_SDPIN_REG09              0x09

/**
 * @brief 串行数据输出端口寄存器 (0x0A)
 * 
 * 功能：
 * - ADC串行数字端口配置
 * - 输出数据格式设置
 * - 输出字长设置
 */
#define ES8311_SDPOUT_REG0A             0x0A

// === 系统控制寄存器组 (0x0B-0x14) ===
/**
 * @brief 系统寄存器B (0x0B)
 * 
 * 功能：
 * - 系统级控制设置
 * - 工作模式配置
 */
#define ES8311_SYSTEM_REG0B             0x0B

/**
 * @brief 系统寄存器C (0x0C)
 * 
 * 功能：
 * - 系统级控制设置
 * - 接口配置
 */
#define ES8311_SYSTEM_REG0C             0x0C

/**
 * @brief 系统寄存器D (0x0D)
 * 
 * 功能：
 * - 系统电源管理
 * - 模块上电/断电控制
 */
#define ES8311_SYSTEM_REG0D             0x0D

/**
 * @brief 系统寄存器E (0x0E)
 * 
 * 功能：
 * - 系统电源管理
 * - 各功能模块使能控制
 */
#define ES8311_SYSTEM_REG0E             0x0E

/**
 * @brief 系统寄存器F (0x0F)
 * 
 * 功能：
 * - 系统低功耗模式
 * - 待机模式控制
 */
#define ES8311_SYSTEM_REG0F             0x0F

/**
 * @brief 系统寄存器10 (0x10)
 * 
 * 功能：
 * - 系统配置设置
 * - 工作参数调整
 */
#define ES8311_SYSTEM_REG10             0x10

/**
 * @brief 系统寄存器11 (0x11)
 * 
 * 功能：
 * - 系统配置设置
 * - 高级功能控制
 */
#define ES8311_SYSTEM_REG11             0x11

/**
 * @brief 系统寄存器12 (0x12)
 * 
 * 功能：
 * - 系统控制
 * - DAC使能控制
 */
#define ES8311_SYSTEM_REG12             0x12

/**
 * @brief 系统寄存器13 (0x13)
 * 
 * 功能：
 * - 系统配置
 * - 特殊功能设置
 */
#define ES8311_SYSTEM_REG13             0x13

/**
 * @brief 系统寄存器14 (0x14)
 * 
 * 功能：
 * - 选择数字麦克风/模拟麦克风
 * - 模拟PGA增益选择
 * - 麦克风接口配置
 */
#define ES8311_SYSTEM_REG14             0x14

// === ADC控制寄存器组 (0x15-0x1C) ===
/**
 * @brief ADC寄存器15 (0x15)
 * 
 * 功能：
 * - ADC斜率控制
 * - 数字麦克风灵敏度设置
 * - ADC淡入淡出控制
 */
#define ES8311_ADC_REG15                0x15

/**
 * @brief ADC寄存器16 (0x16)
 * 
 * 功能：
 * - ADC配置设置
 * - 录音路径控制
 */
#define ES8311_ADC_REG16                0x16

/**
 * @brief ADC音量寄存器 (0x17)
 * 
 * 功能：
 * - ADC数字音量控制
 * - 录音增益调整
 * - 音量范围：0dB到-95.5dB
 */
#define ES8311_ADC_REG17                0x17

/**
 * @brief ADC自动电平控制寄存器1 (0x18)
 * 
 * 功能：
 * - ALC(自动电平控制)使能
 * - ALC检测窗口大小设置
 * - 自动增益控制参数
 */
#define ES8311_ADC_REG18                0x18

/**
 * @brief ADC自动电平控制寄存器2 (0x19)
 * 
 * 功能：
 * - ALC最大电平设置
 * - 压缩比控制
 */
#define ES8311_ADC_REG19                0x19

/**
 * @brief ADC自动静音寄存器1 (0x1A)
 * 
 * 功能：
 * - ALC自动静音功能
 * - 静音阈值设置
 */
#define ES8311_ADC_REG1A                0x1A

/**
 * @brief ADC自动静音和高通滤波器寄存器 (0x1B)
 * 
 * 功能：
 * - ALC自动静音控制
 * - ADC高通滤波器第一级
 * - 低频噪声抑制
 */
#define ES8311_ADC_REG1B                0x1B

/**
 * @brief ADC均衡器和高通滤波器寄存器 (0x1C)
 * 
 * 功能：
 * - ADC均衡器设置
 * - 高通滤波器第二级
 * - 音频信号预处理
 */
#define ES8311_ADC_REG1C                0x1C

// === DAC控制寄存器组 (0x31-0x37) ===
/**
 * @brief DAC静音寄存器 (0x31)
 * 
 * 功能：
 * - DAC软件静音控制
 * - 自动静音功能
 * - 静音状态指示
 */
#define ES8311_DAC_REG31                0x31

/**
 * @brief DAC音量寄存器 (0x32)
 * 
 * 功能：
 * - DAC数字音量控制
 * - 播放音量调整
 * - 音量范围：0dB到-95.5dB
 */
#define ES8311_DAC_REG32                0x32

/**
 * @brief DAC偏移寄存器 (0x33)
 * 
 * 功能：
 * - DAC输出偏移调整
 * - 直流偏移补偿
 * - 输出信号校准
 */
#define ES8311_DAC_REG33                0x33

/**
 * @brief DAC动态范围压缩寄存器1 (0x34)
 * 
 * 功能：
 * - DRC(动态范围压缩)使能
 * - DRC检测窗口大小
 * - 音频动态处理
 */
#define ES8311_DAC_REG34                0x34

/**
 * @brief DAC动态范围压缩寄存器2 (0x35)
 * 
 * 功能：
 * - DRC最大电平设置
 * - DRC最小电平设置
 * - 压缩阈值控制
 */
#define ES8311_DAC_REG35                0x35

/**
 * @brief DAC斜率寄存器 (0x37)
 * 
 * 功能：
 * - DAC淡入淡出斜率控制
 * - 避免播放时爆音
 * - 渐变速度设置
 */
#define ES8311_DAC_REG37                0x37

// === GPIO控制寄存器组 (0x44-0x45) ===
/**
 * @brief GPIO控制寄存器 (0x44)
 * 
 * 功能：
 * - GPIO引脚功能配置
 * - DAC到ADC测试回环
 * - 调试和测试功能
 */
#define ES8311_GPIO_REG44               0x44

/**
 * @brief 通用控制寄存器 (0x45)
 * 
 * 功能：
 * - 通用功能控制
 * - 特殊模式设置
 * - 高级配置选项
 */
#define ES8311_GP_REG45                 0x45

// === 芯片识别寄存器组 (0xFD-0xFF) ===
/**
 * @brief 芯片ID寄存器1 (0xFD)
 * 
 * 功能：
 * - 芯片标识符第一部分
 * - 用于验证芯片型号
 * - 只读寄存器
 */
#define ES8311_CHD1_REGFD               0xFD

/**
 * @brief 芯片ID寄存器2 (0xFE)
 * 
 * 功能：
 * - 芯片标识符第二部分
 * - 与CHD1组合确定芯片型号
 * - 只读寄存器
 */
#define ES8311_CHD2_REGFE               0xFE

/**
 * @brief 版本寄存器 (0xFF)
 * 
 * 功能：
 * - 芯片版本信息
 * - 硬件修订版本
 * - 只读寄存器
 */
#define ES8311_CHVER_REGFF              0xFF

// === 寄存器地址范围定义 ===
/**
 * @brief ES8311最大寄存器地址
 * 
 * 定义ES8311芯片支持的最大寄存器地址值，
 * 用于寄存器访问的边界检查。
 */
#define ES8311_MAX_REGISTER             0xFF
