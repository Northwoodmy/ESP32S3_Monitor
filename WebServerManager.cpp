/*
 * WebServerManager.cpp - Web服务器管理器类实现文件
 * ESP32S3监控项目 - Web服务器模块
 */

#include "WebServerManager.h"
#include "Arduino.h"

WebServerManager::WebServerManager(WiFiManager* wifiMgr, ConfigStorage* configStore) :
    server(nullptr),
    wifiManager(wifiMgr),
    configStorage(configStore),
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
    server->on("/scan", [this]() { handleWiFiScan(); });
    server->on("/info", [this]() { handleSystemInfo(); });
    server->on("/restart", [this]() { handleRestart(); });
    server->on("/reset", [this]() { handleResetConfig(); });
    server->on("/api", [this]() { handleAPI(); });
    server->on("/save", HTTP_POST, [this]() { handleSaveWiFi(); });
    server->on("/status", [this]() { handleGetStatus(); });
    server->on("/wifi-configs", [this]() { handleGetWiFiConfigs(); });
    server->on("/wifi-configs", HTTP_DELETE, [this]() { handleDeleteWiFiConfig(); });
    server->on("/connect-wifi", HTTP_POST, [this]() { handleConnectWiFiConfig(); });
    server->onNotFound([this]() { handleNotFound(); });
    
    printf("Web服务器路由配置完成\n");
}

void WebServerManager::start() {
    if (isRunning) {
        printf("Web服务器已经在运行中\n");
        return;
    }
    
    printf("启动Web服务器...\n");
    
    server->begin();
    isRunning = true;
    
    // 创建服务器处理任务
    BaseType_t result = xTaskCreate(
        serverTask,
        "WebServerTask",
        4096,
        this,
        2,
        &serverTaskHandle
    );
    
    if (result == pdPASS) {
        printf("Web服务器任务创建成功\n");
        printf("Web服务器启动成功，端口：80\n");
    } else {
        isRunning = false;
        printf("Web服务器任务创建失败\n");
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
    doc["version"] = "v3.0.1";
    doc["chipModel"] = ESP.getChipModel();
    doc["chipRevision"] = ESP.getChipRevision();
    doc["cpuFreq"] = ESP.getCpuFreqMHz();
    doc["flashSize"] = ESP.getFlashChipSize();
    doc["freeHeap"] = ESP.getFreeHeap();
    doc["totalHeap"] = ESP.getHeapSize();
    doc["uptime"] = millis();
    
    if (wifiManager->isConnected()) {
        doc["wifi"]["status"] = "connected";
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
    
    // 执行配置重置
    bool success = configStorage->resetAllConfig();
    
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
        
        // 检查配置是否已保存
        bool configSaved = configStorage->hasWiFiConfig();
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
    
    DynamicJsonDocument doc(1024);
    JsonArray configs = doc.createNestedArray("configs");
    
    WiFiConfig wifiConfigs[3];
    if (configStorage->loadWiFiConfigs(wifiConfigs)) {
        for (int i = 0; i < ConfigStorage::MAX_WIFI_CONFIGS; i++) {
            if (wifiConfigs[i].isValid && wifiConfigs[i].ssid.length() > 0) {
                JsonObject config = configs.createNestedObject();
                config["index"] = i;
                config["ssid"] = wifiConfigs[i].ssid;
                config["password"] = "***"; // 不返回实际密码，只显示占位符
                config["status"] = "saved";
            }
        }
    }
    
    doc["count"] = configStorage->getWiFiConfigCount();
    doc["maxConfigs"] = ConfigStorage::MAX_WIFI_CONFIGS;
    
    String response;
    serializeJson(doc, response);
    server->send(200, "application/json", response);
}

void WebServerManager::handleDeleteWiFiConfig() {
    printf("处理删除WiFi配置请求\n");
    
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
    
    // 加载现有配置
    WiFiConfig configs[3];
    if (configStorage->loadWiFiConfigs(configs)) {
        if (configs[index].isValid) {
            printf("删除WiFi配置 %d: %s\n", index, configs[index].ssid.c_str());
            
            // 清除指定配置
            configs[index] = WiFiConfig();
            
            // 重新保存配置
            bool success = configStorage->saveWiFiConfigs(configs);
            
            doc["success"] = success;
            if (success) {
                doc["message"] = "WiFi配置删除成功";
            } else {
                doc["message"] = "WiFi配置删除失败";
            }
        } else {
            doc["success"] = false;
            doc["message"] = "指定的配置不存在";
        }
    } else {
        doc["success"] = false;
        doc["message"] = "加载WiFi配置失败";
    }
    
    String response;
    serializeJson(doc, response);
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
    
    // 加载WiFi配置
    WiFiConfig configs[3];
    if (configStorage->loadWiFiConfigs(configs)) {
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