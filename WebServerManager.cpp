/*
 * WebServerManager.cpp - Web服务器管理器类实现文件
 * ESP32S3监控项目 - Web服务器模块
 */

#include "WebServerManager.h"
#include "Version.h"
#include "PSRAMManager.h"
#include "DisplayManager.h"
#include "WeatherManager.h"
#include "LocationManager.h"
#include "Arduino.h"
#include <HTTPClient.h>
#include <WiFiClient.h>
#include "MDNSScanner.h"

WebServerManager::WebServerManager(WiFiManager* wifiMgr, ConfigStorage* configStore, OTAManager* otaMgr, FileManager* fileMgr) :
    server(nullptr),
    wifiManager(wifiMgr),
    configStorage(configStore),
    otaManager(otaMgr),
    fileManager(fileMgr),
    m_psramManager(nullptr),
    m_displayManager(nullptr),
    m_weatherManager(nullptr),
    m_locationManager(nullptr),
    serverTaskHandle(nullptr),
    isRunning(false) {
    server = new WebServer(80);
}

WebServerManager::~WebServerManager() {
    stop();
    if (server) {
        delete server;
        server = nullptr;
    }
}

void WebServerManager::init() {
    printf("初始化Web服务器...\n");
    
    if (!server) {
        printf("Web服务器初始化失败\n");
        return;
    }
    
    // 设置路由处理函数
    server->on("/", [this]() { handleRoot(); });
    server->on("/config", [this]() { handleWiFiConfig(); });
    server->on("/ota", [this]() { handleOTAPage(); });
    server->on("/scan", [this]() { handleWiFiScan(); });
    server->on("/info", [this]() { handleSystemInfo(); });
    server->on("/restart", [this]() { handleRestart(); });
    server->on("/reset", [this]() { handleResetConfig(); });
    server->on("/api", [this]() { handleAPI(); });
    server->on("/save", HTTP_POST, [this]() { handleSaveWiFi(); });
    server->on("/status", [this]() { handleGetStatus(); });
    server->on("/wifi-configs", HTTP_GET, [this]() { handleGetWiFiConfigs(); });
    server->on("/delete-wifi-config", HTTP_POST, [this]() { handleDeleteWiFiConfig(); });
    server->on("/connect-wifi", HTTP_POST, [this]() { handleConnectWiFiConfig(); });
    server->on("/update-wifi-priority", HTTP_POST, [this]() { handleUpdateWiFiPriority(); });
    server->on("/set-wifi-priorities", HTTP_POST, [this]() { handleSetWiFiPriorities(); });
    server->on("/ota-upload", HTTP_POST, [this]() { 
        // POST请求完成后的响应处理
        printf("OTA上传POST请求完成，发送响应\n");
        server->send(200, "application/json", otaManager->getStatusJSON());
    }, [this]() { handleOTAUpload(); });
    server->on("/ota-status", HTTP_GET, [this]() { handleOTAStatus(); });
    server->on("/ota-reboot", HTTP_POST, [this]() { handleOTAReboot(); });
    
    // 服务器OTA升级相关路由
    server->on("/api/ota/server-start", HTTP_POST, [this]() { handleServerOTAStart(); });
    server->on("/api/ota/server-status", HTTP_GET, [this]() { handleServerOTAStatus(); });
    server->on("/api/ota/firmware-list", HTTP_GET, [this]() { handleServerFirmwareList(); });
    server->on("/api/ota/firmware-version", HTTP_GET, [this]() { handleServerFirmwareVersion(); });
    
    // 文件管理路由
    server->on("/files", HTTP_GET, [this]() { handleFileManager(); });
    server->on("/api/files", HTTP_GET, [this]() { handleFileList(); });
    server->on("/api/files/upload", HTTP_POST, [this]() { 
        printf("文件上传POST请求完成，发送响应\n");
        server->send(200, "application/json", "{\"success\":true,\"message\":\"文件上传成功\"}");
    }, [this]() { handleFileUpload(); });
    server->on("/api/files/download", HTTP_GET, [this]() { handleFileDownload(); });
    server->on("/api/files/delete", HTTP_DELETE, [this]() { handleFileDelete(); });
    server->on("/api/files/rename", HTTP_POST, [this]() { handleFileRename(); });
    server->on("/api/files/create", HTTP_POST, [this]() { handleFileCreate(); });
    server->on("/api/filesystem/status", HTTP_GET, [this]() { handleFileSystemStatus(); });
    server->on("/api/filesystem/format", HTTP_POST, [this]() { handleFileSystemFormat(); });
    server->on("/api/filesystem/format-status", HTTP_GET, [this]() { handleFileSystemFormatStatus(); });
    
    // 屏幕配置路由
    server->on("/api/screen/config", HTTP_GET, [this]() { handleScreenConfig(); });
    server->on("/api/screen/brightness", HTTP_POST, [this]() { handleSetBrightness(); });
    server->on("/api/screen/test", HTTP_POST, [this]() { handleScreenTest(); });
    
    // 系统设置路由
    server->on("/settings", [this]() { handleSystemSettings(); });
    server->on("/api/time/config", HTTP_GET, [this]() { handleGetTimeConfig(); });
    server->on("/api/time/config", HTTP_POST, [this]() { handleSetTimeConfig(); });
    server->on("/api/time/sync", HTTP_POST, [this]() { handleSyncTime(); });
    
    // 天气设置路由
    server->on("/weather-settings", [this]() { handleWeatherSettings(); });
    server->on("/api/weather/config", HTTP_GET, [this]() { handleGetWeatherConfig(); });
    server->on("/api/weather/api-key", HTTP_POST, [this]() { handleSetWeatherApiKey(); });
    server->on("/api/weather/city", HTTP_POST, [this]() { handleSetWeatherCity(); });
    server->on("/api/weather/update-config", HTTP_POST, [this]() { handleSetWeatherUpdateConfig(); });
    server->on("/api/weather/current", HTTP_GET, [this]() { handleGetCurrentWeather(); });
    server->on("/api/weather/stats", HTTP_GET, [this]() { handleGetWeatherStats(); });
    server->on("/api/weather/test", HTTP_POST, [this]() { handleTestWeatherApi(); });
    server->on("/api/weather/update", HTTP_POST, [this]() { handleUpdateWeatherNow(); });
    
    // 定位相关路由
    server->on("/api/location/data", HTTP_GET, [this]() { handleGetLocationData(); });
    server->on("/api/location/api-key", HTTP_POST, [this]() { handleSetLocationApiKey(); });
    server->on("/api/location/locate", HTTP_POST, [this]() { handleLocationNow(); });
    
    // 屏幕设置路由
    server->on("/screen-settings", [this]() { handleScreenSettings(); });
    server->on("/api/screen/settings", HTTP_GET, [this]() { handleGetScreenSettings(); });
    server->on("/api/screen/settings", HTTP_POST, [this]() { handleSetScreenSettings(); });
    server->on("/api/screen/rotation", HTTP_GET, [this]() { handleGetCurrentRotation(); });
    
    // 主题设置路由
    server->on("/api/theme/settings", HTTP_GET, [this]() { handleGetThemeSettings(); });
    server->on("/api/theme/settings", HTTP_POST, [this]() { handleSetThemeSettings(); });
    
    // 服务器设置路由
    server->on("/server-settings", [this]() { handleServerSettingsPage(); });
    server->on("/api/server/config", HTTP_GET, [this]() { handleGetServerConfig(); });
    server->on("/api/server/config", HTTP_POST, [this]() { handleSetServerConfig(); });
    server->on("/api/server/test", HTTP_POST, [this]() { handleTestServerConnection(); });
    server->on("/api/server/data", HTTP_GET, [this]() { handleGetServerData(); });
    server->on("/api/server/mdns-scan", HTTP_GET, [this]() { handleMDNSScanServers(); });
    
    server->onNotFound([this]() { handleNotFound(); });
    
    printf("Web服务器路由配置完成\n");
}

void WebServerManager::setPSRAMManager(PSRAMManager* psramManager) {
    m_psramManager = psramManager;
}

void WebServerManager::setDisplayManager(DisplayManager* displayManager) {
    m_displayManager = displayManager;
}

void WebServerManager::setWeatherManager(WeatherManager* weatherManager) {
    m_weatherManager = weatherManager;
}

void WebServerManager::setLocationManager(LocationManager* locationManager) {
    m_locationManager = locationManager;
}

void WebServerManager::start() {
    if (isRunning) {
        printf("Web服务器已经在运行中\n");
        return;
    }
    
    printf("启动Web服务器...\n");
    
    server->begin();
    isRunning = true;
    
    // 使用SRAM栈创建Web服务器任务
    printf("使用SRAM栈创建Web服务器任务\n");
    BaseType_t result = xTaskCreatePinnedToCore(
        serverTask,
        "WebServerTask",
        4096,
        this,
        2,
        &serverTaskHandle,
        0                   // 运行在核心0
    );
    
    if (result == pdPASS) {
        printf("Web服务器任务(SRAM栈)创建成功\n");
        printf("Web服务器启动成功，端口：80\n");
    } else {
        isRunning = false;
        printf("Web服务器任务(SRAM栈)创建失败\n");
    }
}

void WebServerManager::stop() {
    if (!isRunning) {
        return;
    }
    
    printf("停止Web服务器...\n");
    
    if (serverTaskHandle != nullptr) {
        vTaskDelete(serverTaskHandle);
        serverTaskHandle = nullptr;
    }
    
    server->stop();
    isRunning = false;
    
    printf("Web服务器已停止\n");
}

void WebServerManager::handleClient() {
    if (server && isRunning) {
        server->handleClient();
    }
}

void WebServerManager::serverTask(void* parameter) {
    WebServerManager* manager = static_cast<WebServerManager*>(parameter);
    
    printf("Web服务器任务开始运行\n");
    
    while (manager->isRunning) {
        manager->handleClient();
        vTaskDelay(pdMS_TO_TICKS(10)); // 10ms延时
    }
    
    printf("Web服务器任务结束\n");
    vTaskDelete(nullptr);
}

void WebServerManager::handleRoot() {
    printf("处理首页请求\n");
    server->send(200, "text/html", getIndexHTML());
}

void WebServerManager::handleWiFiConfig() {
    printf("处理WiFi配置页面请求\n");
    server->send(200, "text/html", getIndexHTML());
}

void WebServerManager::handleOTAPage() {
    printf("处理OTA页面请求\n");
    server->send(200, "text/html", getOTAPageHTML());
}

void WebServerManager::handleWiFiScan() {
    printf("处理WiFi扫描请求\n");
    
    DynamicJsonDocument doc(2048);
    JsonArray networks = doc.createNestedArray("networks");
    
    int n = WiFi.scanNetworks();
    printf("扫描到 %d 个WiFi网络\n", n);
    
    for (int i = 0; i < n; i++) {
        JsonObject network = networks.createNestedObject();
        network["ssid"] = WiFi.SSID(i);
        network["rssi"] = WiFi.RSSI(i);
        network["secure"] = (WiFi.encryptionType(i) != WIFI_AUTH_OPEN);
    }
    
    String response;
    serializeJson(doc, response);
    server->send(200, "application/json", response);
}

void WebServerManager::handleSystemInfo() {
    printf("处理系统信息请求\n");
    
    DynamicJsonDocument doc(1024);
    doc["device"] = "ESP32S3 Monitor";
            doc["version"] = VERSION_STRING;
    doc["chipModel"] = ESP.getChipModel();
    doc["chipRevision"] = ESP.getChipRevision();
    doc["cpuFreq"] = ESP.getCpuFreqMHz();
    doc["flashSize"] = ESP.getFlashChipSize();
    doc["uptime"] = millis();
    
    // SRAM内存信息（已使用/总容量格式）
    size_t totalSRAM = ESP.getHeapSize();
    size_t freeSRAM = ESP.getFreeHeap();
    size_t usedSRAM = totalSRAM - freeSRAM;
    doc["sramUsed"] = usedSRAM;
    doc["sramTotal"] = totalSRAM;
    doc["sramFree"] = freeSRAM;
    doc["sramUsagePercent"] = (float)usedSRAM / totalSRAM * 100.0;
    
    // PSRAM内存信息
    size_t totalPSRAM = ESP.getPsramSize();
    if (totalPSRAM > 0) {
        size_t freePSRAM = ESP.getFreePsram();
        size_t usedPSRAM = totalPSRAM - freePSRAM;
        doc["psramUsed"] = usedPSRAM;
        doc["psramTotal"] = totalPSRAM;
        doc["psramFree"] = freePSRAM;
        doc["psramUsagePercent"] = (float)usedPSRAM / totalPSRAM * 100.0;
        doc["psramAvailable"] = true;
    } else {
        doc["psramAvailable"] = false;
    }
    
    if (wifiManager->isConnected()) {
        doc["wifi"]["status"] = "connected";
        doc["wifi"]["ssid"] = WiFi.SSID();
        doc["wifi"]["ip"] = wifiManager->getLocalIP();
        doc["wifi"]["rssi"] = WiFi.RSSI();
    } else {
        doc["wifi"]["status"] = "ap_mode";
        doc["wifi"]["ip"] = wifiManager->getAPIP();
    }
    
    String response;
    serializeJson(doc, response);
    server->send(200, "application/json", response);
}

void WebServerManager::handleRestart() {
    printf("处理重启请求\n");
    server->send(200, "text/plain", "设备将在3秒后重启...");
    
    vTaskDelay(pdMS_TO_TICKS(3000));
    ESP.restart();
}

void WebServerManager::handleResetConfig() {
    printf("处理配置重置请求\n");
    
    DynamicJsonDocument doc(256);
    
    // 异步执行配置重置
    bool success = configStorage->resetAllConfigAsync(5000);
    
    doc["success"] = success;
    
    if (success) {
        doc["message"] = "配置已重置为默认值，设备将在3秒后重启";
        
        String response;
        serializeJson(doc, response);
        server->send(200, "application/json", response);
        
        // 延时3秒后重启设备
        vTaskDelay(pdMS_TO_TICKS(3000));
        ESP.restart();
    } else {
        doc["message"] = "配置重置失败";
        
        String response;
        serializeJson(doc, response);
        server->send(500, "application/json", response);
    }
}

void WebServerManager::handleNotFound() {
    printf("处理404请求: %s\n", server->uri().c_str());
    server->send(404, "text/plain", "页面未找到");
}

void WebServerManager::handleAPI() {
    printf("处理API请求\n");
    
    DynamicJsonDocument doc(512);
    doc["status"] = "ok";
    doc["message"] = "ESP32S3 Monitor API";
    doc["endpoints"] = "/scan, /info, /save, /status, /restart, /reset";
    
    String response;
    serializeJson(doc, response);
    server->send(200, "application/json", response);
}

