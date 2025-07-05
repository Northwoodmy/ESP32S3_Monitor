#include "MDNSScanner.h"

// 使用关键词过滤设备 - 完全基于demo的实现
std::vector<MDNSDeviceInfo> UniversalMDNSScanner::findDevicesByKeywords(const std::vector<String>& keywords, bool printLog) {
    std::vector<MDNSDeviceInfo> devices;
    
    if (WiFi.status() != WL_CONNECTED) {
        if (printLog) printf("[mDNS Scanner] WiFi not connected\n");
        return devices;
    }
    
    if (printLog) printf("[mDNS Scanner] Starting comprehensive mDNS scan for cp02 devices\n");
    
    // 确保mDNS已初始化
    if (!MDNS.begin("esp32_power_monitor")) {
        if (printLog) printf("[mDNS Scanner] Failed to start mDNS\n");
        return devices;
    }
    
    // 首先浏览HTTP服务，获取所有可用的服务信息
    if (printLog) printf("[mDNS Scanner] Browsing for HTTP services to find cp02 devices\n");
    
    int n = MDNS.queryService("http", "tcp");
    if (n > 0) {
        if (printLog) printf("[mDNS Scanner] Found %d HTTP services\n", n);
        
        for (int i = 0; i < n; i++) {
            String serviceName = MDNS.hostname(i);
            IPAddress serviceIP = MDNS.address(i);
            uint16_t servicePort = MDNS.port(i);
            
            if (printLog) printf("[mDNS Scanner] Checking HTTP service: %s at %s:%d\n", 
                               serviceName.c_str(), serviceIP.toString().c_str(), servicePort);
            
            // 检查服务名是否包含cp02相关信息（不区分大小写）
            String lowerServiceName = serviceName;
            lowerServiceName.toLowerCase();
            
            bool isCP02Service = false;
            for (const String& keyword : keywords) {
                String lowerKeyword = keyword;
                lowerKeyword.toLowerCase();
                
                if (lowerServiceName.indexOf(lowerKeyword) >= 0) {
                    isCP02Service = true;
                    break;
                }
            }
            
            if (isCP02Service) {
                // 检查是否已经添加过这个IP
                bool alreadyExists = false;
                for (const auto& existingDevice : devices) {
                    if (existingDevice.ip == serviceIP.toString()) {
                        alreadyExists = true;
                        break;
                    }
                }
                
                if (!alreadyExists) {
                    MDNSDeviceInfo device;
                    device.hostname = serviceName;
                    device.ip = serviceIP.toString();
                    device.name = serviceName;  // 使用实际的服务名称
                    device.serviceType = "http";
                    device.port = servicePort;
                    device.isValid = true;
                    device.customInfo = "Found via HTTP service discovery";
                    
                    devices.push_back(device);
                    if (printLog) printf("[mDNS Scanner] Added cp02 service: %s (%s:%d)\n", 
                                       device.name.c_str(), device.ip.c_str(), device.port);
                }
            }
        }
    }
    
    // 方法2：直接查找cp02主机名（仅当HTTP服务发现没有找到时）
    if (devices.empty()) {
        if (printLog) printf("[mDNS Scanner] No cp02 devices found via HTTP service discovery, trying direct mDNS lookup\n");
        
        for (const String& keyword : keywords) {
            String deviceIP;
            if (findDeviceByMDNS(keyword, deviceIP, printLog)) {
                // 检查是否已经添加过这个IP
                bool alreadyExists = false;
                for (const auto& existingDevice : devices) {
                    if (existingDevice.ip == deviceIP) {
                        alreadyExists = true;
                        break;
                    }
                }
                
                if (!alreadyExists) {
                    MDNSDeviceInfo device;
                    device.hostname = keyword;
                    device.ip = deviceIP;
                    device.name = keyword + " Power Monitor";  // 只有在直接查找时才使用通用名称
                    device.serviceType = "http";
                    device.port = 80;
                    device.isValid = true;
                    device.customInfo = "Found via direct mDNS lookup";
                    
                    devices.push_back(device);
                    if (printLog) printf("[mDNS Scanner] Added cp02 device via direct lookup: %s (%s)\n", device.name.c_str(), device.ip.c_str());
                }
            }
        }
    }
    
    // 方法3：查找通用的power monitor相关服务（仅当前面没有找到时）
    if (devices.empty()) {
        if (printLog) printf("[mDNS Scanner] Looking for power monitor related services\n");
        
        // 常见的cp02设备可能使用的服务名称
        const char* commonNames[] = {"power", "monitor", "metrics", "energy"};
        
        for (const char* name : commonNames) {
            if (printLog) printf("[mDNS Scanner] Checking for services containing '%s'\n", name);
            
            for (int i = 0; i < n; i++) {
                String serviceName = MDNS.hostname(i);
                IPAddress serviceIP = MDNS.address(i);
                uint16_t servicePort = MDNS.port(i);
                
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
                        // 检查是否包含cp02关键词
                        String lowerServiceName = serviceName;
                        lowerServiceName.toLowerCase();
                        
                        bool isCP02Service = false;
                        for (const String& keyword : keywords) {
                            String lowerKeyword = keyword;
                            lowerKeyword.toLowerCase();
                            
                            if (lowerServiceName.indexOf(lowerKeyword) >= 0) {
                                isCP02Service = true;
                                break;
                            }
                        }
                        
                        if (isCP02Service) {
                            MDNSDeviceInfo device;
                            device.hostname = serviceName;
                            device.ip = serviceIP.toString();
                            device.name = serviceName;  // 使用实际的服务名称
                            device.serviceType = "http";
                            device.port = servicePort;
                            device.isValid = true;
                            device.customInfo = "Found via power monitor service discovery";
                            
                            devices.push_back(device);
                            if (printLog) printf("[mDNS Scanner] Added power monitor service: %s (%s:%d)\n", 
                                               device.name.c_str(), device.ip.c_str(), device.port);
                        }
                    }
                }
            }
        }
    }
    
    if (printLog) printf("[mDNS Scanner] mDNS scan completed, found %d valid cp02 devices\n", devices.size());
    
    return devices;
}

