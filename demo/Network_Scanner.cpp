#include "Network_Scanner.h"

bool NetworkScanner::findMetricsServer(String& outUrl, bool printLog) {
    if (WiFi.status() != WL_CONNECTED) {
        if(printLog) ; // printf("[Scanner] WiFi not connected\n");
        return false;
    }

    if(printLog) ; // printf("[Scanner] Starting mDNS scan for cp02 device...\n");
    
    // 首先尝试使用mDNS查找cp02设备
    String deviceIP;
    if (findDeviceByMDNS("cp02", deviceIP, printLog)) {
        if(printLog) ; // printf("[Scanner] Found cp02 via mDNS at IP: %s\n", deviceIP.c_str());
        
        // 验证设备是否提供metrics端点
        if (testMetricsEndpoint(deviceIP, printLog)) {
            outUrl = deviceIP;
            if(printLog) ; // printf("[Scanner] cp02 metrics server confirmed at: %s\n", outUrl.c_str());
            return true;
        } else {
            if(printLog) ; // printf("[Scanner] cp02 found but no metrics endpoint available\n");
        }
    }
    
    if(printLog) ; // printf("[Scanner] mDNS scan failed, no cp02 device found\n");
    
    // 只使用mDNS扫描，不进行IP扫描
    return false;
}

// 新增：扫描所有cp02设备
std::vector<DeviceInfo> NetworkScanner::scanAllCP02Devices(bool printLog) {
    std::vector<DeviceInfo> devices;
    
    if (WiFi.status() != WL_CONNECTED) {
        if(printLog) ; // printf("[Scanner] WiFi not connected\n");
        return devices;
    }
    
    if(printLog) ; // printf("[Scanner] Starting comprehensive mDNS scan for all cp02 devices...\n");
    
    // 仅使用mDNS扫描
    devices = scanCP02ByMDNS(printLog);
    
    if(printLog) ; // printf("[Scanner] Found %d cp02 devices total\n", devices.size());
    
    return devices;
}

// 新增：仅使用mDNS扫描cp02设备
std::vector<DeviceInfo> NetworkScanner::scanCP02ByMDNS(bool printLog) {
    std::vector<DeviceInfo> devices;
    
    if(printLog) ; // printf("[Scanner] Starting mDNS-only scan for cp02 devices...\n");
    
    // 确保mDNS已初始化
    if (!MDNS.begin("esp32_power_monitor")) {
        if(printLog) ; // printf("[Scanner] Failed to start mDNS\n");
        return devices;
    }
    
    // 方法1：直接查找cp02主机名
    String deviceIP;
    if (findDeviceByMDNS("cp02", deviceIP, printLog)) {
        DeviceInfo device;
        device.hostname = "cp02";
        device.ip = deviceIP;
        device.name = "CP02 Power Monitor";
        device.hasMetrics = testMetricsEndpoint(deviceIP, printLog);
        
        if (device.hasMetrics) {
            devices.push_back(device);
            if(printLog) ; // printf("[Scanner] Added cp02 device: %s (%s)\n", device.name.c_str(), device.ip.c_str());
        }
    }
    
    // 方法2：浏览HTTP服务来查找更多cp02设备
    if(printLog) ; // printf("[Scanner] Browsing for HTTP services to find more cp02 devices...\n");
    
    int n = MDNS.queryService("http", "tcp");
    if (n > 0) {
        if(printLog) ; // printf("[Scanner] Found %d HTTP services\n", n);
        
        for (int i = 0; i < n; i++) {
            String serviceName = MDNS.hostname(i);
            IPAddress serviceIP = MDNS.address(i);
            
            if(printLog) ; // printf("[Scanner] Checking HTTP service: %s at %s\n", serviceName.c_str(), serviceIP.toString().c_str());
            
            // 检查服务名是否包含cp02相关信息
            if (serviceName.indexOf("cp02") >= 0 || serviceName.indexOf("CP02") >= 0) {
                // 检查是否已经添加过这个IP
                bool alreadyExists = false;
                for (const auto& existingDevice : devices) {
                    if (existingDevice.ip == serviceIP.toString()) {
                        alreadyExists = true;
                        break;
                    }
                }
                
                if (!alreadyExists) {
                    DeviceInfo device;
                    device.hostname = serviceName;
                    device.ip = serviceIP.toString();
                    device.name = "CP02 Device (" + serviceName + ")";
                    device.hasMetrics = testMetricsEndpoint(device.ip, printLog);
                    
                    if (device.hasMetrics) {
                        devices.push_back(device);
                        if(printLog) ; // printf("[Scanner] Added cp02 service: %s (%s)\n", device.name.c_str(), device.ip.c_str());
                    }
                }
            }
        }
    }
    
    // 方法3：查找通用的power monitor相关服务
    if(printLog) ; // printf("[Scanner] Looking for power monitor related services...\n");
    
    // 常见的cp02设备可能使用的服务名称
    const char* commonNames[] = {"power", "monitor", "metrics", "energy"};
    
    for (const char* name : commonNames) {
        if(printLog) ; // printf("[Scanner] Checking for services containing '%s'...\n", name);
        
        for (int i = 0; i < n; i++) {
            String serviceName = MDNS.hostname(i);
            IPAddress serviceIP = MDNS.address(i);
            
            if (serviceName.indexOf(name) >= 0) {
                // 检查是否已经添加过这个IP
                bool alreadyExists = false;
                for (const auto& existingDevice : devices) {
                    if (existingDevice.ip == serviceIP.toString()) {
                        alreadyExists = true;
                        break;
                    }
                }
                
                if (!alreadyExists) {
                    // 测试是否有metrics端点
                    if (testMetricsEndpoint(serviceIP.toString(), printLog)) {
                        DeviceInfo device;
                        device.hostname = serviceName;
                        device.ip = serviceIP.toString();
                        device.name = "Power Monitor (" + serviceName + ")";
                        device.hasMetrics = true;
                        
                        devices.push_back(device);
                        if(printLog) ; // printf("[Scanner] Added power monitor service: %s (%s)\n", device.name.c_str(), device.ip.c_str());
                    }
                }
            }
        }
    }
    
    if(printLog) ; // printf("[Scanner] mDNS scan completed, found %d valid cp02 devices\n", devices.size());
    
    return devices;
}