void WebServerManager::handleSaveWiFi() {
    printf("处理WiFi保存请求\n");
    
    if (!server->hasArg("ssid") || !server->hasArg("password")) {
        server->send(400, "application/json", "{\"success\":false,\"message\":\"缺少SSID或密码参数\"}");
        return;
    }
    
    String ssid = server->arg("ssid");
    String password = server->arg("password");
    
    printf("保存WiFi配置: SSID=%s\n", ssid.c_str());
    
    // 尝试连接WiFi
    bool connected = wifiManager->connectToWiFi(ssid, password);
    
    DynamicJsonDocument doc(256);
    doc["success"] = connected;
    
    if (connected) {
        // 等待NVS刷新数据
        vTaskDelay(pdMS_TO_TICKS(100));
        
        // 异步检查配置是否已保存
        bool configSaved = configStorage->hasWiFiConfigAsync(3000);
        if (configSaved) {
            doc["message"] = "WiFi连接成功，配置已保存";
        } else {
            doc["message"] = "WiFi连接成功，但配置保存失败";
            printf("⚠ 警告: Web接口检测到配置保存失败\n");
        }
        doc["ip"] = wifiManager->getLocalIP();
    } else {
        doc["message"] = "WiFi连接失败";
    }
    
    String response;
    serializeJson(doc, response);
    server->send(200, "application/json", response);
}

void WebServerManager::handleGetStatus() {
    printf("处理状态请求\n");
    
    DynamicJsonDocument doc(512);
    
    if (wifiManager->isConnected()) {
        doc["wifi"]["connected"] = true;
        doc["wifi"]["ssid"] = WiFi.SSID();
        doc["wifi"]["ip"] = wifiManager->getLocalIP();
        doc["wifi"]["rssi"] = WiFi.RSSI();
    } else {
        doc["wifi"]["connected"] = false;
        doc["wifi"]["mode"] = "AP";
        doc["wifi"]["ip"] = wifiManager->getAPIP();
    }
    
    doc["system"]["freeHeap"] = ESP.getFreeHeap();
    doc["system"]["uptime"] = millis();
    
    String response;
    serializeJson(doc, response);
    server->send(200, "application/json", response);
}

void WebServerManager::handleGetWiFiConfigs() {
    printf("处理获取WiFi配置列表请求\n");
    
    // 添加缓存控制头
    server->sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
    server->sendHeader("Pragma", "no-cache");
    server->sendHeader("Expires", "0");
    
    DynamicJsonDocument doc(1024);
    JsonArray configs = doc.createNestedArray("configs");
    
    WiFiConfig wifiConfigs[3];
    if (configStorage->loadWiFiConfigsAsync(wifiConfigs, 3000)) {
        printf("成功加载WiFi配置，开始构建响应\n");
        for (int i = 0; i < ConfigStorage::MAX_WIFI_CONFIGS; i++) {
            if (wifiConfigs[i].isValid && wifiConfigs[i].ssid.length() > 0) {
                JsonObject config = configs.createNestedObject();
                config["index"] = i;
                config["ssid"] = wifiConfigs[i].ssid;
                config["password"] = "***"; // 不返回实际密码，只显示占位符
                config["priority"] = wifiConfigs[i].priority;  // 新增：返回优先级
                config["status"] = "saved";
                printf("添加配置 %d: %s (优先级 %d)\n", i, wifiConfigs[i].ssid.c_str(), wifiConfigs[i].priority);
            }
        }
    } else {
        printf("加载WiFi配置失败\n");
    }
    
    int configCount = configStorage->getWiFiConfigCountAsync(3000);
    doc["count"] = configCount;
    doc["maxConfigs"] = ConfigStorage::MAX_WIFI_CONFIGS;
    
    printf("配置数量: %d，最大配置数: %d\n", configCount, ConfigStorage::MAX_WIFI_CONFIGS);
    
    String response;
    serializeJson(doc, response);
    server->send(200, "application/json", response);
}

void WebServerManager::handleDeleteWiFiConfig() {
    printf("处理删除WiFi配置请求\n");
    
    if (!server->hasArg("index")) {
        printf("缺少index参数\n");
        server->send(400, "application/json", "{\"success\":false,\"message\":\"缺少index参数\"}");
        return;
    }
    
    int index = server->arg("index").toInt();
    printf("请求删除配置索引: %d\n", index);
    
    if (index < 0 || index >= ConfigStorage::MAX_WIFI_CONFIGS) {
        printf("无效的配置索引: %d\n", index);
        server->send(400, "application/json", "{\"success\":false,\"message\":\"无效的配置索引\"}");
        return;
    }
    
    DynamicJsonDocument doc(256);
    
    // 异步加载现有配置
    WiFiConfig configs[3];
    printf("开始异步加载WiFi配置...\n");
    if (configStorage->loadWiFiConfigsAsync(configs, 3000)) {
        printf("成功加载WiFi配置\n");
        
        // 打印所有配置状态
        for (int i = 0; i < ConfigStorage::MAX_WIFI_CONFIGS; i++) {
            printf("配置 %d: valid=%s, ssid=%s\n", i, 
                   configs[i].isValid ? "true" : "false", 
                   configs[i].ssid.c_str());
        }
        
        if (configs[index].isValid && configs[index].ssid.length() > 0) {
            printf("删除WiFi配置 %d: %s\n", index, configs[index].ssid.c_str());
            
            // 清除指定配置
            configs[index] = WiFiConfig();
            printf("已清除配置 %d\n", index);
            
            // 异步重新保存配置
            printf("开始异步保存更新后的配置...\n");
            bool success = configStorage->saveWiFiConfigsAsync(configs, 5000);
            printf("配置保存结果: %s\n", success ? "成功" : "失败");
            
            // 等待NVS数据刷新
            if (success) {
                vTaskDelay(pdMS_TO_TICKS(100));
                printf("等待NVS数据刷新完成\n");
            }
            
            doc["success"] = success;
            if (success) {
                doc["message"] = "WiFi配置删除成功";
                printf("删除操作成功完成\n");
            } else {
                doc["message"] = "WiFi配置删除失败";
                printf("删除操作失败\n");
            }
        } else {
            printf("指定的配置 %d 不存在或无效\n", index);
            doc["success"] = false;
            doc["message"] = "指定的配置不存在";
        }
    } else {
        printf("加载WiFi配置失败\n");
        doc["success"] = false;
        doc["message"] = "加载WiFi配置失败";
    }
    
    String response;
    serializeJson(doc, response);
    printf("返回删除结果: %s\n", response.c_str());
    server->send(200, "application/json", response);
}

void WebServerManager::handleConnectWiFiConfig() {
    printf("处理手动连接WiFi配置请求\n");
    
    if (!server->hasArg("index")) {
        server->send(400, "application/json", "{\"success\":false,\"message\":\"缺少index参数\"}");
        return;
    }
    
    int index = server->arg("index").toInt();
    
    if (index < 0 || index >= ConfigStorage::MAX_WIFI_CONFIGS) {
        server->send(400, "application/json", "{\"success\":false,\"message\":\"无效的配置索引\"}");
        return;
    }
    
    DynamicJsonDocument doc(256);
    
    // 异步加载WiFi配置
    WiFiConfig configs[3];
    if (configStorage->loadWiFiConfigsAsync(configs, 3000)) {
        if (configs[index].isValid && configs[index].ssid.length() > 0) {
            printf("手动连接WiFi配置 %d: %s\n", index, configs[index].ssid.c_str());
            
            // 尝试连接指定的WiFi配置
            bool connected = wifiManager->tryConnectToConfig(configs[index], 20);
            
            doc["success"] = connected;
            if (connected) {
                doc["message"] = "WiFi连接成功";
                doc["ssid"] = configs[index].ssid;
                doc["ip"] = wifiManager->getLocalIP();
            } else {
                doc["message"] = "WiFi连接失败，请检查网络状态";
            }
        } else {
            doc["success"] = false;
            doc["message"] = "指定的配置不存在或无效";
        }
    } else {
        doc["success"] = false;
        doc["message"] = "加载WiFi配置失败";
    }
    
    String response;
    serializeJson(doc, response);
    server->send(200, "application/json", response);
}

void WebServerManager::handleOTAUpload() {
    HTTPUpload& upload = server->upload();
    
    if (upload.status == UPLOAD_FILE_START) {
        printf("处理OTA固件上传请求\n");
        printf("开始OTA文件上传: %s\n", upload.filename.c_str());
        
        // ESP32的WebServer在START阶段totalSize通常为0，所以使用动态大小
        if (!otaManager->beginOTA(0)) {  // 使用0表示动态大小
            printf("OTA开始失败\n");
            return;
        }
        
    } else if (upload.status == UPLOAD_FILE_WRITE) {
        if (!otaManager->writeOTAData(upload.buf, upload.currentSize)) {
            printf("写入OTA数据失败\n");
            return;
        }
        
    } else if (upload.status == UPLOAD_FILE_END) {
        printf("OTA文件上传结束，总大小: %u 字节\n", upload.totalSize);
        
        // 在结束时更新实际的文件大小
        if (!otaManager->setActualSize(upload.totalSize)) {
            printf("设置实际文件大小失败\n");
            return;
        }
        
        if (otaManager->endOTA()) {
            printf("OTA升级成功完成\n");
        } else {
            printf("OTA升级失败\n");
        }
        
    } else if (upload.status == UPLOAD_FILE_ABORTED) {
        printf("OTA文件上传被中止\n");
        otaManager->abortOTA();
    }
    
    // 不在upload回调中发送响应，而是在POST处理函数中统一处理
}

void WebServerManager::handleOTAStatus() {
    String statusJson = otaManager->getStatusJSON();
    server->send(200, "application/json", statusJson);
}

void WebServerManager::handleOTAReboot() {
    printf("处理OTA重启请求\n");
    
    DynamicJsonDocument doc(256);
    
    if (otaManager->getStatus() == OTAStatus::SUCCESS) {
        doc["success"] = true;
        doc["message"] = "设备将在3秒后重启以应用新固件";
        
        String response;
        serializeJson(doc, response);
        server->send(200, "application/json", response);
        
        // 延时后重启设备
        otaManager->rebootDevice();
    } else {
        doc["success"] = false;
        doc["message"] = "OTA升级未成功，无法重启";
        
        String response;
        serializeJson(doc, response);
        server->send(400, "application/json", response);
    }
}

void WebServerManager::handleFileManager() {
    printf("处理文件管理页面请求\n");
    server->send(200, "text/html", getFileManagerHTML());
}

void WebServerManager::handleFileList() {
    printf("处理文件列表请求\n");
    
    String path = "/";
    if (server->hasArg("path")) {
        path = server->arg("path");
    }
    
    if (!fileManager->isReady()) {
        server->send(500, "application/json", "{\"success\":false,\"message\":\"文件系统未初始化\"}");
        return;
    }
    
    String response = fileManager->getFileListJSON(path);
    server->send(200, "application/json", response);
}

void WebServerManager::handleFileUpload() {
    HTTPUpload& upload = server->upload();
    static File uploadFile;
    static String uploadPath;
    
    if (upload.status == UPLOAD_FILE_START) {
        printf("开始文件上传: %s\n", upload.filename.c_str());
        
        if (!fileManager->isReady()) {
            printf("文件系统未初始化\n");
            return;
        }
        
        // 获取目标路径
        uploadPath = "/" + upload.filename;
        if (server->hasArg("path")) {
            String path = server->arg("path");
            if (!path.endsWith("/")) {
                path += "/";
            }
            uploadPath = path + upload.filename;
        }
        
        // 清理路径
        uploadPath = fileManager->sanitizePath(uploadPath);
        
        // 检查可用空间（粗略检查）
        if (fileManager->getFreeBytes() < 1024) { // 至少需要1KB空间
            printf("可用空间不足\n");
            return;
        }
        
        // 创建临时文件开始写入
        uploadFile = SPIFFS.open(uploadPath, FILE_WRITE);
        if (!uploadFile) {
            printf("无法创建文件: %s\n", uploadPath.c_str());
            return;
        }
        
        printf("开始写入文件: %s\n", uploadPath.c_str());
        
    } else if (upload.status == UPLOAD_FILE_WRITE) {
        // 分块写入文件数据
        if (uploadFile) {
            size_t written = uploadFile.write(upload.buf, upload.currentSize);
            if (written != upload.currentSize) {
                printf("写入数据失败，期望: %zu，实际: %zu\n", upload.currentSize, written);
                uploadFile.close();
                SPIFFS.remove(uploadPath); // 删除不完整的文件
                return;
            }
            printf("写入数据块: %zu 字节\n", written);
        }
        
    } else if (upload.status == UPLOAD_FILE_END) {
        printf("文件上传结束，总大小: %u 字节\n", upload.totalSize);
        
        if (uploadFile) {
            uploadFile.close();
            printf("文件上传成功: %s\n", uploadPath.c_str());
        }
        
    } else if (upload.status == UPLOAD_FILE_ABORTED) {
        printf("文件上传被中止\n");
        if (uploadFile) {
            uploadFile.close();
            SPIFFS.remove(uploadPath); // 删除不完整的文件
        }
    }
}

void WebServerManager::handleFileDownload() {
    printf("处理文件下载请求\n");
    
    if (!server->hasArg("path")) {
        server->send(400, "application/json", "{\"success\":false,\"message\":\"缺少文件路径参数\"}");
        return;
    }
    
    String filePath = server->arg("path");
    
    if (!fileManager->isReady()) {
        server->send(500, "application/json", "{\"success\":false,\"message\":\"文件系统未初始化\"}");
        return;
    }
    
    if (!fileManager->exists(filePath)) {
        server->send(404, "application/json", "{\"success\":false,\"message\":\"文件不存在\"}");
        return;
    }
    
    uint8_t* data = nullptr;
    size_t size = 0;
    
    if (fileManager->downloadFile(filePath, data, size)) {
        // 确定文件MIME类型
        String contentType = "application/octet-stream";
        if (filePath.endsWith(".txt")) {
            contentType = "text/plain";
        } else if (filePath.endsWith(".html")) {
            contentType = "text/html";
        } else if (filePath.endsWith(".css")) {
            contentType = "text/css";
        } else if (filePath.endsWith(".js")) {
            contentType = "application/javascript";
        } else if (filePath.endsWith(".json")) {
            contentType = "application/json";
        } else if (filePath.endsWith(".jpg") || filePath.endsWith(".jpeg")) {
            contentType = "image/jpeg";
        } else if (filePath.endsWith(".png")) {
            contentType = "image/png";
        } else if (filePath.endsWith(".gif")) {
            contentType = "image/gif";
        }
        
        // 设置下载文件名
        String fileName = filePath.substring(filePath.lastIndexOf("/") + 1);
        server->sendHeader("Content-Disposition", "attachment; filename=\"" + fileName + "\"");
        
        server->send_P(200, contentType.c_str(), (const char*)data, size);
        delete[] data;
        
        printf("文件下载成功: %s (%zu bytes)\n", filePath.c_str(), size);
    } else {
        server->send(500, "application/json", "{\"success\":false,\"message\":\"文件读取失败\"}");
        printf("文件下载失败: %s\n", filePath.c_str());
    }
}

