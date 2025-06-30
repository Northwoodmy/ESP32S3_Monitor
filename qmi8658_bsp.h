#ifndef QMI8658_BSP_H
#define QMI8658_BSP_H

#include <stdint.h>
#include <stdbool.h>
#include "touch_bsp.h"  // 引用触摸屏BSP以使用I2C互斥锁

#ifdef __cplusplus
extern "C" {
#endif

// === QMI8658 设备地址 ===
#define QMI8658_L_SLAVE_ADDRESS                 (0x6B)
#define QMI8658_H_SLAVE_ADDRESS                 (0x6A)

// === 寄存器地址定义（基于官方QMI8658Constants.h）===
// 通用寄存器
#define QMI8658_REG_WHOAMI                      (0x00)
#define QMI8658_REG_REVISION                    (0x01)

// 设置和控制寄存器
#define QMI8658_REG_CTRL1                       (0x02)
#define QMI8658_REG_CTRL2                       (0x03)
#define QMI8658_REG_CTRL3                       (0x04)
#define QMI8658_REG_CTRL5                       (0x06)
#define QMI8658_REG_CTRL7                       (0x08)
#define QMI8658_REG_CTRL8                       (0x09)
#define QMI8658_REG_CTRL9                       (0x0A)

// 主机控制校准寄存器
#define QMI8658_REG_CAL1_L                      (0x0B)
#define QMI8658_REG_CAL1_H                      (0x0C)
#define QMI8658_REG_CAL2_L                      (0x0D)
#define QMI8658_REG_CAL2_H                      (0x0E)
#define QMI8658_REG_CAL3_L                      (0x0F)
#define QMI8658_REG_CAL3_H                      (0x10)
#define QMI8658_REG_CAL4_L                      (0x11)
#define QMI8658_REG_CAL4_H                      (0x12)

// FIFO寄存器
#define QMI8658_REG_FIFO_WTM_TH                 (0x13)
#define QMI8658_REG_FIFO_CTRL                   (0x14)
#define QMI8658_REG_FIFO_COUNT                  (0x15)
#define QMI8658_REG_FIFO_STATUS                 (0x16)
#define QMI8658_REG_FIFO_DATA                   (0x17)

// 状态寄存器
#define QMI8658_REG_STATUS_INT                  (0x2D)
#define QMI8658_REG_STATUS0                     (0x2E)
#define QMI8658_REG_STATUS1                     (0x2F)

// 时间戳寄存器
#define QMI8658_REG_TIMESTAMP_L                 (0x30)
#define QMI8658_REG_TIMESTAMP_M                 (0x31)
#define QMI8658_REG_TIMESTAMP_H                 (0x32)

// 数据输出寄存器
#define QMI8658_REG_TEMPERATURE_L               (0x33)
#define QMI8658_REG_TEMPERATURE_H               (0x34)
#define QMI8658_REG_AX_L                        (0x35)
#define QMI8658_REG_AX_H                        (0x36)
#define QMI8658_REG_AY_L                        (0x37)
#define QMI8658_REG_AY_H                        (0x38)
#define QMI8658_REG_AZ_L                        (0x39)
#define QMI8658_REG_AZ_H                        (0x3A)
#define QMI8658_REG_GX_L                        (0x3B)
#define QMI8658_REG_GX_H                        (0x3C)
#define QMI8658_REG_GY_L                        (0x3D)
#define QMI8658_REG_GY_H                        (0x3E)
#define QMI8658_REG_GZ_L                        (0x3F)
#define QMI8658_REG_GZ_H                        (0x40)

// COD相关寄存器
#define QMI8658_REG_COD_STATUS                  (0x46)
#define QMI8658_REG_DQW_L                       (0x49)
#define QMI8658_REG_DQW_H                       (0x4A)
#define QMI8658_REG_DQX_L                       (0x4B)
#define QMI8658_REG_DQX_H                       (0x4C)
#define QMI8658_REG_DQY_L                       (0x4D)
#define QMI8658_REG_DQY_H                       (0x4E)
#define QMI8658_REG_DQZ_L                       (0x4F)
#define QMI8658_REG_DQZ_H                       (0x50)
#define QMI8658_REG_DVX_L                       (0x51)
#define QMI8658_REG_DVX_H                       (0x52)
#define QMI8658_REG_DVY_L                       (0x53)
#define QMI8658_REG_DVY_H                       (0x54)
#define QMI8658_REG_DVZ_L                       (0x55)
#define QMI8658_REG_DVZ_H                       (0x56)

// 活动检测输出寄存器
#define QMI8658_REG_TAP_STATUS                  (0x59)
#define QMI8658_REG_STEP_CNT_LOW                (0x5A)
#define QMI8658_REG_STEP_CNT_MID                (0x5B)
#define QMI8658_REG_STEP_CNT_HIGH               (0x5C)

// 重置寄存器
#define QMI8658_REG_RESET                       (0x60)
// 注意：QMI8658_REG_RST_RESULT 与 QMI8658_REG_DQY_L 地址冲突，按官方文档应该去掉重复定义

// === 默认值定义 ===
#define QMI8658_REG_WHOAMI_DEFAULT              (0x05)
#define QMI8658_REG_STATUS_DEFAULT              (0x03)
#define QMI8658_REG_RESET_DEFAULT               (0xB0)
#define QMI8658_REG_RST_RESULT_VAL              (0x80)

// === 控制位定义 ===
#define QMI8658_ACCEL_EN_MASK                   (0x01)
#define QMI8658_GYRO_EN_MASK                    (0x02)
#define QMI8658_ACCEL_GYRO_EN_MASK              (0x03)
#define QMI8658_ACCEL_LPF_MASK                  (0xF9)
#define QMI8658_GYRO_LPF_MASK                   (0x9F)
#define QMI8658_FIFO_MAP_INT1                   (0x04)

// === 状态位定义 ===
#define QMI8658_STATUS0_ACCEL_AVAIL             (0x01)
#define QMI8658_STATUS0_GYRO_AVAIL              (0x02)

// === 枚举定义（基于官方SensorQMI8658.hpp）===
typedef enum {
    QMI8658_ACC_RANGE_2G = 0,
    QMI8658_ACC_RANGE_4G,
    QMI8658_ACC_RANGE_8G,
    QMI8658_ACC_RANGE_16G
} QMI8658_AccelRange_t;

typedef enum {
    QMI8658_GYR_RANGE_16DPS = 0,
    QMI8658_GYR_RANGE_32DPS,
    QMI8658_GYR_RANGE_64DPS,
    QMI8658_GYR_RANGE_128DPS,
    QMI8658_GYR_RANGE_256DPS,
    QMI8658_GYR_RANGE_512DPS,
    QMI8658_GYR_RANGE_1024DPS
} QMI8658_GyroRange_t;

typedef enum {
    QMI8658_ACC_ODR_1000Hz = 3,
    QMI8658_ACC_ODR_500Hz,
    QMI8658_ACC_ODR_250Hz,
    QMI8658_ACC_ODR_125Hz,
    QMI8658_ACC_ODR_62_5Hz,
    QMI8658_ACC_ODR_31_25Hz,
    QMI8658_ACC_ODR_LOWPOWER_128Hz = 12,
    QMI8658_ACC_ODR_LOWPOWER_21Hz,
    QMI8658_ACC_ODR_LOWPOWER_11Hz,
    QMI8658_ACC_ODR_LOWPOWER_3Hz
} QMI8658_AccelODR_t;

typedef enum {
    QMI8658_GYR_ODR_7174_4Hz = 0,
    QMI8658_GYR_ODR_3587_2Hz,
    QMI8658_GYR_ODR_1793_6Hz,
    QMI8658_GYR_ODR_896_8Hz,
    QMI8658_GYR_ODR_448_4Hz,
    QMI8658_GYR_ODR_224_2Hz,
    QMI8658_GYR_ODR_112_1Hz,
    QMI8658_GYR_ODR_56_05Hz,
    QMI8658_GYR_ODR_28_025Hz
} QMI8658_GyroODR_t;

typedef enum {
    QMI8658_LPF_MODE_0 = 0,    // 2.66% of ODR
    QMI8658_LPF_MODE_1,        // 3.63% of ODR
    QMI8658_LPF_MODE_2,        // 5.39% of ODR
    QMI8658_LPF_MODE_3,        // 13.37% of ODR
    QMI8658_LPF_OFF            // OFF Low-Pass Filter
} QMI8658_LpfMode_t;

typedef enum {
    QMI8658_TAP_EVENT_INVALID = 0,
    QMI8658_TAP_EVENT_SINGLE,
    QMI8658_TAP_EVENT_DOUBLE
} QMI8658_TapEvent_t;

typedef enum {
    QMI8658_FIFO_MODE_BYPASS = 0,  // 禁用FIFO功能
    QMI8658_FIFO_MODE_FIFO,        // FIFO模式：满后停止写入
    QMI8658_FIFO_MODE_STREAM       // 流模式：满后覆盖旧数据
} QMI8658_FifoMode_t;

typedef enum {
    QMI8658_FIFO_SAMPLES_16 = 0,
    QMI8658_FIFO_SAMPLES_32,
    QMI8658_FIFO_SAMPLES_64,
    QMI8658_FIFO_SAMPLES_128
} QMI8658_FifoSamples_t;

typedef enum {
    QMI8658_INT_PIN_1 = 0,
    QMI8658_INT_PIN_2,
    QMI8658_INT_PIN_DISABLE
} QMI8658_IntPin_t;

typedef enum {
    QMI8658_SAMPLE_MODE_SYNC = 0,    // 同步采样模式
    QMI8658_SAMPLE_MODE_ASYNC        // 异步采样模式
} QMI8658_SampleMode_t;

typedef enum {
    QMI8658_TAP_PRIORITY_X_Y_Z = 0,  // X > Y > Z
    QMI8658_TAP_PRIORITY_X_Z_Y,      // X > Z > Y  
    QMI8658_TAP_PRIORITY_Y_X_Z,      // Y > X > Z
    QMI8658_TAP_PRIORITY_Y_Z_X,      // Y > Z > X
    QMI8658_TAP_PRIORITY_Z_X_Y,      // Z > X > Y
    QMI8658_TAP_PRIORITY_Z_Y_X       // Z > Y > X
} QMI8658_TapPriority_t;

// 命令表定义
typedef enum {
    QMI8658_CTRL_CMD_ACK                            = 0x00,
    QMI8658_CTRL_CMD_RST_FIFO                       = 0x04,
    QMI8658_CTRL_CMD_REQ_FIFO                       = 0x05,
    QMI8658_CTRL_CMD_WRITE_WOM_SETTING              = 0x08,
    QMI8658_CTRL_CMD_ACCEL_HOST_DELTA_OFFSET        = 0x09,
    QMI8658_CTRL_CMD_GYRO_HOST_DELTA_OFFSET         = 0x0A,
    QMI8658_CTRL_CMD_CONFIGURE_TAP                  = 0x0C,
    QMI8658_CTRL_CMD_CONFIGURE_PEDOMETER            = 0x0D,
    QMI8658_CTRL_CMD_CONFIGURE_MOTION               = 0x0E,
    QMI8658_CTRL_CMD_RESET_PEDOMETER                = 0x0F,
    QMI8658_CTRL_CMD_COPY_USID                      = 0x10,
    QMI8658_CTRL_CMD_SET_RPU                        = 0x11,
    QMI8658_CTRL_CMD_AHB_CLOCK_GATING               = 0x12,
    QMI8658_CTRL_CMD_ON_DEMAND_CALIBRATION          = 0xA2,
    QMI8658_CTRL_CMD_APPLY_GYRO_GAINS               = 0xAA
} QMI8658_CommandTable_t;

// === 数据结构定义 ===
typedef struct {
    float x;
    float y;
    float z;
} QMI8658_IMUData_t;

typedef struct {
    uint8_t chip_id;
    uint32_t revision_id;
    uint8_t usid[6];
    bool accel_enabled;
    bool gyro_enabled;
    float accel_scale;
    float gyro_scale;
    QMI8658_AccelRange_t accel_range;
    QMI8658_GyroRange_t gyro_range;
} QMI8658_DeviceInfo_t;

typedef struct {
    uint16_t ped_sample_cnt;
    uint16_t ped_fix_peak2peak;
    uint16_t ped_fix_peak;
    uint16_t ped_time_up;
    uint8_t ped_time_low;
    uint8_t ped_time_cnt_entry;
    uint8_t ped_fix_precision;
} QMI8658_PedometerConfig_t;

typedef struct {
    uint8_t priority;
    uint8_t peak_window;
    uint16_t tap_window;
    uint16_t dtap_window;
    float alpha;
    float gamma;
    float peak_mag_thr;
    float udm_thr;
} QMI8658_TapConfig_t;

typedef struct {
    uint8_t mode_ctrl;              // 运动控制模式
    float any_motion_x_thr;         // X轴任意运动阈值
    float any_motion_y_thr;         // Y轴任意运动阈值
    float any_motion_z_thr;         // Z轴任意运动阈值
    uint8_t any_motion_window;      // 任意运动时间窗口
    float no_motion_x_thr;          // X轴无运动阈值
    float no_motion_y_thr;          // Y轴无运动阈值
    float no_motion_z_thr;          // Z轴无运动阈值
    uint8_t no_motion_window;       // 无运动时间窗口
    uint16_t sig_motion_wait_window;    // 显著运动等待窗口
    uint16_t sig_motion_confirm_window; // 显著运动确认窗口
} QMI8658_MotionConfig_t;

// === 基础接口函数 ===

/**
 * @brief 初始化 QMI8658 传感器
 * @return 0: 成功, 非0: 失败
 */
uint8_t QMI8658_Init(void);

/**
 * @brief 反初始化 QMI8658 传感器
 */
void QMI8658_Deinit(void);

/**
 * @brief 检查传感器是否连接
 * @return 1: 已连接, 0: 未连接
 */
uint8_t QMI8658_IsConnected(void);

/**
 * @brief 获取芯片 ID
 * @return 芯片 ID
 */
uint8_t QMI8658_GetChipID(void);

/**
 * @brief 获取设备信息
 * @param info 设备信息结构体指针
 * @return 0: 成功, 非0: 失败
 */
uint8_t QMI8658_GetDeviceInfo(QMI8658_DeviceInfo_t *info);

/**
 * @brief 软重置传感器
 * @return 0: 成功, 非0: 失败
 */
uint8_t QMI8658_Reset(void);

// === 配置接口函数 ===

/**
 * @brief 配置加速度计
 * @param range 测量范围
 * @param odr 输出数据率
 * @param lpf 低通滤波器模式
 * @return 0: 成功, 非0: 失败
 */
uint8_t QMI8658_ConfigAccelerometer(QMI8658_AccelRange_t range, 
                                   QMI8658_AccelODR_t odr, 
                                   QMI8658_LpfMode_t lpf);

/**
 * @brief 配置陀螺仪
 * @param range 测量范围
 * @param odr 输出数据率
 * @param lpf 低通滤波器模式
 * @return 0: 成功, 非0: 失败
 */
uint8_t QMI8658_ConfigGyroscope(QMI8658_GyroRange_t range, 
                               QMI8658_GyroODR_t odr, 
                               QMI8658_LpfMode_t lpf);

/**
 * @brief 使能加速度计
 * @return 0: 成功, 非0: 失败
 */
uint8_t QMI8658_EnableAccelerometer(void);

/**
 * @brief 禁用加速度计
 * @return 0: 成功, 非0: 失败
 */
uint8_t QMI8658_DisableAccelerometer(void);

/**
 * @brief 使能陀螺仪
 * @return 0: 成功, 非0: 失败
 */
uint8_t QMI8658_EnableGyroscope(void);

/**
 * @brief 禁用陀螺仪
 * @return 0: 成功, 非0: 失败
 */
uint8_t QMI8658_DisableGyroscope(void);

// === 数据读取接口函数 ===

/**
 * @brief 获取加速度计数据
 * @param data 数据结构体指针
 * @return 1: 成功, 0: 失败
 */
uint8_t QMI8658_GetAccelerometerData(QMI8658_IMUData_t *data);

/**
 * @brief 获取陀螺仪数据
 * @param data 数据结构体指针
 * @return 1: 成功, 0: 失败
 */
uint8_t QMI8658_GetGyroscopeData(QMI8658_IMUData_t *data);

/**
 * @brief 获取加速度数据（分量形式）
 * @param x X轴加速度指针
 * @param y Y轴加速度指针
 * @param z Z轴加速度指针
 * @return 1: 成功, 0: 失败
 */
uint8_t QMI8658_GetAcceleration(float *x, float *y, float *z);

/**
 * @brief 获取角速度数据（分量形式）
 * @param x X轴角速度指针
 * @param y Y轴角速度指针
 * @param z Z轴角速度指针
 * @return 1: 成功, 0: 失败
 */
uint8_t QMI8658_GetAngularVelocity(float *x, float *y, float *z);

/**
 * @brief 获取原始加速度计数据
 * @param raw_data 原始数据数组指针（6字节）
 * @return 1: 成功, 0: 失败
 */
uint8_t QMI8658_GetAccelRaw(int16_t *raw_data);

/**
 * @brief 获取原始陀螺仪数据
 * @param raw_data 原始数据数组指针（6字节）
 * @return 1: 成功, 0: 失败
 */
uint8_t QMI8658_GetGyroRaw(int16_t *raw_data);

/**
 * @brief 获取温度值
 * @return 温度值（摄氏度）
 */
float QMI8658_GetTemperature(void);

/**
 * @brief 获取时间戳
 * @return 时间戳值
 */
uint32_t QMI8658_GetTimestamp(void);

/**
 * @brief 获取数据就绪状态
 * @return 状态位组合
 */
uint8_t QMI8658_GetDataReady(void);

// === FIFO接口函数 ===

/**
 * @brief 配置FIFO
 * @param mode FIFO模式
 * @param samples FIFO采样数量
 * @return 0: 成功, 非0: 失败
 */
uint8_t QMI8658_ConfigFIFO(QMI8658_FifoMode_t mode, QMI8658_FifoSamples_t samples);

/**
 * @brief 使能FIFO
 * @return 0: 成功, 非0: 失败
 */
uint8_t QMI8658_EnableFIFO(void);

/**
 * @brief 禁用FIFO
 * @return 0: 成功, 非0: 失败
 */
uint8_t QMI8658_DisableFIFO(void);

/**
 * @brief 重置FIFO
 * @return 0: 成功, 非0: 失败
 */
uint8_t QMI8658_ResetFIFO(void);

/**
 * @brief 获取FIFO计数
 * @return FIFO中的数据个数
 */
uint16_t QMI8658_GetFIFOCount(void);

/**
 * @brief 从FIFO读取数据
 * @param acc_data 加速度计数据数组
 * @param acc_length 加速度计数组长度
 * @param gyro_data 陀螺仪数据数组
 * @param gyro_length 陀螺仪数组长度
 * @return 实际读取的数据个数
 */
uint16_t QMI8658_ReadFromFIFO(QMI8658_IMUData_t *acc_data, uint16_t acc_length,
                              QMI8658_IMUData_t *gyro_data, uint16_t gyro_length);

// === 高级功能接口函数 ===

/**
 * @brief 配置敲击检测
 * @param config 敲击配置结构体指针
 * @return 0: 成功, 非0: 失败
 */
uint8_t QMI8658_ConfigTap(const QMI8658_TapConfig_t *config);

/**
 * @brief 使能敲击检测
 * @param pin 中断引脚配置
 * @return 0: 成功, 非0: 失败
 */
uint8_t QMI8658_EnableTap(QMI8658_IntPin_t pin);

/**
 * @brief 禁用敲击检测
 * @return 0: 成功, 非0: 失败
 */
uint8_t QMI8658_DisableTap(void);

/**
 * @brief 获取敲击状态
 * @return 敲击事件类型
 */
QMI8658_TapEvent_t QMI8658_GetTapStatus(void);

/**
 * @brief 配置计步器
 * @param config 计步器配置结构体指针
 * @return 0: 成功, 非0: 失败
 */
uint8_t QMI8658_ConfigPedometer(const QMI8658_PedometerConfig_t *config);

/**
 * @brief 使能计步器
 * @param pin 中断引脚配置
 * @return 0: 成功, 非0: 失败
 */
uint8_t QMI8658_EnablePedometer(QMI8658_IntPin_t pin);

/**
 * @brief 禁用计步器
 * @return 0: 成功, 非0: 失败
 */
uint8_t QMI8658_DisablePedometer(void);

/**
 * @brief 获取计步器计数
 * @return 步数计数
 */
uint32_t QMI8658_GetPedometerCounter(void);

/**
 * @brief 清除计步器计数
 * @return 0: 成功, 非0: 失败
 */
uint8_t QMI8658_ClearPedometerCounter(void);

/**
 * @brief 配置运动唤醒
 * @param threshold 阈值 (默认200)
 * @param blank_time 空白时间 (默认0x18)
 * @return 0: 成功, 非0: 失败
 */
uint8_t QMI8658_ConfigWakeOnMotion(uint8_t threshold, uint8_t blank_time);

/**
 * @brief 使能运动唤醒
 * @param pin 中断引脚配置
 * @return 0: 成功, 非0: 失败
 */
uint8_t QMI8658_EnableWakeOnMotion(QMI8658_IntPin_t pin);

/**
 * @brief 禁用运动唤醒
 * @return 0: 成功, 非0: 失败
 */
uint8_t QMI8658_DisableWakeOnMotion(void);

/**
 * @brief 配置运动检测
 * @param config 运动检测配置参数
 * @return 0: 成功, 非0: 失败
 */
uint8_t QMI8658_ConfigMotion(const QMI8658_MotionConfig_t *config);

/**
 * @brief 使能运动检测
 * @param pin 中断引脚选择
 * @return 0: 成功, 非0: 失败
 */
uint8_t QMI8658_EnableMotionDetect(QMI8658_IntPin_t pin);

/**
 * @brief 禁用运动检测
 * @return 0: 成功, 非0: 失败
 */
uint8_t QMI8658_DisableMotionDetect(void);

// === 采样模式接口函数 ===

/**
 * @brief 使能同步采样模式
 * @return 0: 成功, 非0: 失败
 */
uint8_t QMI8658_EnableSyncSampleMode(void);

/**
 * @brief 禁用同步采样模式
 * @return 0: 成功, 非0: 失败
 */
uint8_t QMI8658_DisableSyncSampleMode(void);

/**
 * @brief 使能锁定机制
 * @return 0: 成功, 非0: 失败
 */
uint8_t QMI8658_EnableLockingMechanism(void);

/**
 * @brief 禁用锁定机制
 * @return 0: 成功, 非0: 失败
 */
uint8_t QMI8658_DisableLockingMechanism(void);

/**
 * @brief 配置活动中断映射
 * @param pin 中断引脚选择
 * @return 0: 成功, 非0: 失败
 */
uint8_t QMI8658_ConfigActivityInterruptMap(QMI8658_IntPin_t pin);

// === 校准和自检接口函数 ===

/**
 * @brief 陀螺仪校准
 * @return 1: 成功, 0: 失败
 */
uint8_t QMI8658_CalibrateGyro(void);

/**
 * @brief 在线校准（自动校准）
 * @param gX_gain X轴增益指针（可为NULL）
 * @param gY_gain Y轴增益指针（可为NULL）  
 * @param gZ_gain Z轴增益指针（可为NULL）
 * @return 1: 成功, 0: 失败
 */
uint8_t QMI8658_OnDemandCalibration(uint16_t *gX_gain, uint16_t *gY_gain, uint16_t *gZ_gain);

/**
 * @brief 写入校准参数
 * @param gX_gain X轴增益
 * @param gY_gain Y轴增益
 * @param gZ_gain Z轴增益
 * @return 1: 成功, 0: 失败
 */
uint8_t QMI8658_WriteCalibration(uint16_t gX_gain, uint16_t gY_gain, uint16_t gZ_gain);

/**
 * @brief 设置加速度计偏移
 * @param offset_x X轴偏移
 * @param offset_y Y轴偏移
 * @param offset_z Z轴偏移
 */
void QMI8658_SetAccelOffset(int16_t offset_x, int16_t offset_y, int16_t offset_z);

/**
 * @brief 设置陀螺仪偏移
 * @param offset_x X轴偏移
 * @param offset_y Y轴偏移
 * @param offset_z Z轴偏移
 */
void QMI8658_SetGyroOffset(int16_t offset_x, int16_t offset_y, int16_t offset_z);

/**
 * @brief 加速度计自检
 * @return 1: 成功, 0: 失败
 */
uint8_t QMI8658_SelfTestAccel(void);

/**
 * @brief 陀螺仪自检
 * @return 1: 成功, 0: 失败
 */
uint8_t QMI8658_SelfTestGyro(void);

// === 中断和状态接口函数 ===

/**
 * @brief 使能中断引脚
 * @param pin 中断引脚
 * @param enable 使能标志
 */
void QMI8658_EnableINT(QMI8658_IntPin_t pin, bool enable);

/**
 * @brief 使能数据就绪中断
 * @param enable 使能标志
 */
void QMI8658_EnableDataReadyINT(bool enable);

/**
 * @brief 获取中断状态
 * @return 中断状态寄存器值
 */
uint8_t QMI8658_GetIrqStatus(void);

/**
 * @brief 获取状态寄存器
 * @return 状态寄存器值
 */
int QMI8658_GetStatusRegister(void);

// === 电源管理接口函数 ===

/**
 * @brief 传感器掉电
 */
void QMI8658_PowerDown(void);

/**
 * @brief 传感器上电
 */
void QMI8658_PowerOn(void);

// === 调试和信息接口函数 ===

/**
 * @brief 打印传感器状态信息
 */
void QMI8658_PrintStatus(void);

/**
 * @brief 转储控制寄存器
 */
void QMI8658_DumpCtrlRegister(void);

/**
 * @brief 获取芯片USID
 * @param buffer 缓冲区指针
 * @param length 缓冲区长度
 */
void QMI8658_GetChipUsid(uint8_t *buffer, uint8_t length);

/**
 * @brief 获取芯片固件版本
 * @return 固件版本
 */
uint32_t QMI8658_GetChipFirmwareVersion(void);

// === 内部命令接口函数 ===

/**
 * @brief 写入命令到CTRL9寄存器
 * @param cmd 命令
 * @param wait_ms 等待超时时间（毫秒）
 * @return 0: 成功, 非0: 失败
 */
uint8_t QMI8658_WriteCommand(QMI8658_CommandTable_t cmd, uint32_t wait_ms);

#ifdef __cplusplus
}
#endif

#endif // QMI8658_BSP_H 