/*
 * WebServerManager.cpp - Web服务器管理器类实现文件
 * ESP32S3监控项目 - Web服务器模块
 */

#include "WebServerManager.h"
#include "Arduino.h"

WebServerManager::WebServerManager(WiFiManager* wifiMgr, ConfigStorage* configStore, OTAManager* otaMgr, FileManager* fileMgr) :
    server(nullptr),
    wifiManager(wifiMgr),
    configStorage(configStore),
    otaManager(otaMgr),
    fileManager(fileMgr),
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
    server->on("/ota-upload", HTTP_POST, [this]() { 
        // POST请求完成后的响应处理
        printf("OTA上传POST请求完成，发送响应\n");
        server->send(200, "application/json", otaManager->getStatusJSON());
    }, [this]() { handleOTAUpload(); });
    server->on("/ota-status", HTTP_GET, [this]() { handleOTAStatus(); });
    server->on("/ota-reboot", HTTP_POST, [this]() { handleOTAReboot(); });
    
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
    doc["version"] = "v3.3.25";
    doc["chipModel"] = ESP.getChipModel();
    doc["chipRevision"] = ESP.getChipRevision();
    doc["cpuFreq"] = ESP.getCpuFreqMHz();
    doc["flashSize"] = ESP.getFlashChipSize();
    doc["freeHeap"] = ESP.getFreeHeap();
    doc["totalHeap"] = ESP.getHeapSize();
    doc["uptime"] = millis();
    
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
    if (configStorage->loadWiFiConfigs(wifiConfigs)) {
        printf("成功加载WiFi配置，开始构建响应\n");
        for (int i = 0; i < ConfigStorage::MAX_WIFI_CONFIGS; i++) {
            if (wifiConfigs[i].isValid && wifiConfigs[i].ssid.length() > 0) {
                JsonObject config = configs.createNestedObject();
                config["index"] = i;
                config["ssid"] = wifiConfigs[i].ssid;
                config["password"] = "***"; // 不返回实际密码，只显示占位符
                config["status"] = "saved";
                printf("添加配置 %d: %s\n", i, wifiConfigs[i].ssid.c_str());
            }
        }
    } else {
        printf("加载WiFi配置失败\n");
    }
    
    int configCount = configStorage->getWiFiConfigCount();
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
    
    // 加载现有配置
    WiFiConfig configs[3];
    printf("开始加载WiFi配置...\n");
    if (configStorage->loadWiFiConfigs(configs)) {
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
            
            // 重新保存配置
            printf("开始保存更新后的配置...\n");
            bool success = configStorage->saveWiFiConfigs(configs);
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
    html += "display: flex;";
    html += "gap: 12px;";
    html += "flex-wrap: wrap;";
    html += "margin-bottom: 24px;";
    html += "}";
    html += ".primary-btn, .danger-btn, .success-btn {";
    html += "padding: 12px 16px;";
    html += "border: none;";
    html += "border-radius: 12px;";
    html += "font-weight: 500;";
    html += "cursor: pointer;";
    html += "transition: all 0.3s ease;";
    html += "display: flex;";
    html += "align-items: center;";
    html += "justify-content: center;";
    html += "font-size: 0.9rem;";
    html += "text-decoration: none;";
    html += "}";
    html += ".primary-btn {";
    html += "background: linear-gradient(135deg, #3b82f6, #1d4ed8);";
    html += "color: white;";
    html += "}";
    html += ".primary-btn:hover {";
    html += "transform: translateY(-2px);";
    html += "box-shadow: 0 8px 25px rgba(59, 130, 246, 0.4);";
    html += "}";
    html += ".success-btn {";
    html += "background: linear-gradient(135deg, #10b981, #059669);";
    html += "color: white;";
    html += "}";
    html += ".success-btn:hover {";
    html += "transform: translateY(-2px);";
    html += "box-shadow: 0 8px 25px rgba(16, 185, 129, 0.4);";
    html += "}";
    html += ".danger-btn {";
    html += "background: linear-gradient(135deg, #ef4444, #dc2626);";
    html += "color: white;";
    html += "}";
    html += ".danger-btn:hover {";
    html += "transform: translateY(-2px);";
    html += "box-shadow: 0 8px 25px rgba(239, 68, 68, 0.4);";
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
    html += "<h1>文件管理器</h1>";
    html += "<div class='subtitle'>ESP32S3 Monitor - 文件系统管理</div>";
    html += "</div>";
    
    html += "<div class='card'>";
    html += "<div class='action-buttons'>";
    html += "<button class='primary-btn' onclick='refreshFiles()'>刷新</button>";
    html += "<button class='success-btn' onclick='showCreateModal()'>新建文件</button>";
    html += "<button class='primary-btn' onclick=\"document.getElementById('fileInput').click()\">上传文件</button>";
    html += "<button class='danger-btn' onclick='formatFileSystem()'>格式化</button>";
    html += "<a href='/' class='primary-btn'>返回首页</a>";
    html += "</div>";
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
    html += "html+='<button class=\"file-btn file-btn-primary\" onclick=\"downloadFile(\\'' + f.name + '\\')\">[下载]</button>';";
    html += "html+='<button class=\"file-btn file-btn-success\" onclick=\"showRenameModal(\\'' + f.name + '\\')\">[重命名]</button>';";
    html += "}";
    html += "html+='<button class=\"file-btn file-btn-danger\" onclick=\"deleteFile(\\'' + f.name + '\\')\">[删除]</button>';";
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