void WebServerManager::handleFileDelete() {
    printf("处理文件删除请求\n");
    
    if (!server->hasArg("path")) {
        server->send(400, "application/json", "{\"success\":false,\"message\":\"缺少文件路径参数\"}");
        return;
    }
    
    String filePath = server->arg("path");
    
    if (!fileManager->isReady()) {
        server->send(500, "application/json", "{\"success\":false,\"message\":\"文件系统未初始化\"}");
        return;
    }
    
    DynamicJsonDocument doc(256);
    bool success = fileManager->deleteFile(filePath);
    
    doc["success"] = success;
    if (success) {
        doc["message"] = "文件删除成功";
        printf("文件删除成功: %s\n", filePath.c_str());
    } else {
        doc["message"] = "文件删除失败";
        printf("文件删除失败: %s\n", filePath.c_str());
    }
    
    String response;
    serializeJson(doc, response);
    server->send(200, "application/json", response);
}

void WebServerManager::handleFileRename() {
    printf("处理文件重命名请求\n");
    
    if (!server->hasArg("oldPath") || !server->hasArg("newPath")) {
        server->send(400, "application/json", "{\"success\":false,\"message\":\"缺少文件路径参数\"}");
        return;
    }
    
    String oldPath = server->arg("oldPath");
    String newPath = server->arg("newPath");
    
    if (!fileManager->isReady()) {
        server->send(500, "application/json", "{\"success\":false,\"message\":\"文件系统未初始化\"}");
        return;
    }
    
    DynamicJsonDocument doc(256);
    bool success = fileManager->renameFile(oldPath, newPath);
    
    doc["success"] = success;
    if (success) {
        doc["message"] = "文件重命名成功";
        printf("文件重命名成功: %s -> %s\n", oldPath.c_str(), newPath.c_str());
    } else {
        doc["message"] = "文件重命名失败";
        printf("文件重命名失败: %s -> %s\n", oldPath.c_str(), newPath.c_str());
    }
    
    String response;
    serializeJson(doc, response);
    server->send(200, "application/json", response);
}

void WebServerManager::handleFileCreate() {
    printf("处理文件创建请求\n");
    
    if (!server->hasArg("path")) {
        server->send(400, "application/json", "{\"success\":false,\"message\":\"缺少文件路径参数\"}");
        return;
    }
    
    String filePath = server->arg("path");
    String content = "";
    
    if (server->hasArg("content")) {
        content = server->arg("content");
    }
    
    if (!fileManager->isReady()) {
        server->send(500, "application/json", "{\"success\":false,\"message\":\"文件系统未初始化\"}");
        return;
    }
    
    DynamicJsonDocument doc(256);
    bool success = fileManager->writeFileFromString(filePath, content);
    
    doc["success"] = success;
    if (success) {
        doc["message"] = "文件创建成功";
        printf("文件创建成功: %s\n", filePath.c_str());
    } else {
        doc["message"] = "文件创建失败";
        printf("文件创建失败: %s\n", filePath.c_str());
    }
    
    String response;
    serializeJson(doc, response);
    server->send(200, "application/json", response);
}

void WebServerManager::handleFileSystemStatus() {
    printf("处理文件系统状态请求\n");
    
    String response = fileManager->getFileSystemStatusJSON();
    server->send(200, "application/json", response);
}

void WebServerManager::handleFileSystemFormat() {
    printf("处理文件系统格式化请求\n");
    
    if (!fileManager->isReady()) {
        server->send(500, "application/json", "{\"success\":false,\"message\":\"文件系统未初始化\"}");
        return;
    }
    
    // 检查是否已经在格式化中
    if (fileManager->isFormatting()) {
        server->send(409, "application/json", "{\"success\":false,\"message\":\"格式化操作正在进行中\"}");
        return;
    }
    
    DynamicJsonDocument doc(256);
    
    // 启动异步格式化任务
    bool started = fileManager->startFormatFileSystem();
    
    doc["success"] = started;
    if (started) {
        doc["message"] = "格式化任务已启动，请使用状态接口查询进度";
        doc["formatting"] = true;
    } else {
        doc["message"] = "格式化任务启动失败";
        doc["formatting"] = false;
    }
    
    String response;
    serializeJson(doc, response);
    server->send(200, "application/json", response);
}

void WebServerManager::handleFileSystemFormatStatus() {
    printf("处理格式化状态查询请求\n");
    
    DynamicJsonDocument doc(256);
    
    doc["formatting"] = fileManager->isFormatting();
    
    if (fileManager->isFormatting()) {
        doc["message"] = "格式化正在进行中，请稍候...";
        doc["completed"] = false;
    } else {
        doc["message"] = "格式化已完成";
        doc["completed"] = true;
        doc["success"] = fileManager->getFormatResult();
    }
    
        String response;
    serializeJson(doc, response);
    server->send(200, "application/json", response);
}

// 天气设置相关API实现
void WebServerManager::handleWeatherSettings() {
    printf("处理天气设置页面请求\n");
    server->send(200, "text/html", getWeatherSettingsHTML());
}

void WebServerManager::handleGetWeatherConfig() {
    printf("处理获取天气配置请求\n");
    
    DynamicJsonDocument doc(512);
    
    if (m_weatherManager) {
        WeatherConfig config = m_weatherManager->getConfig();
        doc["success"] = true;
        doc["config"]["apiKey"] = config.apiKey;
        doc["config"]["cityCode"] = config.cityCode;
        doc["config"]["cityName"] = config.cityName;
        doc["config"]["autoUpdate"] = config.autoUpdate;
        doc["config"]["updateInterval"] = config.updateInterval;
        doc["config"]["enableForecast"] = config.enableForecast;
        
        // 添加完整的位置信息
        if (configStorage) {
            String fullLocation = configStorage->getStringAsync("weather_fullLocation", "");
            if (!fullLocation.isEmpty()) {
                doc["config"]["fullLocation"] = fullLocation;
            }
        }
        
        doc["message"] = "天气配置获取成功";
    } else {
        doc["success"] = false;
        doc["message"] = "天气管理器未初始化";
    }
    
    String response;
    serializeJson(doc, response);
    server->send(200, "application/json", response);
}

void WebServerManager::handleSetWeatherApiKey() {
    printf("处理设置天气API密钥请求\n");
    
    DynamicJsonDocument doc(256);
    
    if (!server->hasArg("apiKey")) {
        doc["success"] = false;
        doc["message"] = "缺少API密钥参数";
    } else if (!m_weatherManager) {
        doc["success"] = false;
        doc["message"] = "天气管理器未初始化";
    } else {
        String apiKey = server->arg("apiKey");
        bool success = m_weatherManager->setApiKey(apiKey);
        doc["success"] = success;
        doc["message"] = success ? "API密钥设置成功" : "API密钥设置失败";
    }
    
    String response;
    serializeJson(doc, response);
    server->send(200, "application/json", response);
}

void WebServerManager::handleSetWeatherCity() {
    printf("处理设置城市请求\n");
    
    DynamicJsonDocument doc(256);
    
    if (!m_weatherManager) {
        doc["success"] = false;
        doc["message"] = "天气管理器未初始化";
    } else {
        bool hasCity = false;
        WeatherConfig config = m_weatherManager->getConfig();
        
        if (server->hasArg("cityName")) {
            config.cityName = server->arg("cityName");
            hasCity = true;
        }
        
        if (server->hasArg("cityCode")) {
            config.cityCode = server->arg("cityCode");
            hasCity = true;
        }
        
        if (!hasCity) {
            doc["success"] = false;
            doc["message"] = "缺少城市参数";
        } else {
            bool success = m_weatherManager->setConfig(config);
            doc["success"] = success;
            doc["message"] = success ? "城市设置成功" : "城市设置失败";
        }
    }
    
    String response;
    serializeJson(doc, response);
    server->send(200, "application/json", response);
}

void WebServerManager::handleSetWeatherUpdateConfig() {
    printf("处理设置天气更新配置请求\n");
    
    DynamicJsonDocument doc(256);
    
    if (!m_weatherManager) {
        doc["success"] = false;
        doc["message"] = "天气管理器未初始化";
    } else {
        WeatherConfig config = m_weatherManager->getConfig();
        
        if (server->hasArg("autoUpdate")) {
            config.autoUpdate = server->arg("autoUpdate") == "true";
        }
        
        if (server->hasArg("updateInterval")) {
            int interval = server->arg("updateInterval").toInt();
            if (interval >= 5 && interval <= 1440) {
                config.updateInterval = interval;
            }
        }
        
        if (server->hasArg("enableForecast")) {
            config.enableForecast = server->arg("enableForecast") == "true";
        }
        
        bool success = m_weatherManager->setConfig(config);
        doc["success"] = success;
        doc["message"] = success ? "更新配置设置成功" : "更新配置设置失败";
    }
    
    String response;
    serializeJson(doc, response);
    server->send(200, "application/json", response);
}

void WebServerManager::handleGetCurrentWeather() {
    printf("处理获取当前天气请求\n");
    
    DynamicJsonDocument doc(1024);
    
    if (!m_weatherManager) {
        doc["success"] = false;
        doc["message"] = "天气管理器未初始化";
    } else {
        WeatherData weather = m_weatherManager->getCurrentWeather();
        doc["success"] = true;
        doc["weather"]["isValid"] = weather.isValid;
        
        if (weather.isValid) {
            doc["weather"]["city"] = weather.city;
            doc["weather"]["adcode"] = weather.adcode;
            doc["weather"]["weather"] = weather.weather;
            doc["weather"]["temperature"] = weather.temperature;
            doc["weather"]["humidity"] = weather.humidity;
            doc["weather"]["winddirection"] = weather.winddirection;
            doc["weather"]["windpower"] = weather.windpower;
            doc["weather"]["reporttime"] = weather.reporttime;
        }
        
        doc["message"] = weather.isValid ? "天气数据获取成功" : "暂无有效天气数据";
    }
    
    String response;
    serializeJson(doc, response);
    server->send(200, "application/json", response);
}

void WebServerManager::handleGetWeatherStats() {
    printf("处理获取天气统计请求\n");
    
    DynamicJsonDocument doc(512);
    
    if (!m_weatherManager) {
        doc["success"] = false;
        doc["message"] = "天气管理器未初始化";
    } else {
        WeatherStatistics stats = m_weatherManager->getStatistics();
        doc["success"] = true;
        doc["stats"]["totalRequests"] = stats.totalRequests;
        doc["stats"]["successRequests"] = stats.successRequests;
        doc["stats"]["failedRequests"] = stats.failedRequests;
        doc["stats"]["lastUpdateTime"] = stats.lastUpdateTime;
        doc["stats"]["nextUpdateTime"] = stats.nextUpdateTime;
        doc["message"] = "统计信息获取成功";
    }
    
    String response;
    serializeJson(doc, response);
    server->send(200, "application/json", response);
}

void WebServerManager::handleTestWeatherApi() {
    printf("处理测试天气API请求\n");
    
    DynamicJsonDocument doc(256);
    
    if (!m_weatherManager) {
        doc["success"] = false;
        doc["message"] = "天气管理器未初始化";
    } else {
        bool success = m_weatherManager->updateWeatherData();
        doc["success"] = success;
        doc["message"] = success ? "天气API测试成功" : "天气API测试失败";
        
        if (success) {
            doc["state"] = m_weatherManager->getStateString();
        }
    }
    
    String response;
    serializeJson(doc, response);
    server->send(200, "application/json", response);
}

void WebServerManager::handleUpdateWeatherNow() {
    printf("处理立即更新天气请求\n");
    
    DynamicJsonDocument doc(256);
    
    if (!m_weatherManager) {
        doc["success"] = false;
        doc["message"] = "天气管理器未初始化";
    } else {
        bool success = m_weatherManager->updateWeatherData();
        doc["success"] = success;
        doc["message"] = success ? "天气数据更新成功" : "天气数据更新失败";
        
        if (success) {
            doc["state"] = m_weatherManager->getStateString();
            doc["lastUpdate"] = m_weatherManager->getLastUpdateTime();
        }
    }
    
    String response;
    serializeJson(doc, response);
    server->send(200, "application/json", response);
}
 
