#ifndef POWER_MONITOR_DATA_H
#define POWER_MONITOR_DATA_H

/**
 * @brief 端口数据结构
 */
struct PortData {
    int id;                 ///< 端口ID
    bool state;             ///< 端口状态
    int fc_protocol;        ///< 快充协议
    int current;            ///< 电流(mA)
    int voltage;            ///< 电压(mV)
    int power;              ///< 功率(mW)
    char protocol_name[16]; ///< 协议名称
    bool valid;             ///< 数据有效性
};

/**
 * @brief 系统监控数据结构
 */
struct SystemData {
    unsigned long boot_time;    ///< 启动时间
    int reset_reason;          ///< 重置原因
    unsigned long free_heap;   ///< 可用内存
    bool valid;                ///< 数据有效性
};

/**
 * @brief WiFi监控数据结构
 */
struct WiFiData {
    char ssid[32];      ///< WiFi名称
    char bssid[18];     ///< BSSID
    int channel;        ///< 频道
    int rssi;           ///< 信号强度
    bool valid;         ///< 数据有效性
};

/**
 * @brief 功率监控数据结构
 */
struct PowerMonitorData {
    PortData ports[4];      ///< 端口数据数组
    int total_power;        ///< 总功率(mW)
    int port_count;         ///< 端口数量
    SystemData system;      ///< 系统数据
    WiFiData wifi;          ///< WiFi数据
    unsigned long timestamp; ///< 数据时间戳
    bool valid;             ///< 数据有效性
};

// 功率监控数据回调函数类型
typedef void (*PowerDataCallback)(const PowerMonitorData& data, void* userData);

#endif // POWER_MONITOR_DATA_H 