// 新增：验证设备是否有效
bool NetworkScanner::validateDevice(const DeviceInfo& device, bool printLog) {
    if(printLog) ; // printf("[Scanner] Validating device: %s (%s)\n", device.name.c_str(), device.ip.c_str());
    
    return testMetricsEndpoint(device.ip, printLog);
}

bool NetworkScanner::testMetricsEndpoint(const String& ip, bool printLog) {
    if(printLog) ; // printf("[Scanner] Testing IP: %s\n", ip.c_str());
    
    HTTPClient http;
    String url = "http://" + ip + "/metrics";
    
    http.begin(url);
    http.setTimeout(500); // 稍微增加超时时间以确保可靠性
    
    int httpCode = http.GET();
    bool success = (httpCode == HTTP_CODE_OK);
    
    if(printLog) {
        if(success) {
            ; // printf("[Scanner] Success: %s responded with metrics data\n", ip.c_str());
        } else {
            ; // printf("[Scanner] Failed: %s (HTTP code: %d)\n", ip.c_str(), httpCode);
        }
    }
    
    http.end();
    return success;
}

bool NetworkScanner::findDeviceByMDNS(const char* hostname, String& outIP, bool printLog) {
    if(printLog) ; // printf("[Scanner] Starting mDNS lookup for hostname: %s\n", hostname);
    
    // 确保mDNS已初始化
    if (!MDNS.begin("esp32_power_monitor")) {
        if(printLog) ; // printf("[Scanner] Failed to start mDNS\n");
        return false;
    }
    
    // 查找指定主机名的设备
    if(printLog) ; // printf("[Scanner] Querying mDNS for %s.local...\n", hostname);
    
    IPAddress resolvedIP = MDNS.queryHost(hostname);
    
    if (resolvedIP == INADDR_NONE) {
        if(printLog) ; // printf("[Scanner] Failed to resolve %s.local via mDNS\n", hostname);
        
        // 尝试浏览HTTP服务来查找设备
        if(printLog) ; // printf("[Scanner] Browsing for HTTP services...\n");
        
        int n = MDNS.queryService("http", "tcp");
        if (n > 0) {
            for (int i = 0; i < n; i++) {
                String serviceName = MDNS.hostname(i);
                if(printLog) ; // printf("[Scanner] Found HTTP service: %s\n", serviceName.c_str());
                
                // 检查服务名是否包含我们要查找的主机名
                if (serviceName.indexOf(hostname) >= 0) {
                    IPAddress serviceIP = MDNS.address(i);
                    outIP = serviceIP.toString();
                    if(printLog) ; // printf("[Scanner] Found %s via service discovery at IP: %s\n", hostname, outIP.c_str());
                    return true;
                }
            }
        }
        
        if(printLog) ; // printf("[Scanner] No matching services found\n");
        return false;
    }
    
    outIP = resolvedIP.toString();
    if(printLog) ; // printf("[Scanner] Successfully resolved %s.local to IP: %s\n", hostname, outIP.c_str());
    return true;
}