String WebServerManager::getFileManagerHTML() {
    String html = "";
    html += "<!DOCTYPE html>";
    html += "<html><head><meta charset='UTF-8'>";
    html += "<meta name='viewport' content='width=device-width, initial-scale=1.0'>";
    html += "<title>文件管理器 - ESP32S3 Monitor</title>";
    html += "<style>";
    html += "* {";
    html += "margin: 0;";
    html += "padding: 0;";
    html += "box-sizing: border-box;";
    html += "}";
    html += "body {";
    html += "font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, Oxygen, Ubuntu, Cantarell, sans-serif;";
    html += "background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);";
    html += "min-height: 100vh;";
    html += "color: #333;";
    html += "}";
    html += ".container {";
    html += "max-width: 1000px;";
    html += "margin: 0 auto;";
    html += "padding: 20px;";
    html += "}";
    html += ".header {";
    html += "text-align: center;";
    html += "color: white;";
    html += "margin-bottom: 30px;";
    html += "}";
    html += ".header h1 {";
    html += "font-size: 2.5rem;";
    html += "font-weight: 700;";
    html += "margin-bottom: 10px;";
    html += "text-shadow: 0 2px 4px rgba(0,0,0,0.3);";
    html += "}";
    html += ".subtitle {";
    html += "font-size: 1.1rem;";
    html += "opacity: 0.9;";
    html += "}";
    html += ".card {";
    html += "background: white;";
    html += "border-radius: 16px;";
    html += "padding: 24px;";
    html += "margin-bottom: 24px;";
    html += "box-shadow: 0 8px 32px rgba(0,0,0,0.1);";
    html += "backdrop-filter: blur(10px);";
    html += "}";
    html += ".action-buttons {";
    html += "display: grid;";
    html += "grid-template-columns: repeat(auto-fit, minmax(140px, 1fr));";
    html += "gap: 16px;";
    html += "margin-bottom: 24px;";
    html += "padding: 24px;";
    html += "background: white;";
    html += "border-radius: 16px;";
    html += "box-shadow: 0 8px 32px rgba(0,0,0,0.1);";
    html += "backdrop-filter: blur(10px);";
    html += "}";
    html += "@media (max-width: 768px) {";
    html += ".action-buttons {";
    html += "grid-template-columns: repeat(2, 1fr);";
    html += "gap: 12px;";
    html += "padding: 20px;";
    html += "}";
    html += "}";
    html += "@media (max-width: 480px) {";
    html += ".action-buttons {";
    html += "grid-template-columns: 1fr;";
    html += "gap: 12px;";
    html += "padding: 16px;";
    html += "}";
    html += ".primary-btn, .danger-btn, .success-btn {";
    html += "padding: 12px 16px;";
    html += "font-size: 0.9rem;";
    html += "min-height: 45px;";
    html += "}";

    html += "}";
    html += ".primary-btn, .danger-btn, .success-btn {";
    html += "padding: 14px 20px;";
    html += "border: none;";
    html += "border-radius: 16px;";
    html += "font-weight: 600;";
    html += "cursor: pointer;";
    html += "transition: all 0.4s cubic-bezier(0.4, 0, 0.2, 1);";
    html += "display: flex;";
    html += "align-items: center;";
    html += "justify-content: center;";
    html += "font-size: 0.95rem;";
    html += "text-decoration: none;";
    html += "position: relative;";
    html += "overflow: hidden;";
    html += "min-height: 50px;";
    html += "box-shadow: 0 4px 15px rgba(0,0,0,0.1);";
    html += "}";
    html += ".primary-btn::before, .danger-btn::before, .success-btn::before {";
    html += "content: '';";
    html += "position: absolute;";
    html += "top: 0;";
    html += "left: -100%;";
    html += "width: 100%;";
    html += "height: 100%;";
    html += "background: linear-gradient(90deg, transparent, rgba(255,255,255,0.3), transparent);";
    html += "transition: left 0.5s;";
    html += "}";
    html += ".primary-btn:hover::before, .danger-btn:hover::before, .success-btn:hover::before {";
    html += "left: 100%;";
    html += "}";


    html += ".primary-btn {";
    html += "background: linear-gradient(135deg, #3b82f6, #1d4ed8);";
    html += "color: white;";
    html += "}";
    html += ".primary-btn:hover {";
    html += "transform: translateY(-3px) scale(1.02);";
    html += "box-shadow: 0 12px 35px rgba(59, 130, 246, 0.4);";
    html += "}";
    html += ".primary-btn:active {";
    html += "transform: translateY(-1px) scale(0.98);";
    html += "transition: all 0.1s ease;";
    html += "}";
    html += ".success-btn {";
    html += "background: linear-gradient(135deg, #10b981, #059669);";
    html += "color: white;";
    html += "}";
    html += ".success-btn:hover {";
    html += "transform: translateY(-3px) scale(1.02);";
    html += "box-shadow: 0 12px 35px rgba(16, 185, 129, 0.4);";
    html += "}";
    html += ".success-btn:active {";
    html += "transform: translateY(-1px) scale(0.98);";
    html += "transition: all 0.1s ease;";
    html += "}";
    html += ".danger-btn {";
    html += "background: linear-gradient(135deg, #ef4444, #dc2626);";
    html += "color: white;";
    html += "}";
    html += ".danger-btn:hover {";
    html += "transform: translateY(-3px) scale(1.02);";
    html += "box-shadow: 0 12px 35px rgba(239, 68, 68, 0.4);";
    html += "}";
    html += ".danger-btn:active {";
    html += "transform: translateY(-1px) scale(0.98);";
    html += "transition: all 0.1s ease;";
    html += "}";
    html += ".status-card {";
    html += "background: rgba(255,255,255,0.95);";
    html += "border-radius: 16px;";
    html += "padding: 20px;";
    html += "margin-bottom: 24px;";
    html += "box-shadow: 0 8px 32px rgba(0,0,0,0.1);";
    html += "border-left: 4px solid #10b981;";
    html += "}";
    html += ".upload-zone {";
    html += "border: 2px dashed #d1d5db;";
    html += "border-radius: 12px;";
    html += "padding: 40px 20px;";
    html += "text-align: center;";
    html += "background: #f9fafb;";
    html += "transition: all 0.3s ease;";
    html += "margin-bottom: 24px;";
    html += "}";
    html += ".upload-zone:hover {";
    html += "border-color: #3b82f6;";
    html += "background: #f0f9ff;";
    html += "}";
    html += ".file-item {";
    html += "display: flex;";
    html += "justify-content: space-between;";
    html += "align-items: center;";
    html += "padding: 16px;";
    html += "border-bottom: 1px solid #e5e7eb;";
    html += "transition: all 0.2s ease;";
    html += "}";
    html += ".file-item:hover {";
    html += "background: #f8fafc;";
    html += "}";
    html += ".file-item:last-child {";
    html += "border-bottom: none;";
    html += "}";
    html += ".file-info {";
    html += "display: flex;";
    html += "flex-direction: column;";
    html += "gap: 4px;";
    html += "}";
    html += ".file-name {";
    html += "font-weight: 500;";
    html += "color: #1f2937;";
    html += "}";
    html += ".file-size {";
    html += "color: #6b7280;";
    html += "font-size: 0.875rem;";
    html += "}";
    html += ".file-actions {";
    html += "display: flex;";
    html += "gap: 8px;";
    html += "}";
    html += ".file-btn {";
    html += "padding: 6px 12px;";
    html += "border: none;";
    html += "border-radius: 8px;";
    html += "font-size: 0.8rem;";
    html += "cursor: pointer;";
    html += "transition: all 0.2s ease;";
    html += "color: white;";
    html += "}";
    html += ".file-btn-primary {";
    html += "background: #3b82f6;";
    html += "}";
    html += ".file-btn-primary:hover {";
    html += "background: #2563eb;";
    html += "transform: translateY(-1px);";
    html += "}";
    html += ".file-btn-success {";
    html += "background: #10b981;";
    html += "}";
    html += ".file-btn-success:hover {";
    html += "background: #059669;";
    html += "transform: translateY(-1px);";
    html += "}";
    html += ".file-btn-danger {";
    html += "background: #ef4444;";
    html += "}";
    html += ".file-btn-danger:hover {";
    html += "background: #dc2626;";
    html += "transform: translateY(-1px);";
    html += "}";
    html += ".modal {";
    html += "display: none;";
    html += "position: fixed;";
    html += "z-index: 1000;";
    html += "left: 0;";
    html += "top: 0;";
    html += "width: 100%;";
    html += "height: 100%;";
    html += "background: rgba(0,0,0,0.5);";
    html += "backdrop-filter: blur(4px);";
    html += "}";
    html += ".modal-content {";
    html += "background: white;";
    html += "margin: 10% auto;";
    html += "padding: 24px;";
    html += "border-radius: 16px;";
    html += "width: 90%;";
    html += "max-width: 400px;";
    html += "box-shadow: 0 20px 60px rgba(0,0,0,0.3);";
    html += "}";
    html += ".modal h3 {";
    html += "margin-bottom: 20px;";
    html += "color: #1f2937;";
    html += "font-size: 1.25rem;";
    html += "}";
    html += ".form-group {";
    html += "margin-bottom: 20px;";
    html += "}";
    html += ".form-group label {";
    html += "display: block;";
    html += "margin-bottom: 8px;";
    html += "font-weight: 500;";
    html += "color: #374151;";
    html += "}";
    html += ".form-group input, .form-group textarea {";
    html += "width: 100%;";
    html += "padding: 12px 16px;";
    html += "border: 2px solid #e5e7eb;";
    html += "border-radius: 12px;";
    html += "font-size: 1rem;";
    html += "transition: all 0.3s ease;";
    html += "background: #f9fafb;";
    html += "}";
    html += ".form-group input:focus, .form-group textarea:focus {";
    html += "outline: none;";
    html += "border-color: #3b82f6;";
    html += "background: white;";
    html += "box-shadow: 0 0 0 3px rgba(59, 130, 246, 0.1);";
    html += "}";
    html += ".empty-state {";
    html += "text-align: center;";
    html += "color: #6b7280;";
    html += "padding: 60px 20px;";
    html += "background: #f9fafb;";
    html += "border-radius: 12px;";
    html += "border: 2px dashed #d1d5db;";
    html += "}";
    html += ".empty-state h3 {";
    html += "font-size: 1.25rem;";
    html += "margin-bottom: 8px;";
    html += "color: #374151;";
    html += "}";
    // 存储容量可视化样式
    html += ".storage-info {";
    html += "padding: 16px 0;";
    html += "}";
    html += ".storage-bar {";
    html += "width: 100%;";
    html += "height: 24px;";
    html += "background: #f3f4f6;";
    html += "border-radius: 12px;";
    html += "position: relative;";
    html += "overflow: hidden;";
    html += "margin-bottom: 12px;";
    html += "box-shadow: inset 0 2px 4px rgba(0,0,0,0.1);";
    html += "}";
    html += ".storage-progress {";
    html += "height: 100%;";
    html += "background: linear-gradient(90deg, #10b981, #059669);";
    html += "border-radius: 12px;";
    html += "width: 0%;";
    html += "transition: all 0.8s cubic-bezier(0.4, 0, 0.2, 1);";
    html += "position: relative;";
    html += "box-shadow: 0 2px 8px rgba(16, 185, 129, 0.3);";
    html += "}";
    html += ".storage-progress::after {";
    html += "content: '';";
    html += "position: absolute;";
    html += "top: 0;";
    html += "left: 0;";
    html += "right: 0;";
    html += "bottom: 0;";
    html += "background: linear-gradient(90deg, transparent, rgba(255,255,255,0.3), transparent);";
    html += "animation: storage-shine 2s infinite;";
    html += "}";
    html += "@keyframes storage-shine {";
    html += "0% { transform: translateX(-100%); }";
    html += "100% { transform: translateX(100%); }";
    html += "}";
    html += ".storage-text {";
    html += "display: flex;";
    html += "justify-content: space-between;";
    html += "align-items: center;";
    html += "font-size: 0.9rem;";
    html += "color: #6b7280;";
    html += "font-weight: 500;";
    html += "}";
    html += ".usage-percent {";
    html += "font-weight: 600;";
    html += "color: #1f2937;";
    html += "}";
    html += "</style></head><body>";
    
    html += "<div class='container'>";
    html += "<div class='header'>";
    html += "<h1>小屏幕配置</h1>";
    html += "<div class='subtitle'>文件管理器</div>";
    html += "</div>";
    
    html += "<div class='action-buttons'>";
    html += "<button class='primary-btn' onclick='refreshFiles()'>刷新</button>";
    html += "<button class='success-btn' onclick='showCreateModal()'>新建文件</button>";
    html += "<button class='primary-btn' onclick=\"document.getElementById('fileInput').click()\">上传文件</button>";
    html += "<button class='danger-btn' onclick='formatFileSystem()'>格式化</button>";
    html += "<a href='/' class='primary-btn'>返回首页</a>";
    html += "</div>";
    
    html += "<div class='status-card' id='fsStatus'>文件系统状态：加载中...</div>";
    
    // 存储容量可视化显示
    html += "<div class='card'>";
    html += "<h3 style='margin-bottom: 16px; color: #1f2937;'>存储容量</h3>";
    html += "<div class='storage-info' id='storageInfo'>";
    html += "<div class='storage-bar'>";
    html += "<div class='storage-progress' id='storageProgress'></div>";
    html += "</div>";
    html += "<div class='storage-text' id='storageText'>加载中...</div>";
    html += "</div>";
    html += "</div>";
    
    html += "<div class='card'>";
    html += "<div class='upload-zone' id='uploadZone'>";
    html += "<h3>文件上传</h3>";
    html += "<p>拖拽文件到此处上传，或点击上传按钮选择文件</p>";
    html += "<input type='file' id='fileInput' style='display:none' multiple>";
    html += "</div>";
    html += "</div>";
    
    html += "<div class='card'>";
    html += "<h2 style='margin-bottom: 20px; color: #1f2937;'>文件列表</h2>";
    html += "<div id='fileList'>加载文件列表中...</div>";
    html += "</div>";
    html += "</div>";
    
    // 新建文件模态框
    html += "<div id='createModal' class='modal'>";
    html += "<div class='modal-content'>";
    html += "<h3>新建文件</h3>";
    html += "<div class='form-group'>";
    html += "<label>文件名：</label>";
    html += "<input type='text' id='fileName' placeholder='例如：test.txt'>";
    html += "</div>";
    html += "<div class='form-group'>";
    html += "<label>文件内容：</label>";
    html += "<textarea id='fileContent' rows='6'></textarea>";
    html += "</div>";
    html += "<div style='display: flex; gap: 12px; justify-content: flex-end;'>";
    html += "<button class='primary-btn' onclick='createFile()'>创建</button>";
    html += "<button class='primary-btn' onclick='closeCreateModal()' style='background: #6b7280;'>取消</button>";
    html += "</div>";
    html += "</div></div>";
    
    // 重命名模态框
    html += "<div id='renameModal' class='modal'>";
    html += "<div class='modal-content'>";
    html += "<h3>重命名文件</h3>";
    html += "<div class='form-group'>";
    html += "<label>新文件名：</label>";
    html += "<input type='text' id='newFileName'>";
    html += "</div>";
    html += "<div style='display: flex; gap: 12px; justify-content: flex-end;'>";
    html += "<button class='primary-btn' onclick='renameFile()'>重命名</button>";
    html += "<button class='primary-btn' onclick='closeRenameModal()' style='background: #6b7280;'>取消</button>";
    html += "</div>";
    html += "</div></div>";
    
    // JavaScript
    html += "<script>";
    html += "var currentRenamePath='';";
    html += "document.addEventListener('DOMContentLoaded',function(){loadFileSystemStatus();loadFileList();setupFileUpload();});";
    
    html += "function loadFileSystemStatus(){";
    html += "fetch('/api/filesystem/status').then(function(r){return r.json();}).then(function(data){";
    html += "var el=document.getElementById('fsStatus');";
    html += "if(data.initialized){";
    html += "el.innerHTML='文件系统状态：正常运行<br>总容量：'+data.totalFormatted+' | 已使用：'+data.usedFormatted+' | 可用：'+data.freeFormatted;";
    html += "updateStorageVisualization(data);";
    html += "}else{el.innerHTML='警告：文件系统未初始化';resetStorageVisualization();}";
    html += "}).catch(function(){document.getElementById('fsStatus').innerHTML='错误：无法加载状态';resetStorageVisualization();});";
    html += "}";
    
    html += "function loadFileList(){";
    html += "fetch('/api/files').then(function(r){return r.json();}).then(function(data){";
    html += "var el=document.getElementById('fileList');";
    html += "if(data.success&&data.files.length>0){";
    html += "var html='';";
    html += "for(var i=0;i<data.files.length;i++){";
    html += "var f=data.files[i];";
    html += "html+='<div class=\"file-item\">';";
    html += "html+='<div class=\"file-info\">';";
    html += "html+='<div class=\"file-name\">'+f.name+'</div>';";
    html += "html+='<div class=\"file-size\">大小：'+f.sizeFormatted+'</div>';";
    html += "html+='</div>';";
    html += "html+='<div class=\"file-actions\">';";
    html += "if(!f.isDirectory){";
    html += "html+='<button class=\"file-btn file-btn-primary\" onclick=\"downloadFile(\\'' + f.name + '\\')\">下载</button>';";
    html += "html+='<button class=\"file-btn file-btn-success\" onclick=\"showRenameModal(\\'' + f.name + '\\')\">重命名</button>';";
    html += "}";
    html += "html+='<button class=\"file-btn file-btn-danger\" onclick=\"deleteFile(\\'' + f.name + '\\')\">删除</button>';";
    html += "html+='</div>';";
    html += "html+='</div>';";
    html += "}";
    html += "el.innerHTML=html;";
    html += "}else{";
    html += "el.innerHTML='<div class=\"empty-state\"><h3>文件系统为空</h3><p>还没有任何文件，快来上传一些文件吧！</p></div>';";
    html += "}";
    html += "}).catch(function(){";
    html += "document.getElementById('fileList').innerHTML='<div class=\"empty-state\"><h3>加载失败</h3><p>无法连接到服务器</p></div>';";
    html += "});";
    html += "}";
    
    html += "function refreshFiles(){loadFileSystemStatus();loadFileList();}";
    
    // 存储容量可视化更新函数
    html += "function updateStorageVisualization(data){";
    html += "var progressEl=document.getElementById('storageProgress');";
    html += "var textEl=document.getElementById('storageText');";
    html += "if(progressEl&&textEl){";
    html += "var usagePercent=data.usagePercent||0;";
    html += "progressEl.style.width=usagePercent+'%';";
    // 根据使用率改变颜色
    html += "var color='#10b981';"; // 默认绿色
    html += "if(usagePercent>=90){color='#ef4444';}"; // 红色：危险
    html += "else if(usagePercent>=75){color='#f59e0b';}"; // 橙色：警告
    html += "else if(usagePercent>=50){color='#3b82f6';}"; // 蓝色：中等
    html += "progressEl.style.background='linear-gradient(90deg, '+color+', '+color+'cc)';";
    html += "progressEl.style.boxShadow='0 2px 8px '+color+'4d';";
    html += "textEl.innerHTML='<span>'+data.usedFormatted+' / '+data.totalFormatted+'</span><span class=\"usage-percent\">'+usagePercent.toFixed(1)+'%</span>';";
    html += "}";
    html += "}";
    
    html += "function resetStorageVisualization(){";
    html += "var progressEl=document.getElementById('storageProgress');";
    html += "var textEl=document.getElementById('storageText');";
    html += "if(progressEl&&textEl){";
    html += "progressEl.style.width='0%';";
    html += "progressEl.style.background='#d1d5db';";
    html += "textEl.innerHTML='存储状态未知';";
    html += "}";
    html += "}";
    
    html += "function setupFileUpload(){";
    html += "var fileInput=document.getElementById('fileInput');";
    html += "var uploadZone=document.getElementById('uploadZone');";
    html += "fileInput.addEventListener('change',function(e){uploadFiles(e.target.files);});";
    html += "uploadZone.addEventListener('dragover',function(e){e.preventDefault();uploadZone.style.borderColor='#3b82f6';uploadZone.style.background='#f0f9ff';});";
    html += "uploadZone.addEventListener('dragleave',function(e){e.preventDefault();uploadZone.style.borderColor='#d1d5db';uploadZone.style.background='#f9fafb';});";
    html += "uploadZone.addEventListener('drop',function(e){e.preventDefault();uploadZone.style.borderColor='#d1d5db';uploadZone.style.background='#f9fafb';uploadFiles(e.dataTransfer.files);});";
    html += "}";
    
    html += "function uploadFiles(files){";
    html += "if(files.length===0)return;";
    html += "for(var i=0;i<files.length;i++){";
    html += "uploadSingleFile(files[i]);";
    html += "}";
    html += "}";
    
    html += "function uploadSingleFile(file){";
    html += "var formData=new FormData();";
    html += "formData.append('file',file);";
    html += "document.getElementById('fsStatus').innerHTML='正在上传文件：'+file.name+'...';";
    html += "fetch('/api/files/upload',{method:'POST',body:formData})";
    html += ".then(function(r){return r.json();}).then(function(data){";
    html += "if(data.success){";
    html += "document.getElementById('fsStatus').innerHTML='文件上传成功：'+file.name;";
    html += "setTimeout(function(){refreshFiles();},1000);";
    html += "}else{";
    html += "document.getElementById('fsStatus').innerHTML='文件上传失败：'+(data.message||'未知错误');";
    html += "alert('文件上传失败：'+(data.message||'未知错误'));";
    html += "}";
    html += "}).catch(function(err){";
    html += "document.getElementById('fsStatus').innerHTML='文件上传失败：网络错误';";
    html += "alert('文件上传失败：网络错误');";
    html += "});";
    html += "}";
    
    html += "function downloadFile(filename){";
    html += "var url='/api/files/download?path='+encodeURIComponent(filename);";
    html += "var link=document.createElement('a');link.href=url;link.download=filename;";
    html += "document.body.appendChild(link);link.click();document.body.removeChild(link);";
    html += "}";
    
    html += "function deleteFile(filename){";
    html += "if(!confirm('确定要删除文件 \"'+filename+'\" 吗？'))return;";
    html += "fetch('/api/files/delete?path='+encodeURIComponent(filename),{method:'DELETE'})";
    html += ".then(function(r){return r.json();}).then(function(data){";
    html += "if(data.success){alert('删除成功');refreshFiles();}else{alert('删除失败：'+data.message);}";
    html += "}).catch(function(){alert('删除失败');});";
    html += "}";
    
    html += "function showCreateModal(){document.getElementById('createModal').style.display='block';";
    html += "document.getElementById('fileName').value='';document.getElementById('fileContent').value='';}";
    html += "function closeCreateModal(){document.getElementById('createModal').style.display='none';}";
    
    html += "function createFile(){";
    html += "var name=document.getElementById('fileName').value.trim();";
    html += "var content=document.getElementById('fileContent').value;";
    html += "if(!name){alert('请输入文件名');return;}";
    html += "var formData=new FormData();formData.append('path',name);formData.append('content',content);";
    html += "fetch('/api/files/create',{method:'POST',body:formData})";
    html += ".then(function(r){return r.json();}).then(function(data){";
    html += "if(data.success){alert('创建成功');closeCreateModal();refreshFiles();}else{alert('创建失败：'+data.message);}";
    html += "}).catch(function(){alert('创建失败');});";
    html += "}";
    
    html += "function showRenameModal(filename){currentRenamePath=filename;";
    html += "document.getElementById('renameModal').style.display='block';";
    html += "document.getElementById('newFileName').value=filename;}";
    html += "function closeRenameModal(){document.getElementById('renameModal').style.display='none';}";
    
    html += "function renameFile(){";
    html += "var newName=document.getElementById('newFileName').value.trim();";
    html += "if(!newName){alert('请输入新文件名');return;}";
    html += "var formData=new FormData();formData.append('oldPath',currentRenamePath);formData.append('newPath',newName);";
    html += "fetch('/api/files/rename',{method:'POST',body:formData})";
    html += ".then(function(r){return r.json();}).then(function(data){";
    html += "if(data.success){alert('重命名成功');closeRenameModal();refreshFiles();}else{alert('重命名失败：'+data.message);}";
    html += "}).catch(function(){alert('重命名失败');});";
    html += "}";
    
    html += "function formatFileSystem(){";
    html += "if(!confirm('警告：格式化将删除所有文件！确定要继续吗？'))return;";
    html += "if(!confirm('最后确认：您真的要格式化文件系统吗？'))return;";
    html += "startFormatting();";
    html += "}";
    
    html += "function startFormatting(){";
    html += "document.getElementById('fsStatus').innerHTML='正在启动格式化任务...';";
    html += "fetch('/api/filesystem/format',{method:'POST'})";
    html += ".then(function(r){return r.json()}).then(function(data){";
    html += "if(data.success){";
    html += "document.getElementById('fsStatus').innerHTML='格式化任务已启动，正在进行中...';";
    html += "checkFormatStatus();";
    html += "}else{";
    html += "document.getElementById('fsStatus').innerHTML='格式化启动失败：'+data.message;";
    html += "alert('格式化启动失败：'+data.message);";
    html += "}";
    html += "}).catch(function(){";
    html += "document.getElementById('fsStatus').innerHTML='格式化启动失败';";
    html += "alert('格式化启动失败');";
    html += "});";
    html += "}";
    
    html += "function checkFormatStatus(){";
    html += "fetch('/api/filesystem/format-status')";
    html += ".then(function(r){return r.json()}).then(function(data){";
    html += "if(data.formatting){";
    html += "document.getElementById('fsStatus').innerHTML='格式化进行中，请稍候...';";
    html += "setTimeout(checkFormatStatus,2000);";
    html += "}else{";
    html += "if(data.success){";
    html += "alert('格式化成功完成！');";
    html += "document.getElementById('fsStatus').innerHTML='格式化完成';";
    html += "}else{";
    html += "alert('格式化失败');";
    html += "document.getElementById('fsStatus').innerHTML='格式化失败';";
    html += "}";
    html += "setTimeout(function(){refreshFiles();},1000);";
    html += "}";
    html += "}).catch(function(){";
    html += "document.getElementById('fsStatus').innerHTML='无法获取格式化状态';";
    html += "setTimeout(checkFormatStatus,3000);";
    html += "});";
    html += "}";
    
    html += "window.onclick=function(event){";
    html += "if(event.target==document.getElementById('createModal'))closeCreateModal();";
    html += "if(event.target==document.getElementById('renameModal'))closeRenameModal();";
    html += "}";
    
    html += "</script></body></html>";
    
    return html;
}

