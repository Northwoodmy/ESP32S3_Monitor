#pragma once
#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ESPmDNS.h>
#include <vector>

// 设备信息结构体
struct DeviceInfo {
    String hostname;    // 设备主机名
    String ip;         // 设备IP地址
    String name;       // 设备显示名称
    bool hasMetrics;   // 是否有metrics端点
};

class NetworkScanner {
public:
    // 使用mDNS扫描网络并返回找到的cp02设备URL
    static bool findMetricsServer(String& outUrl, bool printLog = false);
    
    // 扫描所有cp02设备并返回设备列表（仅使用mDNS，不使用IP扫描）
    static std::vector<DeviceInfo> scanAllCP02Devices(bool printLog = false);
    
    // 验证指定设备是否有效
    static bool validateDevice(const DeviceInfo& device, bool printLog = false);
    
private:
    static bool testMetricsEndpoint(const String& ip, bool printLog);
    static bool findDeviceByMDNS(const char* hostname, String& outIP, bool printLog);
    
    // 仅使用mDNS扫描cp02设备
    static std::vector<DeviceInfo> scanCP02ByMDNS(bool printLog);
}; 