// 通过mDNS查找设备 - 完全基于demo的实现
bool UniversalMDNSScanner::findDeviceByMDNS(const String& hostname, String& outIP, bool printLog) {
    if (printLog) printf("[mDNS Scanner] Starting mDNS lookup for hostname: %s\n", hostname.c_str());
    
    // 查找指定主机名的设备
    if (printLog) printf("[mDNS Scanner] Querying mDNS for %s.local\n", hostname.c_str());
    
    IPAddress resolvedIP = MDNS.queryHost(hostname);
    
    if (resolvedIP == INADDR_NONE) {
        if (printLog) printf("[mDNS Scanner] Failed to resolve %s.local via mDNS\n", hostname.c_str());
        
        // 尝试浏览HTTP服务来查找设备
        if (printLog) printf("[mDNS Scanner] Browsing for HTTP services\n");
        
        int n = MDNS.queryService("http", "tcp");
        if (n > 0) {
            for (int i = 0; i < n; i++) {
                String serviceName = MDNS.hostname(i);
                if (printLog) printf("[mDNS Scanner] Found HTTP service: %s\n", serviceName.c_str());
                
                // 检查服务名是否包含我们要查找的主机名（忽略大小写）
                String lowerServiceName = serviceName;
                lowerServiceName.toLowerCase();
                String lowerHostname = hostname;
                lowerHostname.toLowerCase();
                
                if (lowerServiceName.indexOf(lowerHostname) >= 0) {
                    IPAddress serviceIP = MDNS.address(i);
                    outIP = serviceIP.toString();
                    if (printLog) printf("[mDNS Scanner] Found %s via service discovery at IP: %s\n", hostname.c_str(), outIP.c_str());
                    return true;
                }
            }
        }
        
        if (printLog) printf("[mDNS Scanner] No matching services found\n");
        return false;
    }
    
    outIP = resolvedIP.toString();
    if (printLog) printf("[mDNS Scanner] Successfully resolved %s.local to IP: %s\n", hostname.c_str(), outIP.c_str());
    return true;
}

// 查找特定主机名的设备
bool UniversalMDNSScanner::findDeviceByHostname(const String& hostname, MDNSDeviceInfo& outDevice, bool printLog) {
    if (printLog) printf("[mDNS Scanner] Looking for hostname: %s\n", hostname.c_str());
    
    if (!MDNS.begin("esp32_power_monitor")) {
        if (printLog) printf("[mDNS Scanner] Failed to start mDNS\n");
        return false;
    }
    
    String deviceIP;
    if (findDeviceByMDNS(hostname, deviceIP, printLog)) {
        outDevice.hostname = hostname;
        outDevice.ip = deviceIP;
        outDevice.name = hostname;
        outDevice.serviceType = "http";
        outDevice.port = 80;
        outDevice.isValid = true;
        outDevice.customInfo = "Resolved via mDNS";
        
        if (printLog) printf("[mDNS Scanner] Found device: %s at %s\n", hostname.c_str(), deviceIP.c_str());
        return true;
    }
    
    return false;
}