void WebServerManager::handleUpdateWiFiPriority() {
    printf("处理更新WiFi优先级请求\n");
    
    if (!server->hasArg("index") || !server->hasArg("priority")) {
        printf("缺少必要参数\n");
        server->send(400, "application/json", "{\"success\":false,\"message\":\"缺少index或priority参数\"}");
        return;
    }
    
    int index = server->arg("index").toInt();
    int priority = server->arg("priority").toInt();
    
    printf("请求更新配置索引 %d 的优先级为 %d\n", index, priority);
    
    if (index < 0 || index >= ConfigStorage::MAX_WIFI_CONFIGS) {
        printf("无效的配置索引: %d\n", index);
        server->send(400, "application/json", "{\"success\":false,\"message\":\"无效的配置索引\"}");
        return;
    }
    
    if (priority < 1 || priority > 99) {
        printf("无效的优先级: %d\n", priority);
        server->send(400, "application/json", "{\"success\":false,\"message\":\"优先级必须在1-99之间\"}");
        return;
    }
    
    DynamicJsonDocument doc(256);
    
    bool success = configStorage->updateWiFiPriorityAsync(index, priority, 3000);
    
    doc["success"] = success;
    if (success) {
        doc["message"] = "WiFi优先级更新成功";
        printf("WiFi优先级更新成功\n");
    } else {
        doc["message"] = "WiFi优先级更新失败";
        printf("WiFi优先级更新失败\n");
    }
    
    String response;
    serializeJson(doc, response);
    server->send(200, "application/json", response);
}

void WebServerManager::handleSetWiFiPriorities() {
    printf("处理批量设置WiFi优先级请求\n");
    
    if (!server->hasArg("priorities")) {
        printf("缺少priorities参数\n");
        server->send(400, "application/json", "{\"success\":false,\"message\":\"缺少priorities参数\"}");
        return;
    }
    
    String prioritiesStr = server->arg("priorities");
    printf("接收到优先级数据: %s\n", prioritiesStr.c_str());
    
    // 解析JSON格式的优先级数组
    DynamicJsonDocument requestDoc(512);
    DeserializationError error = deserializeJson(requestDoc, prioritiesStr);
    
    if (error) {
        printf("JSON解析失败\n");
        server->send(400, "application/json", "{\"success\":false,\"message\":\"JSON格式错误\"}");
        return;
    }
    
    if (!requestDoc.is<JsonArray>()) {
        printf("priorities不是数组格式\n");
        server->send(400, "application/json", "{\"success\":false,\"message\":\"priorities必须是数组格式\"}");
        return;
    }
    
    JsonArray priorityArray = requestDoc.as<JsonArray>();
    if (priorityArray.size() != ConfigStorage::MAX_WIFI_CONFIGS) {
        printf("优先级数组长度不正确: %d\n", priorityArray.size());
        server->send(400, "application/json", "{\"success\":false,\"message\":\"优先级数组长度必须为3\"}");
        return;
    }
    
    int priorities[3];
    for (int i = 0; i < ConfigStorage::MAX_WIFI_CONFIGS; i++) {
        priorities[i] = priorityArray[i].as<int>();
        if (priorities[i] < 1 || priorities[i] > 99) {
            printf("无效的优先级值: %d\n", priorities[i]);
            server->send(400, "application/json", "{\"success\":false,\"message\":\"优先级必须在1-99之间\"}");
            return;
        }
        printf("设置配置 %d 优先级为 %d\n", i, priorities[i]);
    }
    
    DynamicJsonDocument doc(256);
    
    bool success = configStorage->setWiFiPrioritiesAsync(priorities, 3000);
    
    doc["success"] = success;
    if (success) {
        doc["message"] = "WiFi优先级批量设置成功";
        printf("WiFi优先级批量设置成功\n");
    } else {
        doc["message"] = "WiFi优先级批量设置失败";
        printf("WiFi优先级批量设置失败\n");
    }
    
    String response;
    serializeJson(doc, response);
    server->send(200, "application/json", response);
}

void WebServerManager::handleScreenConfig() {
    printf("处理屏幕配置获取请求\n");
    
    DynamicJsonDocument doc(512);
    
    if (m_displayManager) {
        // 获取当前亮度（DisplayManager使用0-100范围，转换为0-255）
        uint8_t currentBrightness = m_displayManager->getBrightness();
        uint8_t brightness255 = (currentBrightness * 255) / 100;
        
        doc["success"] = true;
        doc["brightness"] = brightness255;
        doc["brightnessPercent"] = currentBrightness;
        doc["screenOn"] = true;  // 假设屏幕开启
        doc["backlightOn"] = true;  // 假设背光开启
        doc["maxBrightness"] = "350nit";  // 最大亮度
        
        // 新增屏幕参数信息
        doc["resolution"] = "368x448";  // 屏幕分辨率
        doc["colorSupport"] = "RGB565";  // 色彩支持
        doc["material"] = "AMOLED";  // 屏幕材质
        
        doc["message"] = "屏幕配置获取成功";
        
        printf("当前屏幕亮度: %d%% (%d/255)\n", currentBrightness, brightness255);
    } else {
        doc["success"] = false;
        doc["message"] = "显示管理器未初始化";
        doc["brightness"] = 128;  // 默认值
        doc["brightnessPercent"] = 50;
        doc["screenOn"] = false;
        doc["backlightOn"] = false;
        doc["maxBrightness"] = "350nit";  // 最大亮度
        
        // 默认屏幕参数信息
        doc["resolution"] = "368x448";  // 屏幕分辨率
        doc["colorSupport"] = "RGB565";  // 色彩支持
        doc["material"] = "AMOLED";  // 屏幕材质
        
        printf("显示管理器未初始化\n");
    }
    
    String response;
    serializeJson(doc, response);
    server->send(200, "application/json", response);
}

