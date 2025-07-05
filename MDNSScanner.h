#pragma once
#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ESPmDNS.h>
#include <vector>
#include <functional>

// 通用设备信息结构体
struct MDNSDeviceInfo {
    String hostname;        // 设备主机名
    String ip;             // 设备IP地址
    String name;           // 设备显示名称
    String serviceType;    // 服务类型 (http, https, ftp, etc.)
    uint16_t port;         // 服务端口
    bool isValid;          // 设备是否有效
    String customInfo;     // 自定义信息
};

// 扫描配置结构体
struct MDNSScanConfig {
    String targetHostname;           // 目标主机名（可选）
    String serviceType;              // 服务类型（如 "http", "https", "ftp"）
    String protocol;                 // 协议（如 "tcp", "udp"）
    std::vector<String> keywords;    // 关键词过滤
    uint16_t timeout;                // 超时时间（毫秒）
    bool validateEndpoint;           // 是否验证端点
    String validationPath;           // 验证路径（如 "/metrics", "/status"）
    
    // 构造函数设置默认值
    MDNSScanConfig() : 
        serviceType("http"), 
        protocol("tcp"), 
        timeout(500), 
        validateEndpoint(false),
        validationPath("/") {}
};

// 自定义验证函数类型
typedef std::function<bool(const MDNSDeviceInfo&, bool)> ValidationFunction;

class UniversalMDNSScanner {
public:
    // 使用配置扫描设备
    static std::vector<MDNSDeviceInfo> scanDevices(const MDNSScanConfig& config, bool printLog = false);
    
    // 查找特定主机名的设备
    static bool findDeviceByHostname(const String& hostname, MDNSDeviceInfo& outDevice, bool printLog = false);
    
    // 查找特定服务类型的所有设备
    static std::vector<MDNSDeviceInfo> findDevicesByService(const String& serviceType, const String& protocol = "tcp", bool printLog = false);
    
    // 使用关键词过滤设备
    static std::vector<MDNSDeviceInfo> findDevicesByKeywords(const std::vector<String>& keywords, bool printLog = false);
    
    // 验证设备是否有效（使用自定义验证函数）
    static bool validateDevice(const MDNSDeviceInfo& device, ValidationFunction validator, bool printLog = false);
    
    // 获取网络中所有mDNS设备
    static std::vector<MDNSDeviceInfo> getAllMDNSDevices(bool printLog = false);
    
    // 测试HTTP端点
    static bool testHTTPEndpoint(const String& ip, uint16_t port, const String& path, bool printLog = false);
    
    // 测试HTTPS端点
    static bool testHTTPSEndpoint(const String& ip, uint16_t port, const String& path, bool printLog = false);
    
private:
    // 初始化mDNS
    static bool initMDNS(const String& hostName = "esp32_scanner");
    
    // 解析主机名到IP
    static bool resolveHostname(const String& hostname, String& outIP, bool printLog = false);
    
    // 通过mDNS查找设备
    static bool findDeviceByMDNS(const String& hostname, String& outIP, bool printLog = false);
    
    // 浏览服务
    static std::vector<MDNSDeviceInfo> browseService(const String& serviceType, const String& protocol, bool printLog = false);
    
    // 过滤设备
    static bool filterDevice(const MDNSDeviceInfo& device, const MDNSScanConfig& config);
    
    // 验证HTTP端点
    static bool validateHTTPEndpoint(const MDNSDeviceInfo& device, const String& path, bool printLog = false);
    
    // 验证HTTPS端点
    static bool validateHTTPSEndpoint(const MDNSDeviceInfo& device, const String& path, bool printLog = false);
}; 