// 初始化mDNS
bool UniversalMDNSScanner::initMDNS(const String& hostName) {
    return MDNS.begin(hostName.c_str());
}

// 解析主机名到IP
bool UniversalMDNSScanner::resolveHostname(const String& hostname, String& outIP, bool printLog) {
    if (printLog) printf("[mDNS Scanner] Resolving hostname: %s\n", hostname.c_str());
    
    IPAddress resolvedIP = MDNS.queryHost(hostname);
    
    if (resolvedIP == INADDR_NONE) {
        if (printLog) printf("[mDNS Scanner] Failed to resolve %s.local\n", hostname.c_str());
        return false;
    }
    
    outIP = resolvedIP.toString();
    if (printLog) printf("[mDNS Scanner] Resolved %s to %s\n", hostname.c_str(), outIP.c_str());
    
    return true;
}

// 使用配置扫描设备
std::vector<MDNSDeviceInfo> UniversalMDNSScanner::scanDevices(const MDNSScanConfig& config, bool printLog) {
    std::vector<MDNSDeviceInfo> devices;
    
    if (WiFi.status() != WL_CONNECTED) {
        if (printLog) printf("[mDNS Scanner] WiFi not connected\n");
        return devices;
    }
    
    // 如果指定了目标主机名，优先查找
    if (!config.targetHostname.isEmpty()) {
        if (printLog) printf("[mDNS Scanner] Looking for specific hostname: %s\n", config.targetHostname.c_str());
        
        MDNSDeviceInfo device;
        if (findDeviceByHostname(config.targetHostname, device, printLog)) {
            devices.push_back(device);
        }
    }
    
    // 如果有关键词，使用关键词搜索
    if (!config.keywords.empty()) {
        std::vector<MDNSDeviceInfo> keywordDevices = findDevicesByKeywords(config.keywords, printLog);
        // 避免重复添加
        for (const auto& keywordDevice : keywordDevices) {
            bool exists = false;
            for (const auto& existingDevice : devices) {
                if (existingDevice.ip == keywordDevice.ip) {
                    exists = true;
                    break;
                }
            }
            if (!exists) {
                devices.push_back(keywordDevice);
            }
        }
    }
    
    if (printLog) printf("[mDNS Scanner] Scan completed, found %d valid devices\n", devices.size());
    
    return devices;
}

// 其他函数保持简化实现
std::vector<MDNSDeviceInfo> UniversalMDNSScanner::findDevicesByService(const String& serviceType, const String& protocol, bool printLog) {
    std::vector<MDNSDeviceInfo> devices;
    if (printLog) printf("[mDNS Scanner] Service discovery - use scanDevices or findDevicesByKeywords instead\n");
    return devices;
}

bool UniversalMDNSScanner::validateDevice(const MDNSDeviceInfo& device, ValidationFunction validator, bool printLog) {
    if (printLog) printf("[mDNS Scanner] Device validation simplified\n");
    return true;
}

std::vector<MDNSDeviceInfo> UniversalMDNSScanner::getAllMDNSDevices(bool printLog) {
    std::vector<MDNSDeviceInfo> devices;
    if (printLog) printf("[mDNS Scanner] Full device discovery - use findDevicesByKeywords instead\n");
    return devices;
}

bool UniversalMDNSScanner::testHTTPEndpoint(const String& ip, uint16_t port, const String& path, bool printLog) {
    if (printLog) printf("[mDNS Scanner] HTTP endpoint testing simplified\n");
    return true;
}

bool UniversalMDNSScanner::testHTTPSEndpoint(const String& ip, uint16_t port, const String& path, bool printLog) {
    if (printLog) printf("[mDNS Scanner] HTTPS endpoint testing simplified\n");
    return true;
}

std::vector<MDNSDeviceInfo> UniversalMDNSScanner::browseService(const String& serviceType, const String& protocol, bool printLog) {
    std::vector<MDNSDeviceInfo> devices;
    if (printLog) printf("[mDNS Scanner] Service browsing simplified\n");
    return devices;
}

bool UniversalMDNSScanner::filterDevice(const MDNSDeviceInfo& device, const MDNSScanConfig& config) {
    return true;
}

bool UniversalMDNSScanner::validateHTTPEndpoint(const MDNSDeviceInfo& device, const String& path, bool printLog) {
    if (printLog) printf("[mDNS Scanner] HTTP endpoint validation simplified\n");
    return true;
}

bool UniversalMDNSScanner::validateHTTPSEndpoint(const MDNSDeviceInfo& device, const String& path, bool printLog) {
    if (printLog) printf("[mDNS Scanner] HTTPS endpoint validation simplified\n");
    return true;
} 