void WebServerManager::handleSetBrightness() {
    printf("处理屏幕亮度设置请求\n");
    
    if (!server->hasArg("brightness")) {
        printf("⚠️ 缺少brightness参数\n");
        server->send(400, "application/json", "{\"success\":false,\"message\":\"缺少brightness参数\"}");
        return;
    }
    
    int brightness255 = server->arg("brightness").toInt();
    printf("📥 接收到亮度设置请求: %d/255\n", brightness255);
    
    if (brightness255 < 10 || brightness255 > 255) {
        printf("❌ 无效的亮度值: %d\n", brightness255);
        server->send(400, "application/json", "{\"success\":false,\"message\":\"亮度值必须在10-255之间\"}");
        return;
    }
    
    DynamicJsonDocument doc(256);
    
    if (m_displayManager) {
        // 将0-255范围转换为0-100范围
        uint8_t brightness100 = (brightness255 * 100) / 255;
        printf("📊 转换亮度: %d/255 -> %d%%\n", brightness255, brightness100);
        printf("🔄 准备调用DisplayManager::setBrightness(%d)\n", brightness100);
        
        m_displayManager->setBrightness(brightness100);
        
        doc["success"] = true;
        doc["message"] = "亮度设置成功";
        doc["brightness"] = brightness255;
        doc["brightnessPercent"] = brightness100;
        
        printf("✅ 屏幕亮度设置请求完成: %d%% (%d/255)\n", brightness100, brightness255);
    } else {
        doc["success"] = false;
        doc["message"] = "显示管理器未初始化";
        
        printf("❌ 显示管理器未初始化，无法设置亮度\n");
    }
    
    String response;
    serializeJson(doc, response);
    server->send(200, "application/json", response);
}

void WebServerManager::handleScreenTest() {
    printf("处理屏幕测试请求\n");
    
    DynamicJsonDocument doc(256);
    
    if (m_displayManager) {
        // 执行屏幕测试：循环显示不同亮度
        m_displayManager->showNotification("屏幕测试：亮度循环", 5000);
        
        // 启动一个简单的亮度测试序列
        // 这里可以创建一个任务来执行测试，暂时只显示通知
        doc["success"] = true;
        doc["message"] = "屏幕测试已启动";
        
        printf("屏幕测试已启动\n");
    } else {
        doc["success"] = false;
        doc["message"] = "显示管理器未初始化";
        
        printf("显示管理器未初始化，无法执行屏幕测试\n");
    }
    
    String response;
    serializeJson(doc, response);
    server->send(200, "application/json", response);
}

void WebServerManager::handleScreenSettings() {
    printf("处理屏幕设置页面请求\n");
    server->send(200, "text/html", getScreenSettingsHTML());
}

void WebServerManager::handleGetScreenSettings() {
    printf("处理获取屏幕设置配置请求\n");
    
    DynamicJsonDocument doc(512);
    
    if (configStorage) {
        ScreenMode mode;
        int startHour, startMinute, endHour, endMinute, timeoutMinutes;
        bool autoRotationEnabled;
        int staticRotation;
        
        bool hasConfig = configStorage->hasScreenConfigAsync(3000);
        if (hasConfig) {
            bool success = configStorage->loadScreenConfigAsync(mode, startHour, startMinute, 
                                                              endHour, endMinute, timeoutMinutes, autoRotationEnabled, staticRotation, 3000);
            if (success) {
                doc["success"] = true;
                doc["mode"] = (int)mode;
                doc["startHour"] = startHour;
                doc["startMinute"] = startMinute;
                doc["endHour"] = endHour;
                doc["endMinute"] = endMinute;
                doc["timeoutMinutes"] = timeoutMinutes;
                doc["autoRotationEnabled"] = autoRotationEnabled;
                doc["staticRotation"] = staticRotation;
                doc["message"] = "屏幕设置配置获取成功";
                
                printf("屏幕设置配置获取成功：模式=%d, 开始时间=%02d:%02d, 结束时间=%02d:%02d, 延时=%d分钟, 自动旋转=%s, 静态旋转=%d度\n",
                       (int)mode, startHour, startMinute, endHour, endMinute, timeoutMinutes, autoRotationEnabled ? "启用" : "禁用", staticRotation * 90);
            } else {
                doc["success"] = false;
                doc["message"] = "屏幕设置配置加载失败";
                printf("屏幕设置配置加载失败\n");
            }
        } else {
            // 返回默认配置
            doc["success"] = true;
            doc["mode"] = SCREEN_MODE_ALWAYS_ON;
            doc["startHour"] = 8;
            doc["startMinute"] = 0;
            doc["endHour"] = 22;
            doc["endMinute"] = 0;
            doc["timeoutMinutes"] = 10;
            doc["autoRotationEnabled"] = true;
            doc["staticRotation"] = 0;
            doc["message"] = "返回默认屏幕设置配置";
            
            printf("返回默认屏幕设置配置\n");
        }
    } else {
        doc["success"] = false;
        doc["message"] = "配置存储未初始化";
        printf("配置存储未初始化\n");
    }
    
    String response;
    serializeJson(doc, response);
    server->send(200, "application/json", response);
}

void WebServerManager::handleSetScreenSettings() {
    printf("处理设置屏幕设置配置请求\n");
    
    DynamicJsonDocument doc(256);
    
    if (!configStorage) {
        doc["success"] = false;
        doc["message"] = "配置存储未初始化";
        printf("配置存储未初始化\n");
        String response;
        serializeJson(doc, response);
        server->send(400, "application/json", response);
        return;
    }
    
    // 检查必要参数
    if (!server->hasArg("mode")) {
        doc["success"] = false;
        doc["message"] = "缺少mode参数";
        printf("缺少mode参数\n");
        String response;
        serializeJson(doc, response);
        server->send(400, "application/json", response);
        return;
    }
    
    int mode = server->arg("mode").toInt();
    int startHour = server->hasArg("startHour") ? server->arg("startHour").toInt() : 8;
    int startMinute = server->hasArg("startMinute") ? server->arg("startMinute").toInt() : 0;
    int endHour = server->hasArg("endHour") ? server->arg("endHour").toInt() : 22;
    int endMinute = server->hasArg("endMinute") ? server->arg("endMinute").toInt() : 0;
    int timeoutMinutes = server->hasArg("timeoutMinutes") ? server->arg("timeoutMinutes").toInt() : 10;
    bool autoRotationEnabled = server->hasArg("autoRotationEnabled") ? (server->arg("autoRotationEnabled") == "true") : true;
    int staticRotation = server->hasArg("staticRotation") ? server->arg("staticRotation").toInt() : 0;
    
    printf("接收到屏幕设置配置：模式=%d, 开始时间=%02d:%02d, 结束时间=%02d:%02d, 延时=%d分钟, 自动旋转=%s, 静态旋转=%d度\n",
           mode, startHour, startMinute, endHour, endMinute, timeoutMinutes, autoRotationEnabled ? "启用" : "禁用", staticRotation * 90);
    
    // 参数验证
    if (mode < 0 || mode > 3) {
        doc["success"] = false;
        doc["message"] = "无效的屏幕模式";
        printf("无效的屏幕模式: %d\n", mode);
        String response;
        serializeJson(doc, response);
        server->send(400, "application/json", response);
        return;
    }
    
    if (startHour < 0 || startHour > 23 || startMinute < 0 || startMinute > 59 ||
        endHour < 0 || endHour > 23 || endMinute < 0 || endMinute > 59) {
        doc["success"] = false;
        doc["message"] = "无效的时间设置";
        printf("无效的时间设置\n");
        String response;
        serializeJson(doc, response);
        server->send(400, "application/json", response);
        return;
    }
    
    if (timeoutMinutes < 1 || timeoutMinutes > 1440) {
        doc["success"] = false;
        doc["message"] = "延时时间必须在1-1440分钟之间";
        printf("无效的延时时间: %d\n", timeoutMinutes);
        String response;
        serializeJson(doc, response);
        server->send(400, "application/json", response);
        return;
    }
    
    if (staticRotation < 0 || staticRotation > 3) {
        doc["success"] = false;
        doc["message"] = "静态旋转角度必须在0-3之间";
        printf("无效的静态旋转角度: %d\n", staticRotation);
        String response;
        serializeJson(doc, response);
        server->send(400, "application/json", response);
        return;
    }
    
    // 保存配置
    bool success = configStorage->saveScreenConfigAsync((ScreenMode)mode, startHour, startMinute,
                                                       endHour, endMinute, timeoutMinutes, autoRotationEnabled, staticRotation, 3000);
    
    doc["success"] = success;
    if (success) {
        doc["message"] = "屏幕设置配置保存成功";
        printf("屏幕设置配置保存成功\n");
        
        // 通知DisplayManager屏幕模式已改变
        if (m_displayManager) {
            // 注意：这里不传递staticRotation，因为setScreenMode会自动获取当前角度
            m_displayManager->setScreenMode((ScreenMode)mode, startHour, startMinute, 
                                          endHour, endMinute, timeoutMinutes, autoRotationEnabled);
            printf("已通知DisplayManager屏幕模式变更\n");
        }
    } else {
        doc["message"] = "屏幕设置配置保存失败";
        printf("屏幕设置配置保存失败\n");
    }
    
    String response;
    serializeJson(doc, response);
    server->send(200, "application/json", response);
}

void WebServerManager::handleGetCurrentRotation() {
    printf("处理获取当前屏幕旋转角度请求\n");
    
    DynamicJsonDocument doc(256);
    
    if (!m_displayManager || !m_displayManager->getLVGLDriver()) {
        doc["success"] = false;
        doc["message"] = "显示管理器未初始化";
        printf("显示管理器未初始化\n");
        String response;
        serializeJson(doc, response);
        server->send(400, "application/json", response);
        return;
    }
    
    // 获取当前屏幕旋转角度
    int currentRotation = (int)m_displayManager->getLVGLDriver()->getScreenRotation();
    
    doc["success"] = true;
    doc["rotation"] = currentRotation;
    doc["angle"] = currentRotation * 90;
    doc["message"] = "当前屏幕旋转角度获取成功";
    
    printf("当前屏幕旋转角度: %d (%d度)\n", currentRotation, currentRotation * 90);
    
    String response;
    serializeJson(doc, response);
    server->send(200, "application/json", response);
}

void WebServerManager::handleGetThemeSettings() {
    printf("处理获取主题设置请求\n");
    
    DynamicJsonDocument doc(256);
    
    if (!configStorage) {
        doc["success"] = false;
        doc["message"] = "配置存储未初始化";
        printf("配置存储未初始化\n");
        String response;
        serializeJson(doc, response);
        server->send(400, "application/json", response);
        return;
    }
    
    // 获取当前主题设置
    if (configStorage->hasThemeConfigAsync(3000)) {
        int theme = configStorage->loadThemeConfigAsync(3000);
        doc["success"] = true;
        doc["theme"] = theme;
        doc["message"] = "主题配置获取成功";
        printf("主题配置获取成功: %d\n", theme);
    } else {
        doc["success"] = true;
        doc["theme"] = 0; // 默认UI1主题
        doc["message"] = "使用默认主题配置";
        printf("使用默认主题配置\n");
    }
    
    String response;
    serializeJson(doc, response);
    server->send(200, "application/json", response);
}

void WebServerManager::handleSetThemeSettings() {
    printf("处理设置主题配置请求\n");
    
    DynamicJsonDocument doc(256);
    
    if (!configStorage) {
        doc["success"] = false;
        doc["message"] = "配置存储未初始化";
        printf("配置存储未初始化\n");
        String response;
        serializeJson(doc, response);
        server->send(400, "application/json", response);
        return;
    }
    
    // 检查必要参数
    if (!server->hasArg("theme")) {
        doc["success"] = false;
        doc["message"] = "缺少theme参数";
        printf("缺少theme参数\n");
        String response;
        serializeJson(doc, response);
        server->send(400, "application/json", response);
        return;
    }
    
    int theme = server->arg("theme").toInt();
    
    printf("接收到主题切换请求：主题=%d\n", theme);
    
    // 参数验证
    if (theme < 0 || theme > 2) {
        doc["success"] = false;
        doc["message"] = "无效的主题值";
        printf("无效的主题值: %d\n", theme);
        String response;
        serializeJson(doc, response);
        server->send(400, "application/json", response);
        return;
    }
    
    // 检查是否是当前主题
    if (configStorage->hasThemeConfigAsync(3000)) {
        int currentTheme = configStorage->loadThemeConfigAsync(3000);
        if (currentTheme == theme) {
            doc["success"] = true;
            doc["message"] = "当前已是选定的主题";
            doc["theme"] = theme;
            doc["needRestart"] = false;
            printf("当前已是选定的主题: %d\n", theme);
            String response;
            serializeJson(doc, response);
            server->send(200, "application/json", response);
            return;
        }
    }
    
    // 保存主题配置到NVS
    printf("保存主题配置到NVS: %d\n", theme);
    bool saveSuccess = configStorage->saveThemeConfigAsync(theme, 3000);
    
    if (saveSuccess) {
        doc["success"] = true;
        doc["message"] = "主题配置已保存，系统将在3秒后重启以应用新主题";
        doc["theme"] = theme;
        doc["needRestart"] = true;
        doc["restartDelay"] = 3000; // 3秒延迟
        printf("主题配置保存成功，准备重启系统\n");
        
        String response;
        serializeJson(doc, response);
        server->send(200, "application/json", response);
        
        // 延时3秒后重启设备以应用新主题
        vTaskDelay(pdMS_TO_TICKS(3000));
        printf("重启系统以应用新主题...\n");
        ESP.restart();
    } else {
        doc["success"] = false;
        doc["message"] = "主题配置保存失败";
        doc["theme"] = theme;
        doc["needRestart"] = false;
        printf("主题配置保存失败\n");
        
        String response;
        serializeJson(doc, response);
        server->send(500, "application/json", response);
    }
}

void WebServerManager::handleSystemSettings() {
    printf("处理系统设置页面请求\n");
    server->send(200, "text/html", getSystemSettingsHTML());
}

void WebServerManager::handleGetTimeConfig() {
    printf("处理获取时间配置请求\n");
    
    DynamicJsonDocument doc(512);
    
    // 获取当前时间信息
    time_t now;
    struct tm timeinfo;
    time(&now);
    localtime_r(&now, &timeinfo);
    
    doc["success"] = true;
    doc["currentTime"] = asctime(&timeinfo);
    doc["timestamp"] = now;
    doc["timezone"] = "UTC+8";
    doc["ntpEnabled"] = true;
    doc["ntpServer"] = "pool.ntp.org";
    doc["message"] = "时间配置获取成功";
    
    String response;
    serializeJson(doc, response);
    server->send(200, "application/json", response);
}

void WebServerManager::handleSetTimeConfig() {
    printf("处理设置时间配置请求\n");
    
    DynamicJsonDocument doc(256);
    
    if (server->hasArg("timezone")) {
        String timezone = server->arg("timezone");
        printf("设置时区: %s\n", timezone.c_str());
        
        // 这里可以添加时区设置逻辑
        doc["success"] = true;
        doc["message"] = "时区设置成功";
        doc["timezone"] = timezone;
    } else {
        doc["success"] = false;
        doc["message"] = "缺少时区参数";
    }
    
    String response;
    serializeJson(doc, response);
    server->send(200, "application/json", response);
}

void WebServerManager::handleSyncTime() {
    printf("处理时间同步请求\n");
    
    DynamicJsonDocument doc(256);
    
    if (wifiManager->isConnected()) {
        // 执行NTP时间同步
        printf("开始NTP时间同步...\n");
        
        configTime(8 * 3600, 0, "pool.ntp.org", "time.nist.gov");
        
        // 等待时间同步完成
        int retries = 0;
        while (time(nullptr) < 1000000000L && retries < 10) {
            vTaskDelay(pdMS_TO_TICKS(1000));
            retries++;
            printf("等待时间同步... (%d/10)\n", retries);
        }
        
        if (time(nullptr) > 1000000000L) {
            doc["success"] = true;
            doc["message"] = "时间同步成功";
            
            time_t now = time(nullptr);
            struct tm timeinfo;
            localtime_r(&now, &timeinfo);
            doc["currentTime"] = asctime(&timeinfo);
            doc["timestamp"] = now;
            
            printf("时间同步成功: %s\n", asctime(&timeinfo));
        } else {
            doc["success"] = false;
            doc["message"] = "时间同步超时";
            printf("时间同步失败\n");
        }
    } else {
        doc["success"] = false;
        doc["message"] = "设备未连接WiFi，无法同步时间";
        printf("设备未连接WiFi，无法同步时间\n");
    }
    
    String response;
    serializeJson(doc, response);
    server->send(200, "application/json", response);
}

// 服务器OTA升级相关API处理函数
void WebServerManager::handleServerOTAStart() {
    printf("处理服务器OTA启动请求\n");
    
    DynamicJsonDocument doc(512);
    
    if (!wifiManager->isConnected()) {
        doc["success"] = false;
        doc["message"] = "设备未连接WiFi，无法进行服务器OTA升级";
        printf("设备未连接WiFi，无法进行服务器OTA升级\n");
        String response;
        serializeJson(doc, response);
        server->send(400, "application/json", response);
        return;
    }
    
    // 默认服务器地址
    String serverUrl = "http://ota.dlcv.com.cn";
    String firmwareFile = "";
    
    // 检查是否有自定义参数
    if (server->hasArg("serverUrl")) {
        serverUrl = server->arg("serverUrl");
    }
    
    if (server->hasArg("firmwareFile")) {
        firmwareFile = server->arg("firmwareFile");
    }
    
    printf("服务器OTA升级参数 - 服务器: %s, 固件文件: %s\n", 
           serverUrl.c_str(), firmwareFile.c_str());
    
    // 首先检查服务器版本并进行版本比较
    printf("开始版本检查...\n");
    String versionJson = otaManager->checkServerFirmwareVersion(serverUrl);
    
    if (versionJson.length() == 0) {
        doc["success"] = false;
        doc["message"] = "无法获取服务器版本信息，升级已取消";
        printf("无法获取服务器版本信息，升级已取消\n");
        String response;
        serializeJson(doc, response);
        server->send(400, "application/json", response);
        return;
    }
    
    // 解析服务器版本响应
    DynamicJsonDocument serverDoc(512);
    if (deserializeJson(serverDoc, versionJson) != DeserializationError::Ok) {
        doc["success"] = false;
        doc["message"] = "服务器版本信息解析失败，升级已取消";
        printf("服务器版本信息解析失败，升级已取消\n");
        String response;
        serializeJson(doc, response);
        server->send(400, "application/json", response);
        return;
    }
    
    // 获取服务器版本号
    String serverVersion = serverDoc["version"].as<String>();
    if (serverVersion.length() == 0) {
        doc["success"] = false;
        doc["message"] = "服务器版本号为空，升级已取消";
        printf("服务器版本号为空，升级已取消\n");
        String response;
        serializeJson(doc, response);
        server->send(400, "application/json", response);
        return;
    }
    
    // 进行版本比较
    printf("进行版本比较: 当前版本=%s, 服务器版本=%s\n", 
           VERSION_STRING, serverVersion.c_str());
    
    if (!otaManager->needsUpdate(serverVersion)) {
        // 检查版本比较结果
        int compareResult = otaManager->compareVersions(VERSION_STRING, serverVersion);
        
        if (compareResult == 0) {
            // 版本相同
            doc["success"] = false;
            doc["message"] = "当前版本已是最新版本，无需升级";
            doc["currentVersion"] = VERSION_STRING;
            doc["serverVersion"] = serverVersion;
            doc["versionStatus"] = "up_to_date";
            printf("版本相同，无需升级: %s\n", VERSION_STRING);
        } else if (compareResult > 0) {
            // 当前版本更高
            doc["success"] = false;
            doc["message"] = "当前版本比服务器版本更新，无需升级";
            doc["currentVersion"] = VERSION_STRING;
            doc["serverVersion"] = serverVersion;
            doc["versionStatus"] = "newer";
            printf("当前版本更高，无需升级: %s > %s\n", VERSION_STRING, serverVersion.c_str());
        }
        
        String response;
        serializeJson(doc, response);
        server->send(200, "application/json", response);
        return;
    }
    
    printf("版本检查通过，开始升级: %s -> %s\n", VERSION_STRING, serverVersion.c_str());
    
    // 启动服务器OTA升级
    if (otaManager->downloadAndUpdateFromServer(serverUrl, firmwareFile)) {
        doc["success"] = true;
        doc["message"] = "服务器OTA升级已启动";
        doc["serverUrl"] = serverUrl;
        doc["firmwareFile"] = firmwareFile;
        
        printf("服务器OTA升级启动成功\n");
        
        String response;
        serializeJson(doc, response);
        server->send(200, "application/json", response);
    } else {
        doc["success"] = false;
        doc["message"] = "服务器OTA升级启动失败";
        doc["error"] = otaManager->getError();
        
        printf("服务器OTA升级启动失败: %s\n", otaManager->getError().c_str());
        
        String response;
        serializeJson(doc, response);
        server->send(500, "application/json", response);
    }
}

void WebServerManager::handleServerOTAStatus() {
    printf("处理服务器OTA状态查询请求\n");
    
    // 获取OTA状态JSON，已经包含所有必要信息
    String statusJson = otaManager->getStatusJSON();
    server->send(200, "application/json", statusJson);
}

void WebServerManager::handleServerFirmwareList() {
    printf("处理服务器固件列表请求\n");
    
    DynamicJsonDocument doc(1024);
    
    if (!wifiManager->isConnected()) {
        doc["success"] = false;
        doc["message"] = "设备未连接WiFi，无法获取服务器固件列表";
        printf("设备未连接WiFi，无法获取服务器固件列表\n");
        String response;
        serializeJson(doc, response);
        server->send(400, "application/json", response);
        return;
    }
    
    // 默认服务器地址
    String serverUrl = "http://ota.dlcv.com.cn";
    
    // 检查是否有自定义服务器地址
    if (server->hasArg("serverUrl")) {
        serverUrl = server->arg("serverUrl");
    }
    
    printf("从服务器获取固件列表: %s\n", serverUrl.c_str());
    
    // 获取服务器固件列表
    String firmwareListJson = otaManager->getServerFirmwareList(serverUrl);
    
    if (firmwareListJson.length() > 0) {
        // 解析服务器响应
        DynamicJsonDocument serverDoc(1024);
        if (deserializeJson(serverDoc, firmwareListJson) == DeserializationError::Ok) {
            doc["success"] = true;
            doc["message"] = "固件列表获取成功";
            doc["serverUrl"] = serverUrl;
            doc["firmwareList"] = serverDoc;
            
            printf("固件列表获取成功\n");
        } else {
            doc["success"] = false;
            doc["message"] = "解析服务器响应失败";
            doc["rawResponse"] = firmwareListJson;
            
            printf("解析服务器响应失败\n");
        }
    } else {
        doc["success"] = false;
        doc["message"] = "无法连接到服务器或服务器响应为空";
        doc["serverUrl"] = serverUrl;
        
        printf("无法连接到服务器或服务器响应为空\n");
    }
    
    String response;
    serializeJson(doc, response);
    server->send(200, "application/json", response);
}

void WebServerManager::handleServerFirmwareVersion() {
    printf("处理服务器固件版本查询请求\n");
    
    DynamicJsonDocument doc(512);
    
    if (!wifiManager->isConnected()) {
        doc["success"] = false;
        doc["message"] = "设备未连接WiFi，无法查询服务器固件版本";
        printf("设备未连接WiFi，无法查询服务器固件版本\n");
        String response;
        serializeJson(doc, response);
        server->send(400, "application/json", response);
        return;
    }
    
    // 默认服务器地址
    String serverUrl = "http://ota.dlcv.com.cn";
    
    // 检查是否有自定义服务器地址
    if (server->hasArg("serverUrl")) {
        serverUrl = server->arg("serverUrl");
    }
    
    printf("从服务器查询固件版本: %s\n", serverUrl.c_str());
    
    // 查询服务器固件版本
    String versionJson = otaManager->checkServerFirmwareVersion(serverUrl);
    
    if (versionJson.length() > 0) {
        // 解析服务器响应
        DynamicJsonDocument serverDoc(512);
        if (deserializeJson(serverDoc, versionJson) == DeserializationError::Ok) {
            // 获取服务器版本号进行比较
            String serverVersionStr = serverDoc["version"].as<String>();
            
            doc["success"] = true;
            doc["message"] = "固件版本查询成功";
            doc["serverUrl"] = serverUrl;
            doc["currentVersion"] = VERSION_STRING;  // 当前设备版本
            doc["serverVersion"] = serverDoc;
            
            // 添加版本比较结果
            if (serverVersionStr.length() > 0) {
                int compareResult = otaManager->compareVersions(VERSION_STRING, serverVersionStr);
                bool needsUpdate = otaManager->needsUpdate(serverVersionStr);
                
                if (compareResult == 0) {
                    doc["versionStatus"] = "up_to_date";
                    doc["versionMessage"] = "当前版本已是最新版本";
                    doc["canUpdate"] = false;
                } else if (compareResult > 0) {
                    doc["versionStatus"] = "newer";
                    doc["versionMessage"] = "当前版本比服务器版本更新";
                    doc["canUpdate"] = false;
                } else {
                    doc["versionStatus"] = "needs_update";
                    doc["versionMessage"] = "发现新版本，可以升级";
                    doc["canUpdate"] = true;
                }
                
                doc["needsUpdate"] = needsUpdate;
                
                printf("版本比较结果: 当前=%s, 服务器=%s, 状态=%s\n", 
                       VERSION_STRING, serverVersionStr.c_str(), 
                       doc["versionStatus"].as<String>().c_str());
            } else {
                doc["versionStatus"] = "unknown";
                doc["versionMessage"] = "无法获取服务器版本号";
                doc["canUpdate"] = false;
                doc["needsUpdate"] = false;
            }
            
            printf("固件版本查询成功\n");
        } else {
            doc["success"] = false;
            doc["message"] = "解析服务器响应失败";
            doc["rawResponse"] = versionJson;
            
            printf("解析服务器响应失败\n");
        }
    } else {
        doc["success"] = false;
        doc["message"] = "无法连接到服务器或服务器响应为空";
        doc["serverUrl"] = serverUrl;
        
        printf("无法连接到服务器或服务器响应为空\n");
    }
    
    String response;
    serializeJson(doc, response);
    server->send(200, "application/json", response);
}

// 服务器设置相关API处理函数
void WebServerManager::handleServerSettingsPage() {
    printf("处理服务器设置页面请求\n");
    server->send(200, "text/html", getServerSettingsHTML());
}

void WebServerManager::handleGetServerConfig() {
    printf("处理获取服务器配置请求\n");
    
    DynamicJsonDocument doc(512);
    
    if (configStorage) {
        bool hasConfig = configStorage->hasServerConfigAsync(3000);
        
        if (hasConfig) {
            String serverUrl;
            int requestInterval;
            bool enabled;
            int connectionTimeout;
            bool autoGetData;
            bool autoScanServer;
            
            bool success = configStorage->loadServerConfigAsync(serverUrl, requestInterval, enabled, connectionTimeout, autoGetData, autoScanServer, 3000);
            if (success) {
                doc["success"] = true;
                doc["config"]["serverUrl"] = serverUrl;
                doc["config"]["requestInterval"] = requestInterval;
                doc["config"]["enabled"] = enabled;
                doc["config"]["connectionTimeout"] = connectionTimeout;
                doc["config"]["autoGetData"] = autoGetData;
                doc["config"]["autoScanServer"] = autoScanServer;
                doc["message"] = "服务器配置获取成功";
                
                printf("服务器配置获取成功：URL=%s, 间隔=%d分钟, 启用=%s, 超时=%d秒, 自动获取数据=%s, 自动扫描服务器=%s\n",
                       serverUrl.c_str(), requestInterval,
                       enabled ? "是" : "否", connectionTimeout,
                       autoGetData ? "是" : "否", autoScanServer ? "是" : "否");
            } else {
                doc["success"] = false;
                doc["message"] = "服务器配置加载失败";
                printf("服务器配置加载失败\n");
            }
        } else {
            // 返回默认配置
            doc["success"] = true;
            doc["config"]["serverUrl"] = "http://10.10.168.168/metrics.json";
            doc["config"]["requestInterval"] = 250;
            doc["config"]["enabled"] = true;
            doc["config"]["connectionTimeout"] = 1000;
            doc["config"]["autoGetData"] = true;
            doc["config"]["autoScanServer"] = false;
            doc["message"] = "返回默认服务器配置";
            
            printf("返回默认服务器配置\n");
        }
    } else {
        doc["success"] = false;
        doc["message"] = "配置存储未初始化";
        printf("配置存储未初始化\n");
    }
    
    String response;
    serializeJson(doc, response);
    server->send(200, "application/json", response);
}

void WebServerManager::handleSetServerConfig() {
    printf("处理设置服务器配置请求\n");
    
    DynamicJsonDocument doc(256);
    
    if (!configStorage) {
        doc["success"] = false;
        doc["message"] = "配置存储未初始化";
        printf("配置存储未初始化\n");
        String response;
        serializeJson(doc, response);
        server->send(400, "application/json", response);
        return;
    }
    
    // 检查必要参数
    if (!server->hasArg("serverUrl")) {
        doc["success"] = false;
        doc["message"] = "缺少serverUrl参数";
        printf("缺少serverUrl参数\n");
        String response;
        serializeJson(doc, response);
        server->send(400, "application/json", response);
        return;
    }
    
    String serverUrl = server->arg("serverUrl");
    int requestInterval = server->hasArg("requestInterval") ? server->arg("requestInterval").toInt() : 250;
    bool enabled = server->hasArg("enabled") ? (server->arg("enabled") == "true") : true;
    int connectionTimeout = server->hasArg("connectionTimeout") ? server->arg("connectionTimeout").toInt() : 1000;
    bool autoGetData = server->hasArg("autoGetData") ? (server->arg("autoGetData") == "true") : true;
    bool autoScanServer = server->hasArg("autoScanServer") ? (server->arg("autoScanServer") == "true") : false;
    
    printf("接收到服务器配置：URL=%s, 间隔=%d毫秒, 启用=%s, 超时=%d毫秒, 自动获取数据=%s, 自动扫描服务器=%s\n",
           serverUrl.c_str(), requestInterval,
           enabled ? "是" : "否", connectionTimeout,
           autoGetData ? "是" : "否", autoScanServer ? "是" : "否");
    
    // 参数验证
    if (serverUrl.length() < 10 || 
        (!serverUrl.startsWith("http://") && !serverUrl.startsWith("https://"))) {
        doc["success"] = false;
        doc["message"] = "无效的服务器URL";
        printf("无效的服务器URL: %s\n", serverUrl.c_str());
        String response;
        serializeJson(doc, response);
        server->send(400, "application/json", response);
        return;
    }
    
    if (requestInterval < 100 || requestInterval > 1000) {
        doc["success"] = false;
        doc["message"] = "请求间隔必须在100-1000毫秒之间";
        printf("无效的请求间隔: %d\n", requestInterval);
        String response;
        serializeJson(doc, response);
        server->send(400, "application/json", response);
        return;
    }
    
    if (connectionTimeout < 1000 || connectionTimeout > 60000) {
        doc["success"] = false;
        doc["message"] = "连接超时时间必须在1000-60000毫秒之间";
        printf("无效的连接超时时间: %d\n", connectionTimeout);
        String response;
        serializeJson(doc, response);
        server->send(400, "application/json", response);
        return;
    }
    
    // 保存配置
    bool success = configStorage->saveServerConfigAsync(serverUrl, requestInterval, enabled, connectionTimeout, autoGetData, autoScanServer, 3000);
    
    doc["success"] = success;
    if (success) {
        doc["message"] = "服务器配置保存成功";
        printf("服务器配置保存成功\n");
    } else {
        doc["message"] = "服务器配置保存失败";
        printf("服务器配置保存失败\n");
    }
    
    String response;
    serializeJson(doc, response);
    server->send(200, "application/json", response);
}

void WebServerManager::handleTestServerConnection() {
    printf("处理测试服务器连接请求\n");
    
    DynamicJsonDocument doc(512);
    
    if (!wifiManager->isConnected()) {
        doc["success"] = false;
        doc["message"] = "设备未连接WiFi，无法测试服务器连接";
        printf("设备未连接WiFi，无法测试服务器连接\n");
        String response;
        serializeJson(doc, response);
        server->send(400, "application/json", response);
        return;
    }
    
    String serverUrl = "";
    if (server->hasArg("serverUrl")) {
        serverUrl = server->arg("serverUrl");
    } else {
        // 从配置中获取服务器URL
        if (configStorage) {
            bool hasConfig = configStorage->hasServerConfigAsync(3000);
            if (hasConfig) {
                String configServerUrl;
                int requestInterval;
                bool enabled;
                int connectionTimeout;
                bool autoGetData;
                bool autoScanServer;
                bool success = configStorage->loadServerConfigAsync(configServerUrl, requestInterval, enabled, connectionTimeout, autoGetData, autoScanServer, 3000);
                if (success) {
                    serverUrl = configServerUrl;
                }
            }
        }
        
        if (serverUrl.isEmpty()) {
            serverUrl = "http://10.10.168.168/metrics.json";
        }
    }
    
    printf("测试服务器连接: %s\n", serverUrl.c_str());
    
    // 使用WiFiClient测试连接
    WiFiClient client;
    HTTPClient http;
    
    http.begin(client, serverUrl);
    http.setTimeout(10000);  // 10秒超时
    
    int httpCode = http.GET();
    
    if (httpCode > 0) {
        String payload = http.getString();
        
        doc["success"] = true;
        doc["message"] = "服务器连接测试成功";
        doc["serverUrl"] = serverUrl;
        doc["httpCode"] = httpCode;
        doc["responseSize"] = payload.length();
        
        // 尝试解析JSON响应
        DynamicJsonDocument responseDoc(1024);
        if (deserializeJson(responseDoc, payload) == DeserializationError::Ok) {
            doc["responseValid"] = true;
            doc["responseData"] = responseDoc;
        } else {
            doc["responseValid"] = false;
            doc["responsePreview"] = payload.substring(0, 200);
        }
        
        printf("服务器连接测试成功，HTTP状态码: %d, 响应大小: %d字节\n", httpCode, payload.length());
    } else {
        doc["success"] = false;
        doc["message"] = "服务器连接测试失败";
        doc["serverUrl"] = serverUrl;
        doc["error"] = http.errorToString(httpCode);
        
        printf("服务器连接测试失败，错误: %s\n", http.errorToString(httpCode).c_str());
    }
    
    http.end();
    
    String response;
    serializeJson(doc, response);
    server->send(200, "application/json", response);
}

void WebServerManager::handleGetServerData() {
    printf("处理获取服务器数据请求\n");
    
    DynamicJsonDocument doc(1024);
    
    if (!wifiManager->isConnected()) {
        doc["success"] = false;
        doc["message"] = "设备未连接WiFi，无法获取服务器数据";
        printf("设备未连接WiFi，无法获取服务器数据\n");
        String response;
        serializeJson(doc, response);
        server->send(400, "application/json", response);
        return;
    }
    
    String serverUrl = "";
    if (server->hasArg("serverUrl")) {
        serverUrl = server->arg("serverUrl");
    } else {
        // 从配置中获取服务器URL
        if (configStorage) {
            bool hasConfig = configStorage->hasServerConfigAsync(3000);
            if (hasConfig) {
                String configServerUrl;
                int requestInterval;
                bool enabled;
                int connectionTimeout;
                bool autoGetData;
                bool autoScanServer;
                bool success = configStorage->loadServerConfigAsync(configServerUrl, requestInterval, enabled, connectionTimeout, autoGetData, autoScanServer, 3000);
                if (success && enabled) {
                    serverUrl = configServerUrl;
                }
            }
        }
        
        if (serverUrl.isEmpty()) {
            serverUrl = "http://10.10.168.168/metrics.json";
        }
    }
    
    printf("获取服务器数据: %s\n", serverUrl.c_str());
    
    // 使用WiFiClient获取数据
    WiFiClient client;
    HTTPClient http;
    
    http.begin(client, serverUrl);
    http.setTimeout(10000);  // 10秒超时
    
    int httpCode = http.GET();
    
    if (httpCode == HTTP_CODE_OK) {
        String payload = http.getString();
        
        // 尝试解析JSON响应
        DynamicJsonDocument responseDoc(1024);
        if (deserializeJson(responseDoc, payload) == DeserializationError::Ok) {
            doc["success"] = true;
            doc["message"] = "服务器数据获取成功";
            doc["serverUrl"] = serverUrl;
            doc["timestamp"] = millis();
            doc["data"] = responseDoc;
            
            printf("服务器数据获取成功，数据大小: %d字节\n", payload.length());
        } else {
            doc["success"] = false;
            doc["message"] = "服务器响应不是有效的JSON格式";
            doc["serverUrl"] = serverUrl;
            doc["responsePreview"] = payload.substring(0, 200);
            
            printf("服务器响应不是有效的JSON格式\n");
        }
    } else {
        doc["success"] = false;
        doc["message"] = "服务器数据获取失败";
        doc["serverUrl"] = serverUrl;
        doc["httpCode"] = httpCode;
        doc["error"] = http.errorToString(httpCode);
        
        printf("服务器数据获取失败，HTTP状态码: %d, 错误: %s\n", httpCode, http.errorToString(httpCode).c_str());
    }
    
    http.end();
    
    String response;
    serializeJson(doc, response);
    server->send(200, "application/json", response);
}

void WebServerManager::handleMDNSScanServers() {
    printf("处理mDNS扫描服务器请求\n");
    
    DynamicJsonDocument doc(2048);
    
    if (!wifiManager->isConnected()) {
        doc["success"] = false;
        doc["message"] = "设备未连接WiFi，无法进行mDNS扫描";
        printf("设备未连接WiFi，无法进行mDNS扫描\n");
        String response;
        serializeJson(doc, response);
        server->send(400, "application/json", response);
        return;
    }
    
    printf("开始mDNS扫描小电拼...\n");
    
    // 使用关键词搜索小电拼
    std::vector<String> keywords;
    keywords.push_back("cp02");
    keywords.push_back("CP02");
    
    // 扫描包含cp02关键词的设备
    std::vector<MDNSDeviceInfo> devices = UniversalMDNSScanner::findDevicesByKeywords(keywords, true);
    
    doc["success"] = true;
    doc["message"] = "小电拼扫描完成";
    doc["scanTime"] = millis();
    doc["deviceCount"] = devices.size();
    
    // 创建设备列表
    JsonArray deviceArray = doc.createNestedArray("devices");
    
    for (const auto& device : devices) {
        JsonObject deviceObj = deviceArray.createNestedObject();
        deviceObj["hostname"] = device.hostname;
        deviceObj["ip"] = device.ip;
        deviceObj["name"] = device.name;
        deviceObj["port"] = device.port;
        deviceObj["serviceType"] = device.serviceType;
        deviceObj["isValid"] = device.isValid;
        deviceObj["customInfo"] = device.customInfo;
        
        // 生成完整的服务器URL
        String serverUrl = "http://" + device.ip;
        if (device.port != 80) {
            serverUrl += ":" + String(device.port);
        }
        serverUrl += "/metrics.json";
        deviceObj["serverUrl"] = serverUrl;
        
        printf("发现小电拼: %s (%s:%d)\n", device.name.c_str(), device.ip.c_str(), device.port);
    }
    
    if (deviceArray.size() == 0) {
        doc["message"] = "未发现任何小电拼设备";
        printf("未发现任何小电拼设备\n");
    } else {
        printf("小电拼扫描完成，发现 %d 个设备\n", deviceArray.size());
    }
    
    String response;
    serializeJson(doc, response);
    server->send(200, "application/json", response);
}

// 定位相关API处理函数
void WebServerManager::handleGetLocationData() {
    printf("处理获取定位数据请求\n");
    
    if (!m_locationManager) {
        server->send(500, "application/json", "{\"success\":false,\"message\":\"定位管理器未初始化\"}");
        return;
    }
    
    LocationData locationData = m_locationManager->getCurrentLocation();
    
    DynamicJsonDocument doc(1024);
    doc["success"] = true;
    
    if (locationData.isValid) {
        doc["data"]["province"] = locationData.province;
        doc["data"]["city"] = locationData.city;
        doc["data"]["district"] = locationData.district;
        doc["data"]["adcode"] = locationData.adcode;
        doc["data"]["citycode"] = locationData.citycode;
        doc["data"]["rectangle"] = locationData.rectangle;
        doc["data"]["timestamp"] = locationData.timestamp;
        doc["data"]["locationString"] = m_locationManager->getLocationString();
        doc["data"]["weatherCityCode"] = m_locationManager->getCityCodeForWeather();
        doc["message"] = "定位数据获取成功";
    } else {
        doc["data"] = nullptr;
        doc["message"] = "暂无有效的定位数据";
    }
    
    String response;
    serializeJson(doc, response);
    server->send(200, "application/json", response);
}

void WebServerManager::handleSetLocationApiKey() {
    printf("处理设置定位API密钥请求\n");
    
    if (!m_locationManager) {
        server->send(500, "application/json", "{\"success\":false,\"message\":\"定位管理器未初始化\"}");
        return;
    }
    
    if (!server->hasArg("apiKey")) {
        server->send(400, "application/json", "{\"success\":false,\"message\":\"缺少API密钥参数\"}");
        return;
    }
    
    String apiKey = server->arg("apiKey");
    
    if (m_locationManager->setApiKey(apiKey)) {
        printf("定位API密钥设置成功\n");
        server->send(200, "application/json", "{\"success\":true,\"message\":\"定位API密钥设置成功\"}");
    } else {
        printf("定位API密钥设置失败\n");
        server->send(500, "application/json", "{\"success\":false,\"message\":\"定位API密钥设置失败\"}");
    }
}

void WebServerManager::handleLocationNow() {
    printf("处理立即定位请求\n");
    
    if (!m_locationManager) {
        server->send(500, "application/json", "{\"success\":false,\"message\":\"定位管理器未初始化\"}");
        return;
    }
    
    bool result = m_locationManager->locateCurrentPosition();
    
    if (result) {
        // 等待一小段时间让定位完成
        vTaskDelay(pdMS_TO_TICKS(2000));
        
        LocationData locationData = m_locationManager->getCurrentLocation();
        
        DynamicJsonDocument doc(1024);
        doc["success"] = true;
        
        if (locationData.isValid) {
            doc["location"]["province"] = locationData.province;
            doc["location"]["city"] = locationData.city;
            doc["location"]["district"] = locationData.district;
            doc["location"]["adcode"] = locationData.adcode;
            doc["location"]["citycode"] = locationData.citycode;
            doc["location"]["locationString"] = m_locationManager->getLocationString();
            doc["location"]["weatherCityCode"] = m_locationManager->getCityCodeForWeather();
            doc["message"] = "定位成功";
            
            // 自动更新天气配置的城市信息
            if (m_weatherManager && !locationData.citycode.isEmpty()) {
                WeatherConfig config = m_weatherManager->getConfig();
                config.cityCode = locationData.citycode;
                config.cityName = locationData.city;
                
                // 构建完整的位置字符串
                String fullLocation = locationData.province;
                if (!locationData.city.isEmpty() && locationData.city != locationData.province) {
                    fullLocation += " " + locationData.city;
                }
                if (!locationData.district.isEmpty() && locationData.district != locationData.city) {
                    fullLocation += " " + locationData.district;
                }
                
                if (m_weatherManager->setConfig(config)) {
                    // 保存完整位置信息到配置存储
                    if (configStorage) {
                        configStorage->putStringAsync("weather_fullLocation", fullLocation);
                    }
                    doc["weatherUpdated"] = true;
                    doc["message"] = "定位成功，天气配置已自动更新";
                    printf("天气配置已自动更新: %s (%s)\n", fullLocation.c_str(), locationData.citycode.c_str());
                } else {
                    doc["weatherUpdated"] = false;
                    printf("天气配置更新失败\n");
                }
            }
        } else {
            doc["location"] = nullptr;
            doc["message"] = "定位请求已发送，请稍后查看结果";
        }
        
        String response;
        serializeJson(doc, response);
        server->send(200, "application/json", response);
        
        printf("定位请求处理完成\n");
    } else {
        server->send(500, "application/json", "{\"success\":false,\"message\":\"定位请求失败\"}");
        printf("定位请求失败\n");
    }
} 