/*
 * WebServerManager_UI1.cpp - Web服务器UI基础页面实现
 * ESP32S3监控项目 - 主页面和基础样式模块
 */

#include "WebServerManager.h"


String WebServerManager::getIndexHTML() {
    String html = "<!DOCTYPE html>\n";
    html += "<html lang=\"zh-CN\">\n";
    html += "<head>\n";
    html += "    <meta charset=\"UTF-8\">\n";
    html += "    <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\n";
    html += "    <title>ESP32S3 WiFi配置</title>\n";
    html += "    <style>\n";
    html += getCSS();
    html += "    </style>\n";
    html += "</head>\n";
    html += "<body>\n";
    html += "    <div class=\"container\">\n";
    html += "        <header class=\"header\">\n";
    html += "            <h1>小屏幕配置</h1>\n";
    html += "        </header>\n";
    html += "        \n";
    html += "        <div class=\"status-card\" id=\"statusCard\">\n";
    html += "            <div class=\"status-indicator\" id=\"statusIndicator\">\n";
    html += "                <div class=\"pulse\"></div>\n";
    html += "            </div>\n";
    html += "            <div class=\"status-info\">\n";
    html += "                <h3 id=\"statusTitle\">正在连接...</h3>\n";
    html += "                <p id=\"currentWiFi\" class=\"current-wifi hidden\"></p>\n";
    html += "                <p id=\"statusDetail\">检查设备状态中</p>\n";
    html += "            </div>\n";
    html += "        </div>\n";
    html += "        \n";
    html += "        <!-- 标签页导航 -->\n";
    html += "        <div class=\"tab-nav\">\n";
    html += "            <button class=\"tab-btn active\" onclick=\"switchTab('wifi')\" id=\"wifiTab\">\n";
    html += "                WiFi配置\n";
    html += "            </button>\n";
    html += "            <button class=\"tab-btn\" onclick=\"switchTab('screen')\" id=\"screenTab\">\n";
    html += "                屏幕配置\n";
    html += "            </button>\n";
    html += "            <button class=\"tab-btn\" onclick=\"switchTab('system')\" id=\"systemTab\">\n";
    html += "                系统信息\n";
    html += "            </button>\n";
    html += "        </div>\n";
    html += "        \n";
    html += "        <!-- WiFi配置标签页 -->\n";
    html += "        <div class=\"tab-content active\" id=\"wifiContent\">\n";
    html += "            <div class=\"card\">\n";
    html += "                <h2>已保存的WiFi配置</h2>\n";
    html += "                <div class=\"saved-wifi-section\">\n";
    html += "                    <div id=\"savedWiFiList\" class=\"saved-wifi-list\">\n";
    html += "                        <div class=\"loading\" id=\"savedWiFiLoading\">\n";
    html += "                            <div class=\"spinner\"></div>\n";
    html += "                            <span>加载中...</span>\n";
    html += "                        </div>\n";
    html += "                    </div>\n";
    html += "                </div>\n";
    html += "            </div>\n";
    html += "            \n";
    html += "            <div class=\"card\">\n";
    html += "                <h2>添加新的WiFi配置</h2>\n";
    html += "                <div class=\"wifi-section\">\n";
    html += "                    <button id=\"scanBtn\" class=\"scan-btn\">\n";
    html += "                        扫描WiFi网络\n";
    html += "                    </button>\n";
    html += "                    \n";
    html += "                    <div id=\"networkList\" class=\"network-list hidden\">\n";
    html += "                    </div>\n";
    html += "                    \n";
    html += "                    <form id=\"wifiForm\" class=\"wifi-form\">\n";
    html += "                        <div class=\"form-group\">\n";
    html += "                            <label for=\"ssid\">网络名称 (SSID)</label>\n";
    html += "                            <input type=\"text\" id=\"ssid\" name=\"ssid\" required placeholder=\"请输入WiFi名称\">\n";
    html += "                        </div>\n";
    html += "                        \n";
    html += "                        <div class=\"form-group\">\n";
    html += "                            <label for=\"password\">密码</label>\n";
    html += "                            <input type=\"password\" id=\"password\" name=\"password\" placeholder=\"请输入WiFi密码\">\n";
    html += "                        </div>\n";
    html += "                        \n";
    html += "                        <button type=\"submit\" class=\"connect-btn\" id=\"connectBtn\">\n";
    html += "                            <span class=\"btn-text\">连接并保存WiFi</span>\n";
    html += "                            <div class=\"btn-loading hidden\">\n";
    html += "                                <div class=\"spinner-sm\"></div>\n";
    html += "                            </div>\n";
    html += "                        </button>\n";
    html += "                    </form>\n";
    html += "                </div>\n";
    html += "            </div>\n";
    html += "        </div>\n";
    html += "        \n";
    html += "        <!-- 屏幕配置标签页 -->\n";
    html += "        <div class=\"tab-content\" id=\"screenContent\">\n";
    html += "            <div class=\"card\">\n";
    html += "                <h2>屏幕亮度控制</h2>\n";
    html += "                <div class=\"screen-config-section\">\n";
    html += "                    <div class=\"brightness-control\">\n";
    html += "                        <div class=\"slider-container\">\n";
    html += "                            <label for=\"brightnessSlider\" class=\"slider-label\">亮度级别:</label>\n";
    html += "                            <div class=\"slider-wrapper\">\n";
    html += "                                <input type=\"range\" id=\"brightnessSlider\" class=\"brightness-slider\" \n";
    html += "                                       min=\"10\" max=\"255\" value=\"128\" \n";
    html += "                                       oninput=\"updateBrightnessValue(this.value)\"\n";
    html += "                                       onchange=\"setBrightness(this.value)\">\n";
    html += "                                <div class=\"slider-track\"></div>\n";
    html += "                            </div>\n";
    html += "                            <div class=\"brightness-display\">\n";
    html += "                                <span id=\"brightnessValue\">50%</span>\n";
    html += "                            </div>\n";
    html += "                        </div>\n";
    html += "                        <div class=\"brightness-presets\">\n";
    html += "                            <h3>快速设置</h3>\n";
    html += "                            <div class=\"preset-buttons\">\n";
    html += "                                <button class=\"preset-btn\" onclick=\"setPresetBrightness(25)\">低 (10%)</button>\n";
    html += "                                <button class=\"preset-btn\" onclick=\"setPresetBrightness(128)\">中 (50%)</button>\n";
    html += "                                <button class=\"preset-btn\" onclick=\"setPresetBrightness(204)\">高 (80%)</button>\n";
    html += "                                <button class=\"preset-btn\" onclick=\"setPresetBrightness(255)\">最高 (100%)</button>\n";
    html += "                            </div>\n";
    html += "                        </div>\n";
    html += "                    </div>\n";
    html += "                </div>\n";
    html += "            </div>\n";
    html += "            \n";
    html += "            <div class=\"card\">\n";
    html += "                <h2>屏幕信息</h2>\n";
    html += "                <div class=\"screen-settings\">\n";
    html += "                    <div class=\"setting-item\">\n";
    html += "                        <label class=\"setting-label\">当前亮度:</label>\n";
    html += "                        <span class=\"value\" id=\"currentBrightness\">加载中...</span>\n";
    html += "                    </div>\n";
    html += "                    <div class=\"setting-item\">\n";
    html += "                        <label class=\"setting-label\">最大亮度:</label>\n";
    html += "                        <span class=\"value\" id=\"screenStatus\">加载中...</span>\n";
    html += "                    </div>\n";
    html += "                    <div class=\"setting-item\">\n";
    html += "                        <label class=\"setting-label\">背光控制:</label>\n";
    html += "                        <span class=\"value\" id=\"backlightStatus\">加载中...</span>\n";
    html += "                    </div>\n";
    html += "                    <div class=\"setting-item\">\n";
    html += "                        <label class=\"setting-label\">屏幕分辨率:</label>\n";
    html += "                        <span class=\"value\" id=\"screenResolution\">加载中...</span>\n";
    html += "                    </div>\n";
    html += "                    <div class=\"setting-item\">\n";
    html += "                        <label class=\"setting-label\">色彩支持:</label>\n";
    html += "                        <span class=\"value\" id=\"colorSupport\">加载中...</span>\n";
    html += "                    </div>\n";
    html += "                    <div class=\"setting-item\">\n";
    html += "                        <label class=\"setting-label\">屏幕材质:</label>\n";
    html += "                        <span class=\"value\" id=\"screenMaterial\">加载中...</span>\n";
    html += "                    </div>\n";
    html += "                </div>\n";
    html += "            </div>\n";
    html += "        </div>\n";
    html += "        \n";
    html += "        <!-- 系统信息标签页 -->\n";
    html += "        <div class=\"tab-content\" id=\"systemContent\">\n";
    html += "            <div class=\"card\">\n";
    html += "                <h2>设备信息</h2>\n";
    html += "                <div class=\"info-grid\" id=\"systemInfo\">\n";
    html += "                    <div class=\"info-item\">\n";
    html += "                        <span class=\"label\">设备型号:</span>\n";
    html += "                        <span class=\"value\" id=\"deviceModel\">加载中...</span>\n";
    html += "                    </div>\n";
    html += "                    <div class=\"info-item\">\n";
    html += "                        <span class=\"label\">固件版本:</span>\n";
    html += "                        <span class=\"value\" id=\"firmwareVersion\">v6.0.1</span>\n";
    html += "                    </div>\n";
    html += "                    <div class=\"info-item\">\n";
    html += "                        <span class=\"label\">CPU频率:</span>\n";
    html += "                        <span class=\"value\" id=\"cpuFreq\">加载中...</span>\n";
    html += "                    </div>\n";
    html += "                    <div class=\"info-item\">\n";
    html += "                        <span class=\"label\">Flash大小:</span>\n";
    html += "                        <span class=\"value\" id=\"flashSize\">加载中...</span>\n";
    html += "                    </div>\n";
    html += "                    <div class=\"info-item\">\n";
    html += "                        <span class=\"label\">SRAM内存:</span>\n";
    html += "                        <span class=\"value\" id=\"sramInfo\">加载中...</span>\n";
    html += "                    </div>\n";
    html += "                    <div class=\"info-item\" id=\"psramInfoItem\" style=\"display:none;\">\n";
    html += "                        <span class=\"label\">PSRAM内存:</span>\n";
    html += "                        <span class=\"value\" id=\"psramInfo\">不可用</span>\n";
    html += "                    </div>\n";
    html += "                    <div class=\"info-item\">\n";
    html += "                        <span class=\"label\">运行时间:</span>\n";
    html += "                        <span class=\"value\" id=\"uptime\">加载中...</span>\n";
    html += "                    </div>\n";
    html += "                    <div class=\"info-item\" id=\"wifiInfoItem\">\n";
    html += "                        <span class=\"label\">当前WiFi:</span>\n";
    html += "                        <span class=\"value\" id=\"currentWiFiName\">未连接</span>\n";
    html += "                    </div>\n";
    html += "                </div>\n";
    html += "            </div>\n";
    html += "            \n";
    html += "            <div class=\"card\">\n";
    html += "                <h2>设备管理</h2>\n";
    html += "                <div class=\"action-buttons\">\n";
    html += "                    <button onclick=\"refreshInfo()\" class=\"refresh-btn\">\n";
    html += "                        刷新信息\n";
    html += "                    </button>\n";
    html += "                    <button onclick=\"window.location.href='/settings'\" class=\"settings-btn\">\n";
    html += "                        时间设置\n";
    html += "                    </button>\n";
    html += "                    <button onclick=\"window.location.href='/weather-settings'\" class=\"weather-btn\">\n";
    html += "                        天气设置\n";
    html += "                    </button>\n";
    html += "                    <button onclick=\"window.location.href='/server-settings'\" class=\"settings-btn\">\n";
    html += "                        服务器设置\n";
    html += "                    </button>\n";
    html += "                    <button onclick=\"window.location.href='/screen-settings'\" class=\"settings-btn\">\n";
    html += "                        屏幕设置\n";
    html += "                    </button>\n";
    html += "                    <button onclick=\"window.location.href='/files'\" class=\"files-btn\">\n";
    html += "                        文件管理器\n";
    html += "                    </button>\n";
    html += "                    <button onclick=\"window.location.href='/ota'\" class=\"ota-btn\">\n";
    html += "                        固件升级\n";
    html += "                    </button>\n";
    html += "                    <button onclick=\"rebootDevice()\" class=\"reboot-btn\">\n";
    html += "                        重启设备\n";
    html += "                    </button>\n";
    html += "                    <button onclick=\"resetConfig()\" class=\"reset-btn\">\n";
    html += "                        恢复默认配置\n";
    html += "                    </button>\n";
    html += "                </div>\n";
    html += "            </div>\n";
    html += "        </div>\n";
    html += "    </div>\n";
    html += "    \n";
    html += "    <div id=\"toast\" class=\"toast hidden\">\n";
    html += "        <div class=\"toast-content\">\n";
    html += "            <span id=\"toastMessage\"></span>\n";
    html += "        </div>\n";
    html += "    </div>\n";
    html += "    \n";
    html += "    <script>\n";
    html += getJavaScript();
    html += "    </script>\n";
    html += "</body>\n";
    html += "</html>\n";
    return html;
}

String WebServerManager::getOTAPageHTML() {
    String html = "";
    html += "<!DOCTYPE html>";
    html += "<html lang=\"zh-CN\">";
    html += "<head>";
    html += "<meta charset=\"UTF-8\">";
    html += "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">";
    html += "<title>固件升级 - ESP32S3 Monitor</title>";
    html += "<style>";
    html += getCSS();
    html += getOTAPageCSS();
    html += "</style>";
    html += "</head>";
    html += "<body>";
    html += "<div class=\"container\">";
    html += "<header class=\"header\">";
    html += "<h1>小屏幕配置</h1>";
    html += "<div class=\"subtitle\">固件升级</div>";
    html += "</header>";
    html += "<div class=\"card\">";
    html += "<button onclick=\"window.location.href='/'\" class=\"back-home-btn\">返回首页</button>";
    html += "<div class=\"ota-section\">";
    html += "<div class=\"ota-info\">";
    html += "<p class=\"info-text\">选择升级方式</p>";
    html += "<p class=\"warning-text\">升级过程中请勿断电或关闭页面</p>";
    html += "</div>";
    html += "<div class=\"ota-tabs\">";
    html += "<button class=\"tab-btn active\" data-tab=\"local\">本地文件</button>";
    html += "<button class=\"tab-btn\" data-tab=\"server\">服务器升级</button>";
    html += "</div>";
    html += "<div class=\"tab-content active\" id=\"localTab\">";
    html += "<div class=\"file-upload-section\">";
    html += "<input type=\"file\" id=\"firmwareFile\" accept=\".bin\" style=\"display: none;\">";
    html += "<button id=\"selectFileBtn\" class=\"file-select-btn\">选择固件文件</button>";
    html += "<div id=\"fileInfo\" class=\"file-info hidden\">";
    html += "<p><strong>文件名:</strong> <span id=\"fileName\"></span></p>";
    html += "<p><strong>文件大小:</strong> <span id=\"fileSize\"></span></p>";
    html += "</div>";
    html += "</div>";
    html += "<div class=\"upload-section\">";
    html += "<button id=\"uploadBtn\" class=\"upload-btn\" disabled>";
    html += "<span class=\"btn-text\">开始升级</span>";
    html += "<div class=\"btn-loading hidden\"><div class=\"spinner-sm\"></div></div>";
    html += "</button>";
    html += "</div>";
    html += "</div>";
    html += "<div class=\"tab-content\" id=\"serverTab\">";
    html += "<div class=\"server-ota-section\">";
    html += "<div class=\"server-info\">";
    html += "<p class=\"server-text\">从服务器下载最新固件</p>";
    html += "</div>";
    html += "<div class=\"server-actions\">";
    html += "<button id=\"checkVersionBtn\" class=\"server-btn version-btn\">检查版本</button>";
    html += "<button id=\"serverOTABtn\" class=\"server-btn ota-btn\">开始服务器升级</button>";
    html += "</div>";
    html += "<div id=\"versionInfo\" class=\"version-info hidden\">";
    html += "<h4>版本信息</h4>";
    html += "<p><strong>当前版本:</strong> <span id=\"currentVersion\">v5.4.0</span></p>";
    html += "<p><strong>服务器版本:</strong> <span id=\"serverVersion\">检查中...</span></p>";
    html += "</div>";
    html += "</div>";
    html += "</div>";
    html += "<div id=\"otaProgress\" class=\"ota-progress hidden\">";
    html += "<div class=\"progress-info\">";
    html += "<h3 id=\"otaStatusTitle\">准备升级...</h3>";
    html += "<p id=\"otaStatusDetail\"></p>";
    html += "</div>";
    html += "<div class=\"progress-bar\">";
    html += "<div id=\"progressFill\" class=\"progress-fill\"></div>";
    html += "</div>";
    html += "<div class=\"progress-text\">";
    html += "<span id=\"progressPercent\">0%</span>";
    html += "<span id=\"progressSize\">0 / 0 字节</span>";
    html += "</div>";
    html += "</div>";
    html += "<div id=\"otaResult\" class=\"ota-result hidden\">";
    html += "<div class=\"result-success hidden\" id=\"successResult\">";
    html += "<h3>升级成功！</h3>";
    html += "<p>固件已成功升级，设备将自动重启并刷新页面。</p>";
    html += "</div>";
    html += "<div class=\"result-error hidden\" id=\"errorResult\">";
    html += "<h3>升级失败</h3>";
    html += "<p id=\"errorMessage\">升级过程中发生错误</p>";
    html += "<button id=\"retryBtn\" class=\"retry-btn\">重新尝试</button>";
    html += "</div>";
    html += "</div>";
    html += "</div>";
    html += "</div>";
    html += "</div>";
    html += "<div id=\"toast\" class=\"toast hidden\">";
    html += "<div class=\"toast-content\">";
    html += "<span id=\"toastMessage\"></span>";
    html += "</div>";
    html += "</div>";
    html += "<script>";
    html += getOTAJavaScript();
    html += "</script>";
    html += "</body>";
    html += "</html>";
    
    return html;
}

String WebServerManager::getCSS() {
    return R"(
        * {
            margin: 0;
            padding: 0;
            box-sizing: border-box;
        }
        
        body {
            font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, Oxygen, Ubuntu, Cantarell, sans-serif;
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            min-height: 100vh;
            color: #333;
        }
        
        .container {
            max-width: 600px;
            margin: 0 auto;
            padding: 20px;
        }
        
        .header {
            text-align: center;
            color: white;
            margin-bottom: 30px;
        }
        
        .header h1 {
            font-size: 2.5rem;
            font-weight: 700;
            margin: 0;
            text-shadow: 0 2px 4px rgba(0,0,0,0.3);
        }
        
        .back-home-btn {
            position: absolute;
            top: 16px;
            right: 16px;
            background: linear-gradient(135deg, #3b82f6, #1d4ed8);
            color: white;
            border: none;
            padding: 12px 20px;
            border-radius: 12px;
            font-size: 1rem;
            font-weight: 600;
            cursor: pointer;
            transition: all 0.3s ease;
            display: inline-flex;
            align-items: center;
            justify-content: center;
            white-space: nowrap;
            z-index: 10;
            box-shadow: 0 2px 8px rgba(59, 130, 246, 0.3);
        }
        
        .back-home-btn:hover {
            background: linear-gradient(135deg, #2563eb, #1e40af);
            transform: translateY(-2px);
            box-shadow: 0 4px 16px rgba(59, 130, 246, 0.4);
        }
        
        .subtitle {
            font-size: 1.1rem;
            opacity: 0.9;
        }
        
        .card {
            position: relative;
            background: white;
            border-radius: 16px;
            padding: 24px;
            padding-top: 70px;
            margin-bottom: 24px;
            box-shadow: 0 8px 32px rgba(0,0,0,0.1);
            backdrop-filter: blur(10px);
        }
        
        .status-card {
            background: rgba(255,255,255,0.95);
            border-radius: 16px;
            padding: 20px;
            margin-bottom: 24px;
            display: flex;
            align-items: center;
            box-shadow: 0 8px 32px rgba(0,0,0,0.1);
        }
        
        .status-indicator {
            position: relative;
            width: 48px;
            height: 48px;
            border-radius: 50%;
            margin-right: 16px;
            display: flex;
            align-items: center;
            justify-content: center;
        }
        
        .status-connected {
            background: #10b981;
        }
        
        .status-disconnected {
            background: #ef4444;
        }
        
        .status-ap {
            background: #f59e0b;
        }
        
        .pulse {
            width: 24px;
            height: 24px;
            background: white;
            border-radius: 50%;
        }
        
        .current-wifi {
            margin-top: 8px;
            font-weight: 600;
            color: #059669;
            font-size: 0.95rem;
        }
        
        .current-wifi.hidden {
            display: none;
        }
        
        .status-connected .pulse {
            animation: pulse-green 2s infinite;
        }
        
        .status-disconnected .pulse {
            animation: pulse-red 2s infinite;
        }
        
        .status-ap .pulse {
            animation: pulse-orange 2s infinite;
        }
        
        @keyframes pulse-green {
            0% { box-shadow: 0 0 0 0 rgba(16, 185, 129, 0.7); }
            70% { box-shadow: 0 0 0 10px rgba(16, 185, 129, 0); }
            100% { box-shadow: 0 0 0 0 rgba(16, 185, 129, 0); }
        }
        
        @keyframes pulse-red {
            0% { box-shadow: 0 0 0 0 rgba(239, 68, 68, 0.7); }
            70% { box-shadow: 0 0 0 10px rgba(239, 68, 68, 0); }
            100% { box-shadow: 0 0 0 0 rgba(239, 68, 68, 0); }
        }
        
        @keyframes pulse-orange {
            0% { box-shadow: 0 0 0 0 rgba(245, 158, 11, 0.7); }
            70% { box-shadow: 0 0 0 10px rgba(245, 158, 11, 0); }
            100% { box-shadow: 0 0 0 0 rgba(245, 158, 11, 0); }
        }
        
        .status-info h3 {
            font-size: 1.25rem;
            margin-bottom: 4px;
        }
        
        .status-info p {
            color: #666;
            font-size: 0.9rem;
        }
        
        /* 标签页导航样式 */
        .tab-nav, .ota-tabs {
            display: flex;
            background: rgba(255,255,255,0.95);
            border-radius: 16px;
            padding: 6px;
            margin-bottom: 24px;
            box-shadow: 0 8px 32px rgba(0,0,0,0.1);
        }
        
        .tab-btn {
            flex: 1;
            background: transparent;
            border: none;
            padding: 12px 20px;
            border-radius: 12px;
            font-size: 1rem;
            font-weight: 500;
            color: #6b7280;
            cursor: pointer;
            display: flex;
            align-items: center;
            justify-content: center;
            transition: all 0.3s ease;
        }
        
        .tab-btn.active {
            background: linear-gradient(135deg, #3b82f6, #1d4ed8);
            color: white;
            box-shadow: 0 4px 12px rgba(59, 130, 246, 0.4);
        }
        
        .tab-btn:hover:not(.active) {
            background: #f3f4f6;
            color: #374151;
        }
        

        
        /* 标签页内容样式 */
        .tab-content {
            display: none;
        }
        
        .tab-content.active {
            display: block;
            animation: fadeIn 0.3s ease-in-out;
        }
        
        @keyframes fadeIn {
            from {
                opacity: 0;
                transform: translateY(10px);
            }
            to {
                opacity: 1;
                transform: translateY(0);
            }
        }
        
        h2 {
            color: #1f2937;
            margin-bottom: 20px;
            font-size: 1.5rem;
            font-weight: 600;
        }
        
        .scan-btn {
            width: 100%;
            background: linear-gradient(135deg, #3b82f6, #1d4ed8);
            color: white;
            border: none;
            padding: 12px 20px;
            border-radius: 12px;
            font-size: 1rem;
            font-weight: 500;
            cursor: pointer;
            display: flex;
            align-items: center;
            justify-content: center;
            gap: 8px;
            transition: all 0.3s ease;
            margin-bottom: 20px;
        }
        
        .scan-btn:hover {
            transform: translateY(-2px);
            box-shadow: 0 8px 25px rgba(59, 130, 246, 0.4);
        }
        
        .scan-icon {
            font-size: 1.2rem;
        }
        
        .network-list {
            margin-bottom: 20px;
            max-height: 300px;
            overflow-y: auto;
        }
        
        /* 已保存WiFi配置样式 */
        .saved-wifi-list {
            margin-bottom: 20px;
        }
        
        .saved-wifi-item {
            background: #f8fafc;
            border: 2px solid #e2e8f0;
            border-radius: 12px;
            padding: 16px;
            margin-bottom: 12px;
            display: flex;
            justify-content: space-between;
            align-items: center;
            transition: all 0.2s ease;
        }
        
        .saved-wifi-item:hover {
            background: #f1f5f9;
            border-color: #cbd5e1;
        }
        
        .saved-wifi-info {
            flex: 1;
            display: flex;
            flex-direction: column;
            gap: 4px;
        }
        
        .saved-wifi-name {
            font-weight: 600;
            color: #1f2937;
            font-size: 1.2rem;
            line-height: 1.4;
        }
        
        .saved-wifi-priority {
            color: #6b7280;
            font-size: 0.875rem;
            font-weight: 500;
        }
        
        .saved-wifi-details {
            color: #6b7280;
            font-size: 0.875rem;
        }
        
        .saved-wifi-actions {
            display: flex;
            align-items: center;
            gap: 12px;
            flex-wrap: wrap;
        }
        
        .priority-controls {
            display: flex;
            align-items: center;
            gap: 6px;
            margin-right: 8px;
            background: #f3f4f6;
            padding: 6px 10px;
            border-radius: 8px;
            border: 1px solid #e5e7eb;
        }
        
        .priority-controls label {
            font-size: 0.875rem;
            color: #6b7280;
            font-weight: 500;
            margin: 0;
        }
        
        .priority-select {
            padding: 4px 8px;
            border: 1px solid #d1d5db;
            border-radius: 6px;
            font-size: 0.875rem;
            background: white;
            min-width: 50px;
            cursor: pointer;
            transition: all 0.2s ease;
        }
        
        .priority-select:hover {
            border-color: #3b82f6;
        }
        
        .priority-select:focus {
            outline: none;
            border-color: #3b82f6;
            box-shadow: 0 0 0 3px rgba(59, 130, 246, 0.1);
        }
        
        .delete-btn, .connect-btn-small {
            border: none;
            padding: 8px 12px;
            border-radius: 8px;
            font-size: 0.875rem;
            cursor: pointer;
            transition: all 0.2s ease;
            color: white;
        }
        
        .delete-btn {
            background: #ef4444;
        }
        
        .delete-btn:hover {
            background: #dc2626;
            transform: translateY(-1px);
        }
        
        .connect-btn-small {
            background: #10b981;
        }
        
        .connect-btn-small:hover {
            background: #059669;
            transform: translateY(-1px);
        }
        
        .connect-btn-small:disabled {
            background: #9ca3af;
            cursor: not-allowed;
            transform: none;
        }
        
        .priority-badge {
            background: #f59e0b;
            color: white;
            padding: 6px 10px;
            border-radius: 8px;
            font-size: 0.8rem;
            font-weight: 600;
            white-space: nowrap;
            box-shadow: 0 2px 4px rgba(245, 158, 11, 0.2);
        }
        
        .empty-wifi-message {
            text-align: center;
            color: #6b7280;
            padding: 40px 20px;
            background: #f9fafb;
            border-radius: 12px;
            border: 2px dashed #d1d5db;
        }
        
        .network-item {
            background: #f8fafc;
            border: 2px solid transparent;
            border-radius: 12px;
            padding: 12px 16px;
            margin-bottom: 8px;
            cursor: pointer;
            display: flex;
            justify-content: space-between;
            align-items: center;
            transition: all 0.2s ease;
        }
        
        .network-item:hover {
            background: #e2e8f0;
            border-color: #3b82f6;
        }
        
        .network-item.selected {
            background: #dbeafe;
            border-color: #3b82f6;
        }
        
        .network-name {
            font-weight: 500;
            color: #1f2937;
        }
        
        .network-info {
            display: flex;
            align-items: center;
            gap: 8px;
            color: #6b7280;
            font-size: 0.875rem;
        }
        
        .signal-strength {
            display: flex;
            gap: 2px;
            align-items: flex-end;
            height: 20px;
        }
        
        .signal-bar {
            width: 3px;
            background: #d1d5db;
            border-radius: 1px;
        }
        
        .signal-bar.active {
            background: #10b981;
        }
        
        .form-group {
            margin-bottom: 20px;
            position: relative;
        }
        
        label {
            display: block;
            margin-bottom: 8px;
            font-weight: 500;
            color: #374151;
        }
        
        input[type="text"], input[type="password"] {
            width: 100%;
            padding: 12px 16px;
            border: 2px solid #e5e7eb;
            border-radius: 12px;
            font-size: 1rem;
            transition: all 0.3s ease;
            background: #f9fafb;
        }
        
        input[type="text"]:focus, input[type="password"]:focus {
            outline: none;
            border-color: #3b82f6;
            background: white;
            box-shadow: 0 0 0 3px rgba(59, 130, 246, 0.1);
        }
        
        .password-toggle {
            position: absolute;
            right: 12px;
            top: 36px;
            cursor: pointer;
            font-size: 1.2rem;
            color: #6b7280;
            user-select: none;
        }
        
        .connect-btn {
            width: 100%;
            background: linear-gradient(135deg, #10b981, #059669);
            color: white;
            border: none;
            padding: 14px 20px;
            border-radius: 12px;
            font-size: 1.1rem;
            font-weight: 600;
            cursor: pointer;
            position: relative;
            transition: all 0.3s ease;
        }
        
        .connect-btn:hover:not(:disabled) {
            transform: translateY(-2px);
            box-shadow: 0 8px 25px rgba(16, 185, 129, 0.4);
        }
        
        .connect-btn:disabled {
            opacity: 0.6;
            cursor: not-allowed;
        }
        
        .info-grid {
            display: grid;
            gap: 16px;
            margin-bottom: 24px;
        }
        
        .info-item {
            display: flex;
            justify-content: space-between;
            align-items: center;
            padding: 12px 0;
            border-bottom: 1px solid #e5e7eb;
        }
        
        .info-item:last-child {
            border-bottom: none;
        }
        
        .label {
            font-weight: 500;
            color: #374151;
        }
        
        .value {
            color: #6b7280;
            font-family: 'Courier New', monospace;
            text-align: right !important;
            flex-shrink: 0;
        }
        
        .info-item .value {
            text-align: right !important;
            display: block;
            width: auto;
        }
        
        #sramInfo, #psramInfo {
            text-align: right !important;
        }
        
        .action-buttons {
            display: grid;
            grid-template-columns: 1fr 1fr 1fr;
            gap: 12px;
        }
        
        .primary-btn, .danger-btn, .warning-btn, .secondary-btn, .success-btn {
            padding: 12px 16px;
            border: none;
            border-radius: 12px;
            font-weight: 500;
            cursor: pointer;
            transition: all 0.3s ease;
            display: flex;
            align-items: center;
            justify-content: center;
            font-size: 0.9rem;
        }
        
        .primary-btn {
            background: linear-gradient(135deg, #3b82f6, #1d4ed8);
            color: white;
        }
        
        .primary-btn:hover {
            transform: translateY(-2px);
            box-shadow: 0 8px 25px rgba(59, 130, 246, 0.4);
        }
        
        .danger-btn {
            background: linear-gradient(135deg, #ef4444, #dc2626);
            color: white;
        }
        
        .danger-btn:hover {
            transform: translateY(-2px);
            box-shadow: 0 8px 25px rgba(239, 68, 68, 0.4);
        }
        
        .warning-btn {
            background: linear-gradient(135deg, #f59e0b, #d97706);
            color: white;
        }
        
        .warning-btn:hover {
            transform: translateY(-2px);
            box-shadow: 0 8px 25px rgba(245, 158, 11, 0.4);
        }
        
        .secondary-btn {
            background: #f3f4f6;
            color: #374151;
        }
        
        .secondary-btn:hover {
            background: #e5e7eb;
            transform: translateY(-2px);
        }
        
        .success-btn {
            background: linear-gradient(135deg, #10b981, #059669);
            color: white;
        }
        
        .success-btn:hover {
            transform: translateY(-2px);
            box-shadow: 0 8px 25px rgba(16, 185, 129, 0.4);
        }
        
        /* 设备管理按钮 - 统一基础样式和字体大小 */
        .refresh-btn, .settings-btn, .weather-btn, .files-btn, .ota-btn, .reboot-btn, .reset-btn {
            padding: 12px 16px;
            border: none;
            border-radius: 12px;
            font-weight: 500;
            font-size: 1rem;
            cursor: pointer;
            transition: all 0.3s ease;
            display: flex;
            align-items: center;
            justify-content: center;
            color: white;
        }
        
        .refresh-btn, .settings-btn, .weather-btn, .files-btn, .ota-btn, .reboot-btn, .reset-btn {
            transform: translateY(0);
        }
        
        .refresh-btn:hover, .settings-btn:hover, .weather-btn:hover, .files-btn:hover, 
        .ota-btn:hover, .reboot-btn:hover, .reset-btn:hover {
            transform: translateY(-2px);
        }
        
        .refresh-btn {
            background: linear-gradient(135deg, #06b6d4, #0891b2);
        }
        
        .refresh-btn:hover {
            box-shadow: 0 8px 25px rgba(6, 182, 212, 0.4);
        }
        
        .settings-btn {
            background: linear-gradient(135deg, #3b82f6, #1d4ed8);
        }
        
        .settings-btn:hover {
            box-shadow: 0 8px 25px rgba(59, 130, 246, 0.4);
        }
        
        .weather-btn {
            background: linear-gradient(135deg, #14b8a6, #0d9488);
        }
        
        .weather-btn:hover {
            box-shadow: 0 8px 25px rgba(20, 184, 166, 0.4);
        }
        
        .files-btn {
            background: linear-gradient(135deg, #10b981, #059669);
        }
        
        .files-btn:hover {
            box-shadow: 0 8px 25px rgba(16, 185, 129, 0.4);
        }
        
        .ota-btn {
            background: linear-gradient(135deg, #8b5cf6, #7c3aed);
        }
        
        .ota-btn:hover {
            box-shadow: 0 8px 25px rgba(139, 92, 246, 0.4);
        }
        
        .reboot-btn {
            background: linear-gradient(135deg, #f59e0b, #d97706);
        }
        
        .reboot-btn:hover {
            box-shadow: 0 8px 25px rgba(245, 158, 11, 0.4);
        }
        
        .reset-btn {
            background: linear-gradient(135deg, #ef4444, #dc2626);
        }
        
        .reset-btn:hover {
            box-shadow: 0 8px 25px rgba(239, 68, 68, 0.4);
        }

        
        .toast {
            position: fixed;
            top: 20px;
            right: 20px;
            background: white;
            border-radius: 12px;
            padding: 16px 20px;
            box-shadow: 0 8px 32px rgba(0,0,0,0.15);
            transform: translateX(400px);
            transition: transform 0.3s ease;
            z-index: 1000;
        }
        
        .toast.show {
            transform: translateX(0);
        }
        
        .toast.success {
            border-left: 4px solid #10b981;
        }
        
        .toast.error {
            border-left: 4px solid #ef4444;
        }
        
        .loading {
            display: flex;
            align-items: center;
            justify-content: center;
            gap: 12px;
            padding: 20px;
            color: #6b7280;
        }
        
        .spinner, .spinner-sm {
            border: 2px solid #e5e7eb;
            border-top: 2px solid #3b82f6;
            border-radius: 50%;
            animation: spin 1s linear infinite;
        }
        
        .spinner {
            width: 20px;
            height: 20px;
        }
        
        .spinner-sm {
            width: 16px;
            height: 16px;
        }
        
        @keyframes spin {
            0% { transform: rotate(0deg); }
            100% { transform: rotate(360deg); }
        }
        
        .hidden {
            display: none !important;
        }
        
        .btn-loading {
            display: flex;
            align-items: center;
            justify-content: center;
            gap: 8px;
        }
        
        .btn-loading .spinner-sm {
            border-color: rgba(255,255,255,0.3);
            border-top-color: white;
        }
        
        .btn-loading.absolute {
            position: absolute;
            top: 50%;
            left: 50%;
            transform: translate(-50%, -50%);
        }
        
        /* OTA升级相关样式 */
        .ota-section {
            text-align: center;
        }
        
        .ota-info {
            margin-bottom: 24px;
            padding: 20px;
            background: #f0f9ff;
            border-radius: 12px;
            border: 2px solid #0ea5e9;
        }
        
        .info-text {
            color: #0c4a6e;
            font-size: 1.1rem;
            font-weight: 500;
            margin-bottom: 8px;
        }
        
        .warning-text {
            color: #dc2626;
            font-size: 0.95rem;
            font-weight: 500;
        }
        
        .file-upload-section {
            margin-bottom: 24px;
        }
        
        .file-select-btn {
            background: linear-gradient(135deg, #6366f1, #4f46e5);
            color: white;
            border: none;
            padding: 14px 24px;
            border-radius: 12px;
            font-size: 1.1rem;
            font-weight: 600;
            cursor: pointer;
            transition: all 0.3s ease;
            margin-bottom: 16px;
        }
        
        .file-select-btn:hover {
            transform: translateY(-2px);
            box-shadow: 0 8px 25px rgba(99, 102, 241, 0.4);
        }
        
        .file-info {
            background: #f8fafc;
            border: 2px solid #e2e8f0;
            border-radius: 12px;
            padding: 16px;
            text-align: left;
        }
        
        .file-info p {
            margin-bottom: 8px;
            color: #374151;
        }
        
        .file-info p:last-child {
            margin-bottom: 0;
        }
        
        .upload-section {
            margin-bottom: 24px;
        }
        
        .upload-btn {
            background: linear-gradient(135deg, #10b981, #059669);
            color: white;
            border: none;
            padding: 16px 32px;
            border-radius: 12px;
            font-size: 1.2rem;
            font-weight: 600;
            cursor: pointer;
            position: relative;
            transition: all 0.3s ease;
            min-width: 200px;
        }
        
        .upload-btn:hover:not(:disabled) {
            transform: translateY(-2px);
            box-shadow: 0 8px 25px rgba(16, 185, 129, 0.4);
        }
        
        .upload-btn:disabled {
            background: #9ca3af;
            cursor: not-allowed;
            transform: none;
        }
        
        .ota-progress {
            background: #f8fafc;
            border: 2px solid #e2e8f0;
            border-radius: 12px;
            padding: 24px;
            margin-bottom: 24px;
        }
        
        .progress-info {
            margin-bottom: 20px;
        }
        
        .progress-info h3 {
            color: #1f2937;
            margin-bottom: 8px;
            font-size: 1.25rem;
        }
        
        .progress-info p {
            color: #6b7280;
            font-size: 0.95rem;
        }
        
        .progress-bar {
            width: 100%;
            height: 12px;
            background: #e5e7eb;
            border-radius: 6px;
            overflow: hidden;
            margin-bottom: 12px;
        }
        
        .progress-fill {
            height: 100%;
            background: linear-gradient(135deg, #10b981, #059669);
            border-radius: 6px;
            transition: width 0.3s ease;
            width: 0%;
        }
        
        .progress-text {
            display: flex;
            justify-content: space-between;
            align-items: center;
            font-size: 0.875rem;
            color: #6b7280;
        }
        
        .ota-result {
            padding: 24px;
            border-radius: 12px;
            text-align: center;
        }
        
        .result-success {
            background: #f0fdf4;
            border: 2px solid #22c55e;
            border-radius: 12px;
        }
        
        .result-success h3 {
            color: #166534;
            margin-bottom: 12px;
        }
        
        .result-success p {
            color: #15803d;
            margin-bottom: 20px;
        }
        
        .result-error {
            background: #fef2f2;
            border: 2px solid #ef4444;
            border-radius: 12px;
        }
        
        .result-error h3 {
            color: #991b1b;
            margin-bottom: 12px;
        }
        
        .result-error p {
            color: #dc2626;
            margin-bottom: 20px;
        }
        
        .reboot-btn, .retry-btn {
            background: linear-gradient(135deg, #f59e0b, #d97706);
            color: white;
            border: none;
            padding: 12px 24px;
            border-radius: 12px;
            font-size: 1rem;
            font-weight: 600;
            cursor: pointer;
            transition: all 0.3s ease;
        }
        
        .reboot-btn:hover, .retry-btn:hover {
            transform: translateY(-2px);
            box-shadow: 0 8px 25px rgba(245, 158, 11, 0.4);
        }
        
        .retry-btn {
            background: linear-gradient(135deg, #3b82f6, #1d4ed8);
        }
        
        .retry-btn:hover {
            box-shadow: 0 8px 25px rgba(59, 130, 246, 0.4);
        }
        
        /* 屏幕配置相关样式 */
        .screen-config-section {
            padding: 20px 0;
        }
        
        .brightness-control {
            margin-bottom: 24px;
        }
        
        .slider-container {
            margin-bottom: 20px;
        }
        
        .slider-label {
            display: block;
            margin-bottom: 12px;
            font-weight: 600;
            color: #374151;
            font-size: 1.1rem;
        }
        
        .slider-wrapper {
            position: relative;
            margin-bottom: 12px;
        }
        
        .brightness-slider {
            width: 100%;
            height: 8px;
            border-radius: 8px;
            background: linear-gradient(to right, #f3f4f6, #3b82f6);
            outline: none;
            -webkit-appearance: none;
            cursor: pointer;
        }
        
        .brightness-slider::-webkit-slider-thumb {
            -webkit-appearance: none;
            appearance: none;
            width: 24px;
            height: 24px;
            border-radius: 50%;
            background: linear-gradient(135deg, #3b82f6, #1d4ed8);
            box-shadow: 0 4px 12px rgba(59, 130, 246, 0.4);
            cursor: pointer;
            transition: all 0.3s ease;
        }
        
        .brightness-slider::-webkit-slider-thumb:hover {
            transform: scale(1.1);
            box-shadow: 0 6px 16px rgba(59, 130, 246, 0.6);
        }
        
        .brightness-slider::-moz-range-thumb {
            width: 24px;
            height: 24px;
            border-radius: 50%;
            background: linear-gradient(135deg, #3b82f6, #1d4ed8);
            box-shadow: 0 4px 12px rgba(59, 130, 246, 0.4);
            cursor: pointer;
            border: none;
            transition: all 0.3s ease;
        }
        
        .brightness-display {
            text-align: center;
            font-size: 1.2rem;
            font-weight: 600;
            color: #1f2937;
            background: #f8fafc;
            padding: 12px;
            border-radius: 12px;
            border: 2px solid #e2e8f0;
        }
        
        .brightness-presets {
            margin-top: 24px;
        }
        
        .brightness-presets h3 {
            margin-bottom: 16px;
            color: #374151;
            font-size: 1.1rem;
            font-weight: 600;
        }
        
        .preset-buttons {
            display: grid;
            grid-template-columns: repeat(auto-fit, minmax(100px, 1fr));
            gap: 12px;
        }
        
        .preset-btn {
            padding: 12px 16px;
            border: none;
            border-radius: 12px;
            background: linear-gradient(135deg, #6b7280, #4b5563);
            color: white;
            font-weight: 500;
            cursor: pointer;
            transition: all 0.3s ease;
            font-size: 0.9rem;
        }
        
        .preset-btn:hover {
            transform: translateY(-2px);
            box-shadow: 0 8px 25px rgba(107, 114, 128, 0.4);
        }
        
        .preset-btn.active {
            background: linear-gradient(135deg, #3b82f6, #1d4ed8);
            box-shadow: 0 4px 12px rgba(59, 130, 246, 0.4);
        }
        
        .screen-settings {
            margin-bottom: 24px;
        }
        
        .setting-item {
            display: flex;
            justify-content: space-between;
            align-items: center;
            padding: 16px 0;
            border-bottom: 1px solid #e5e7eb;
        }
        
        .setting-item:last-child {
            border-bottom: none;
        }
        
        .setting-label {
            font-weight: 500;
            color: #374151;
            font-size: 1rem;
        }
        
        @media (max-width: 640px) {
            .container {
                padding: 16px;
            }
            
            .header h1 {
                font-size: 2rem;
            }
            
            .card {
                padding: 20px;
            }
            
            .action-buttons {
                flex-direction: column;
            }
            
            .container {
                padding: 16px;
            }
            
            .card {
                padding: 20px;
                padding-top: 60px;
            }
            
            .back-home-btn {
                top: 12px;
                right: 12px;
                font-size: 0.9rem;
                padding: 10px 16px;
            }
            
            .header h1 {
                font-size: 2rem;
            }
        }
        )";
    }

String WebServerManager::getJavaScript() {
    String js = "let statusInterval;\n";
    js += "let networks = [];\n\n";
    
    js += "document.addEventListener('DOMContentLoaded', function() {\n";
    js += "    loadSystemInfo();\n";
    js += "    updateStatus();\n";
    js += "    loadSavedWiFiConfigs();\n";
    js += "    statusInterval = setInterval(updateStatus, 5000);\n";
    js += "    document.getElementById('scanBtn').addEventListener('click', scanWiFi);\n";
    js += "    document.getElementById('wifiForm').addEventListener('submit', connectWiFi);\n";
    js += "});\n\n";
    
    js += "function switchTab(tabName) {\n";
    js += "    const tabs = document.querySelectorAll('.tab-btn');\n";
    js += "    const contents = document.querySelectorAll('.tab-content');\n";
    js += "    for (let i = 0; i < tabs.length; i++) {\n";
    js += "        tabs[i].classList.remove('active');\n";
    js += "    }\n";
    js += "    for (let i = 0; i < contents.length; i++) {\n";
    js += "        contents[i].classList.remove('active');\n";
    js += "    }\n";
    js += "    document.getElementById(tabName + 'Tab').classList.add('active');\n";
    js += "    document.getElementById(tabName + 'Content').classList.add('active');\n";
    js += "    if (tabName === 'system') {\n";
    js += "        loadSystemInfo();\n";
    js += "    } else if (tabName === 'wifi') {\n";
    js += "        loadSavedWiFiConfigs();\n";
    js += "    } else if (tabName === 'screen') {\n";
    js += "        loadScreenConfig();\n";
    js += "    }\n";
    js += "}\n\n";
    
    js += "async function updateStatus() {\n";
    js += "    try {\n";
    js += "        const response = await fetch('/status');\n";
    js += "        const data = await response.json();\n";
    js += "        const statusIndicator = document.getElementById('statusIndicator');\n";
    js += "        const statusTitle = document.getElementById('statusTitle');\n";
    js += "        const statusDetail = document.getElementById('statusDetail');\n";
    js += "        const currentWiFi = document.getElementById('currentWiFi');\n";
    js += "        statusIndicator.className = 'status-indicator';\n";
    js += "        if (data.wifi && data.wifi.connected) {\n";
    js += "            statusIndicator.classList.add('status-connected');\n";
    js += "            statusTitle.textContent = 'WiFi已连接';\n";
    js += "            if (data.wifi.ssid) {\n";
    js += "                currentWiFi.textContent = '当前网络: ' + data.wifi.ssid;\n";
    js += "                currentWiFi.classList.remove('hidden');\n";
    js += "                statusDetail.textContent = 'IP地址: ' + data.wifi.ip + ' | 信号强度: ' + data.wifi.rssi + 'dBm';\n";
    js += "            } else {\n";
    js += "                currentWiFi.classList.add('hidden');\n";
    js += "                statusDetail.textContent = 'IP地址: ' + data.wifi.ip + ' | 信号强度: ' + data.wifi.rssi + 'dBm';\n";
    js += "            }\n";
    js += "        } else if (data.wifi && data.wifi.mode === 'AP') {\n";
    js += "            statusIndicator.classList.add('status-ap');\n";
    js += "            statusTitle.textContent = 'AP配置模式';\n";
    js += "            statusDetail.textContent = '配置IP: ' + data.wifi.ip + ' | 等待WiFi配置';\n";
    js += "            currentWiFi.classList.add('hidden');\n";
    js += "        } else {\n";
    js += "            statusIndicator.classList.add('status-disconnected');\n";
    js += "            statusTitle.textContent = 'WiFi未连接';\n";
    js += "            statusDetail.textContent = '请配置WiFi网络';\n";
    js += "            currentWiFi.classList.add('hidden');\n";
    js += "        }\n";

    js += "        document.getElementById('uptime').textContent = formatUptime(data.system.uptime);\n";
    js += "    } catch (error) {\n";
    js += "        console.error('更新状态失败:', error);\n";
    js += "        showToast('获取状态失败', 'error');\n";
    js += "    }\n";
    js += "}\n\n";
    
    js += "async function loadSystemInfo() {\n";
    js += "    try {\n";
    js += "        const response = await fetch('/info');\n";
    js += "        const data = await response.json();\n";
    js += "        document.getElementById('deviceModel').textContent = data.chipModel + ' Rev.' + data.chipRevision;\n";
    js += "        document.getElementById('firmwareVersion').textContent = data.version;\n";
    js += "        document.getElementById('cpuFreq').textContent = data.cpuFreq + ' MHz';\n";
    js += "        document.getElementById('flashSize').textContent = formatBytes(data.flashSize);\n";
    js += "        // SRAM信息显示\n";
    js += "        document.getElementById('sramInfo').textContent = formatBytes(data.sramUsed) + ' / ' + formatBytes(data.sramTotal) + ' (' + data.sramUsagePercent.toFixed(1) + '%)';\n";
    js += "        \n";
    js += "        // PSRAM信息显示\n";
    js += "        if (data.psramAvailable) {\n";
    js += "            document.getElementById('psramInfoItem').style.display = 'block';\n";
    js += "            document.getElementById('psramInfo').textContent = formatBytes(data.psramUsed) + ' / ' + formatBytes(data.psramTotal) + ' (' + data.psramUsagePercent.toFixed(1) + '%)';\n";
    js += "        } else {\n";
    js += "            document.getElementById('psramInfoItem').style.display = 'none';\n";
    js += "        }\n";
    js += "        document.getElementById('uptime').textContent = formatUptime(data.uptime);\n";
    js += "        \n";
    js += "        // 显示WiFi信息\n";
    js += "        if (data.wifi && data.wifi.status === 'connected' && data.wifi.ssid) {\n";
    js += "            document.getElementById('currentWiFiName').textContent = data.wifi.ssid + ' (已连接)';\n";
    js += "        } else {\n";
    js += "            document.getElementById('currentWiFiName').textContent = '未连接';\n";
    js += "        }\n";
    js += "    } catch (error) {\n";
    js += "        console.error('加载系统信息失败:', error);\n";
    js += "        showToast('加载系统信息失败', 'error');\n";
    js += "    }\n";
    js += "}\n\n";
    
    js += "async function scanWiFi() {\n";
    js += "    const scanBtn = document.getElementById('scanBtn');\n";
    js += "    const networkList = document.getElementById('networkList');\n";
    js += "    scanBtn.disabled = true;\n";
    js += "    scanBtn.innerHTML = '<div class=\"spinner-sm\"></div> 扫描中...';\n";
    js += "    networkList.classList.remove('hidden');\n";
    js += "    try {\n";
    js += "        const response = await fetch('/scan');\n";
    js += "        const data = await response.json();\n";
    js += "        networks = data.networks || [];\n";
    js += "        displayNetworks(networks);\n";
    js += "    } catch (error) {\n";
    js += "        console.error('WiFi扫描失败:', error);\n";
    js += "        showToast('WiFi扫描失败', 'error');\n";
    js += "        networkList.classList.add('hidden');\n";
    js += "    } finally {\n";
    js += "        scanBtn.disabled = false;\n";
    js += "        scanBtn.innerHTML = '扫描WiFi网络';\n";
    js += "    }\n";
    js += "}\n\n";
    
    js += "function displayNetworks(networks) {\n";
    js += "    const networkList = document.getElementById('networkList');\n";
    js += "    if (networks.length === 0) {\n";
    js += "        networkList.innerHTML = '<div class=\"loading\">未发现WiFi网络</div>';\n";
    js += "        return;\n";
    js += "    }\n";
    js += "    networks.sort((a, b) => b.rssi - a.rssi);\n";
    js += "    let html = '';\n";
    js += "    for (let i = 0; i < networks.length; i++) {\n";
    js += "        const network = networks[i];\n";
    js += "        const signalBars = getSignalBars(network.rssi);\n";
    js += "        const secureIcon = network.secure ? '🔒' : '🔓';\n";
    js += "        html += '<div class=\"network-item\" onclick=\"selectNetwork(\\'' + network.ssid + '\\')\">'\n";
    js += "        html += '<div class=\"network-name\">' + network.ssid + '</div>';\n";
    js += "        html += '<div class=\"network-info\">';\n";
    js += "        html += '<span>' + secureIcon + '</span>';\n";
    js += "        html += '<div class=\"signal-strength\">' + signalBars + '</div>';\n";
    js += "        html += '<span>' + network.rssi + 'dBm</span>';\n";
    js += "        html += '</div>';\n";
    js += "        html += '</div>';\n";
    js += "    }\n";
    js += "    networkList.innerHTML = html;\n";
    js += "}\n\n";
    
    js += "function getSignalBars(rssi) {\n";
    js += "    const strength = Math.min(Math.max(2 * (rssi + 100), 0), 100);\n";
    js += "    const bars = Math.ceil(strength / 25);\n";
    js += "    let html = '';\n";
    js += "    // 信号条从左到右：低→高，符合常见WiFi图标习惯\n";
    js += "    for (let i = 1; i <= 4; i++) {\n";
    js += "        const height = i * 4 + 4; // 8px, 12px, 16px, 20px\n";
    js += "        const active = i <= bars ? 'active' : '';\n";
    js += "        html += '<div class=\"signal-bar ' + active + '\" style=\"height: ' + height + 'px;\"></div>';\n";
    js += "    }\n";
    js += "    return html;\n";
    js += "}\n\n";
    
    js += "function selectNetwork(ssid) {\n";
    js += "    document.getElementById('ssid').value = ssid;\n";
    js += "    const items = document.querySelectorAll('.network-item');\n";
    js += "    for (let i = 0; i < items.length; i++) {\n";
    js += "        items[i].classList.remove('selected');\n";
    js += "    }\n";
    js += "    event.target.closest('.network-item').classList.add('selected');\n";
    js += "    document.getElementById('password').focus();\n";
    js += "}\n\n";
    
    js += "async function connectWiFi(event) {\n";
    js += "    event.preventDefault();\n";
    js += "    const ssid = document.getElementById('ssid').value.trim();\n";
    js += "    const password = document.getElementById('password').value;\n";
    js += "    if (!ssid) {\n";
    js += "        showToast('请输入WiFi名称', 'error');\n";
    js += "        return;\n";
    js += "    }\n";
    js += "    const connectBtn = document.getElementById('connectBtn');\n";
    js += "    const btnText = connectBtn.querySelector('.btn-text');\n";
    js += "    const btnLoading = connectBtn.querySelector('.btn-loading');\n";
    js += "    connectBtn.disabled = true;\n";
    js += "    btnText.classList.add('hidden');\n";
    js += "    btnLoading.classList.remove('hidden');\n";
    js += "    try {\n";
    js += "        const formData = new FormData();\n";
    js += "        formData.append('ssid', ssid);\n";
    js += "        formData.append('password', password);\n";
    js += "        const response = await fetch('/save', { method: 'POST', body: formData });\n";
    js += "        const data = await response.json();\n";
    js += "        if (data.success) {\n";
    js += "            showToast('WiFi连接成功！', 'success');\n";
    js += "            setTimeout(() => {\n";
    js += "                updateStatus();\n";
    js += "                loadSavedWiFiConfigs();\n";
    js += "                document.getElementById('networkList').classList.add('hidden');\n";
    js += "                document.getElementById('wifiForm').reset();\n";
    js += "            }, 2000);\n";
    js += "        } else {\n";
    js += "            showToast(data.message || 'WiFi连接失败', 'error');\n";
    js += "        }\n";
    js += "    } catch (error) {\n";
    js += "        console.error('连接WiFi失败:', error);\n";
    js += "        // 如果是连接重置错误，可能是WiFi连接成功导致的网络切换\n";
    js += "        if (error.message && error.message.includes('Failed to fetch')) {\n";
    js += "            showToast('WiFi连接可能成功，正在检查状态...', 'success');\n";
    js += "            // 延时刷新状态，因为可能WiFi已经连接成功\n";
    js += "            setTimeout(() => {\n";
    js += "                updateStatus();\n";
    js += "                loadSavedWiFiConfigs();\n";
    js += "                document.getElementById('networkList').classList.add('hidden');\n";
    js += "                document.getElementById('wifiForm').reset();\n";
    js += "            }, 3000);\n";
    js += "        } else {\n";
    js += "            showToast('连接WiFi失败', 'error');\n";
    js += "        }\n";
    js += "    } finally {\n";
    js += "        connectBtn.disabled = false;\n";
    js += "        btnText.classList.remove('hidden');\n";
    js += "        btnLoading.classList.add('hidden');\n";
    js += "    }\n";
    js += "}\n\n";
    

    
    js += "function resetConfig() {\n";
    js += "    if (confirm('确定要恢复默认配置吗？\\n\\n此操作将：\\n• 清除所有WiFi配置\\n• 清除所有系统设置\\n• 重启设备\\n\\n此操作不可撤销！')) {\n";
    js += "        showToast('正在重置配置...', 'success');\n";
    js += "        fetch('/reset').then(response => response.json()).then(data => {\n";
    js += "            if (data.success) {\n";
    js += "                showToast(data.message, 'success');\n";
    js += "                setTimeout(() => { location.reload(); }, 5000);\n";
    js += "            } else {\n";
    js += "                showToast(data.message || '配置重置失败', 'error');\n";
    js += "            }\n";
    js += "        }).catch(error => {\n";
    js += "            console.error('配置重置失败:', error);\n";
    js += "            showToast('配置重置失败', 'error');\n";
    js += "        });\n";
    js += "    }\n";
    js += "}\n\n";
    
    js += "function rebootDevice() {\n";
    js += "    if (confirm('确定要重启设备吗？')) {\n";
    js += "        showToast('设备将在3秒后重启...', 'success');\n";
    js += "        fetch('/restart').then(() => {\n";
    js += "            setTimeout(() => { location.reload(); }, 5000);\n";
    js += "        }).catch(error => {\n";
    js += "            console.error('重启失败:', error);\n";
    js += "            showToast('重启失败', 'error');\n";
    js += "        });\n";
    js += "    }\n";
    js += "}\n\n";
    
    js += "function refreshInfo() {\n";
    js += "    loadSystemInfo(); updateStatus(); showToast('信息已刷新', 'success');\n";
    js += "}\n\n";
    
    js += "function showToast(message, type) {\n";
    js += "    type = type || 'success';\n";
    js += "    const toast = document.getElementById('toast');\n";
    js += "    const toastMessage = document.getElementById('toastMessage');\n";
    js += "    toastMessage.textContent = message;\n";
    js += "    toast.className = 'toast ' + type;\n";
    js += "    toast.classList.add('show');\n";
    js += "    setTimeout(() => { toast.classList.remove('show'); }, 3000);\n";
    js += "}\n\n";
    
    js += "function formatBytes(bytes) {\n";
    js += "    if (bytes === 0) return '0 B';\n";
    js += "    const k = 1024;\n";
    js += "    const sizes = ['B', 'KB', 'MB', 'GB'];\n";
    js += "    const i = Math.floor(Math.log(bytes) / Math.log(k));\n";
    js += "    return parseFloat((bytes / Math.pow(k, i)).toFixed(2)) + ' ' + sizes[i];\n";
    js += "}\n\n";
    
    js += "function formatUptime(milliseconds) {\n";
    js += "    const seconds = Math.floor(milliseconds / 1000);\n";
    js += "    const minutes = Math.floor(seconds / 60);\n";
    js += "    const hours = Math.floor(minutes / 60);\n";
    js += "    const days = Math.floor(hours / 24);\n";
    js += "    if (days > 0) {\n";
    js += "        return days + '天 ' + (hours % 24) + '小时';\n";
    js += "    } else if (hours > 0) {\n";
    js += "        return hours + '小时 ' + (minutes % 60) + '分钟';\n";
    js += "    } else if (minutes > 0) {\n";
    js += "        return minutes + '分钟 ' + (seconds % 60) + '秒';\n";
    js += "    } else {\n";
    js += "        return seconds + '秒';\n";
    js += "    }\n";
    js += "}\n\n";
    
    js += "async function loadSavedWiFiConfigs() {\n";
    js += "    const savedWiFiList = document.getElementById('savedWiFiList');\n";
    js += "    const savedWiFiLoading = document.getElementById('savedWiFiLoading');\n";
    js += "    \n";
    js += "    if (!savedWiFiList) {\n";
    js += "        console.error('savedWiFiList 元素未找到');\n";
    js += "        return;\n";
    js += "    }\n";
    js += "    \n";
    js += "    if (savedWiFiLoading) {\n";
    js += "        savedWiFiLoading.classList.remove('hidden');\n";
    js += "    }\n";
    js += "    try {\n";
    js += "        // 添加时间戳防止缓存\n";
    js += "        const timestamp = new Date().getTime();\n";
    js += "        const response = await fetch('/wifi-configs?t=' + timestamp, {\n";
    js += "            method: 'GET',\n";
    js += "            headers: {\n";
    js += "                'Cache-Control': 'no-cache, no-store, must-revalidate',\n";
    js += "                'Pragma': 'no-cache',\n";
    js += "                'Expires': '0'\n";
    js += "            }\n";
    js += "        });\n";
    js += "        const data = await response.json();\n";
    js += "        displaySavedWiFiConfigs(data.configs || [], data.count || 0, data.maxConfigs || 3);\n";
    js += "    } catch (error) {\n";
    js += "        console.error('加载WiFi配置失败:', error);\n";
    js += "        savedWiFiList.innerHTML = '<div class=\"empty-wifi-message\">加载WiFi配置失败</div>';\n";
    js += "    } finally {\n";
    js += "        if (savedWiFiLoading) {\n";
    js += "            savedWiFiLoading.classList.add('hidden');\n";
    js += "        }\n";
    js += "    }\n";
    js += "}\n\n";
    
    js += "function displaySavedWiFiConfigs(configs, count, maxConfigs) {\n";
    js += "    const savedWiFiList = document.getElementById('savedWiFiList');\n";
    js += "    \n";
    js += "    if (!savedWiFiList) {\n";
    js += "        console.error('savedWiFiList 元素未找到，无法显示WiFi配置');\n";
    js += "        return;\n";
    js += "    }\n";
    js += "    if (configs.length === 0) {\n";
    js += "        savedWiFiList.innerHTML = '<div class=\"empty-wifi-message\">暂无保存的WiFi配置<br><small>连接新的WiFi网络后会自动保存</small></div>';\n";
    js += "        return;\n";
    js += "    }\n";
    js += "    \n";
    js += "    // 按优先级排序配置\n";
    js += "    configs.sort((a, b) => a.priority - b.priority);\n";
    js += "    \n";
    js += "    let html = '';\n";
    js += "    for (let i = 0; i < configs.length; i++) {\n";
    js += "        const config = configs[i];\n";
    js += "        html += '<div class=\"saved-wifi-item\">';\n";
    js += "        html += '<div class=\"saved-wifi-info\">';\n";
    js += "        html += '<div class=\"saved-wifi-name\">' + config.ssid + '</div>';\n";
    js += "        html += '<div class=\"saved-wifi-priority\">优先级: ' + config.priority + '</div>';\n";
    js += "        html += '</div>';\n";
    js += "        html += '<div class=\"saved-wifi-actions\">';\n";
    js += "        html += '<div class=\"priority-controls\">';\n";
    js += "        html += '<label>优先级:</label>';\n";
    js += "        html += '<select class=\"priority-select\" onchange=\"updateWiFiPriority(' + config.index + ', this.value)\" id=\"priority_' + config.index + '\">';\n";
    js += "        for (let p = 1; p <= 10; p++) {\n";
    js += "            const selected = p === config.priority ? 'selected' : '';\n";
    js += "            html += '<option value=\"' + p + '\" ' + selected + '>' + p + '</option>';\n";
    js += "        }\n";
    js += "        html += '</select>';\n";
    js += "        html += '</div>';\n";
    js += "        html += '<button class=\"connect-btn-small\" onclick=\"connectWiFiConfig(' + config.index + ')\" id=\"connectBtn_' + config.index + '\">';\n";
    js += "        html += '连接';\n";
    js += "        html += '</button>';\n";
    js += "        html += '<button class=\"delete-btn\" onclick=\"deleteWiFiConfig(' + config.index + ')\">';\n";
    js += "        html += '删除';\n";
    js += "        html += '</button>';\n";
    js += "        html += '</div>';\n";
    js += "        html += '</div>';\n";
    js += "    }\n";
    js += "    if (count < maxConfigs) {\n";
    js += "        html += '<div class=\"empty-wifi-message\" style=\"margin-top: 10px;\">';\n";
    js += "        html += '还可以保存 ' + (maxConfigs - count) + ' 个WiFi配置';\n";
    js += "        html += '</div>';\n";
    js += "    }\n";
    js += "    savedWiFiList.innerHTML = html;\n";
    js += "}\n\n";
    
    js += "async function deleteWiFiConfig(index) {\n";
    js += "    if (!confirm('确定要删除这个WiFi配置吗？')) {\n";
    js += "        return;\n";
    js += "    }\n";
    js += "    try {\n";
    js += "        const formData = new FormData();\n";
    js += "        formData.append('index', index);\n";
    js += "        const response = await fetch('/delete-wifi-config', { method: 'POST', body: formData });\n";
    js += "        const data = await response.json();\n";
    js += "        if (data.success) {\n";
    js += "            showToast('WiFi配置删除成功', 'success');\n";
    js += "            // 立即刷新一次\n";
    js += "            await loadSavedWiFiConfigs();\n";
    js += "            // 再延时刷新一次确保数据更新\n";
    js += "            setTimeout(async () => {\n";
    js += "                await loadSavedWiFiConfigs();\n";
    js += "            }, 1000);\n";
    js += "        } else {\n";
    js += "            showToast(data.message || 'WiFi配置删除失败', 'error');\n";
    js += "        }\n";
    js += "    } catch (error) {\n";
    js += "        console.error('删除WiFi配置失败:', error);\n";
    js += "        showToast('删除WiFi配置失败', 'error');\n";
    js += "    }\n";
    js += "}\n\n";
    
    js += "async function connectWiFiConfig(index) {\n";
    js += "    const connectBtn = document.getElementById('connectBtn_' + index);\n";
    js += "    if (!connectBtn) return;\n";
    js += "    \n";
    js += "    // 禁用所有连接按钮\n";
    js += "    const allConnectBtns = document.querySelectorAll('.connect-btn-small');\n";
    js += "    for (let i = 0; i < allConnectBtns.length; i++) {\n";
    js += "        allConnectBtns[i].disabled = true;\n";
    js += "    }\n";
    js += "    \n";
    js += "    connectBtn.textContent = '连接中...';\n";
    js += "    \n";
    js += "    try {\n";
    js += "        const formData = new FormData();\n";
    js += "        formData.append('index', index);\n";
    js += "        const response = await fetch('/connect-wifi', { method: 'POST', body: formData });\n";
    js += "        const data = await response.json();\n";
    js += "        \n";
    js += "        if (data.success) {\n";
    js += "            showToast('WiFi连接成功！IP: ' + data.ip, 'success');\n";
    js += "            setTimeout(() => {\n";
    js += "                updateStatus();\n";
    js += "                loadSavedWiFiConfigs();\n";
    js += "            }, 2000);\n";
    js += "        } else {\n";
    js += "            showToast(data.message || 'WiFi连接失败', 'error');\n";
    js += "        }\n";
    js += "    } catch (error) {\n";
    js += "        console.error('连接WiFi配置失败:', error);\n";
    js += "        showToast('连接WiFi配置失败', 'error');\n";
    js += "    } finally {\n";
    js += "        // 恢复所有连接按钮\n";
    js += "        for (let i = 0; i < allConnectBtns.length; i++) {\n";
    js += "            allConnectBtns[i].disabled = false;\n";
    js += "            allConnectBtns[i].textContent = '连接';\n";
    js += "        }\n";
    js += "    }\n";
    js += "}\n\n";
    
    js += "async function updateWiFiPriority(index, priority) {\n";
    js += "    console.log('更新WiFi配置', index, '的优先级为', priority);\n";
    js += "    \n";
    js += "    if (priority < 1 || priority > 10) {\n";
    js += "        showToast('优先级必须在1-10之间', 'error');\n";
    js += "        return;\n";
    js += "    }\n";
    js += "    \n";
    js += "    try {\n";
    js += "        const formData = new FormData();\n";
    js += "        formData.append('index', index);\n";
    js += "        formData.append('priority', priority);\n";
    js += "        \n";
    js += "        const response = await fetch('/update-wifi-priority', {\n";
    js += "            method: 'POST',\n";
    js += "            body: formData\n";
    js += "        });\n";
    js += "        \n";
    js += "        const data = await response.json();\n";
    js += "        \n";
    js += "        if (data.success) {\n";
    js += "            showToast('WiFi优先级更新成功', 'success');\n";
    js += "            // 刷新WiFi配置列表\n";
    js += "            setTimeout(async () => {\n";
    js += "                await loadSavedWiFiConfigs();\n";
    js += "            }, 500);\n";
    js += "        } else {\n";
    js += "            showToast(data.message || 'WiFi优先级更新失败', 'error');\n";
    js += "            // 恢复原来的选择\n";
    js += "            const select = document.getElementById('priority_' + index);\n";
    js += "            if (select) {\n";
    js += "                select.selectedIndex = 0; // 恢复到第一个选项\n";
    js += "            }\n";
    js += "        }\n";
    js += "    } catch (error) {\n";
    js += "        console.error('更新WiFi优先级失败:', error);\n";
    js += "        showToast('更新WiFi优先级失败', 'error');\n";
    js += "        // 恢复原来的选择\n";
    js += "        const select = document.getElementById('priority_' + index);\n";
    js += "        if (select) {\n";
    js += "            select.selectedIndex = 0; // 恢复到第一个选项\n";
    js += "        }\n";
    js += "    }\n";
    js += "}\n\n";


    
    js += "// 屏幕配置相关函数\n";
    js += "async function loadScreenConfig() {\n";
    js += "    try {\n";
    js += "        const response = await fetch('/api/screen/config');\n";
    js += "        const data = await response.json();\n";
    js += "        if (data.success) {\n";
    js += "            document.getElementById('brightnessSlider').value = data.brightness || 128;\n";
    js += "            updateBrightnessValue(data.brightness || 128);\n";
    js += "            document.getElementById('currentBrightness').textContent = Math.round((data.brightness / 255) * 100) + '%';\n";
    js += "            document.getElementById('screenStatus').textContent = data.maxBrightness || '350nit';\n";
    js += "            document.getElementById('backlightStatus').textContent = data.backlightOn ? '开启' : '关闭';\n";
    js += "            // 新增屏幕信息显示\n";
    js += "            document.getElementById('screenResolution').textContent = data.resolution || '368x448';\n";
    js += "            document.getElementById('colorSupport').textContent = data.colorSupport || 'RGB565';\n";
    js += "            document.getElementById('screenMaterial').textContent = data.material || 'AMOLED';\n";
    js += "        }\n";
    js += "    } catch (error) {\n";
    js += "        console.error('加载屏幕配置失败:', error);\n";
    js += "        showToast('加载屏幕配置失败', 'error');\n";
    js += "    }\n";
    js += "}\n\n";
    
    js += "function updateBrightnessValue(value) {\n";
    js += "    const percentage = Math.round((value / 255) * 100);\n";
    js += "    document.getElementById('brightnessValue').textContent = percentage + '%';\n";
    js += "    // 更新预设按钮状态\n";
    js += "    const presetBtns = document.querySelectorAll('.preset-btn');\n";
    js += "    presetBtns.forEach(btn => btn.classList.remove('active'));\n";
    js += "    // 根据当前值高亮对应的预设按钮\n";
    js += "    if (value <= 30) {\n";
    js += "        presetBtns[0]?.classList.add('active');\n";
    js += "    } else if (value <= 140) {\n";
    js += "        presetBtns[1]?.classList.add('active');\n";
    js += "    } else if (value <= 220) {\n";
    js += "        presetBtns[2]?.classList.add('active');\n";
    js += "    } else {\n";
    js += "        presetBtns[3]?.classList.add('active');\n";
    js += "    }\n";
    js += "}\n\n";
    
    js += "async function setBrightness(value) {\n";
    js += "    try {\n";
    js += "        const response = await fetch('/api/screen/brightness', {\n";
    js += "            method: 'POST',\n";
    js += "            headers: { 'Content-Type': 'application/x-www-form-urlencoded' },\n";
    js += "            body: 'brightness=' + value\n";
    js += "        });\n";
    js += "        const data = await response.json();\n";
    js += "        if (data.success) {\n";
    js += "            showToast('亮度设置成功', 'success');\n";
    js += "            document.getElementById('currentBrightness').textContent = Math.round((value / 255) * 100) + '%';\n";
    js += "        } else {\n";
    js += "            showToast('亮度设置失败: ' + data.message, 'error');\n";
    js += "        }\n";
    js += "    } catch (error) {\n";
    js += "        console.error('设置亮度失败:', error);\n";
    js += "        showToast('设置亮度失败', 'error');\n";
    js += "    }\n";
    js += "}\n\n";
    
    js += "function setPresetBrightness(value) {\n";
    js += "    document.getElementById('brightnessSlider').value = value;\n";
    js += "    updateBrightnessValue(value);\n";
    js += "    setBrightness(value);\n";
    js += "}\n\n";
    
    js += "window.addEventListener('beforeunload', function() {\n";
    js += "    if (statusInterval) { clearInterval(statusInterval); }\n";
    js += "});";
    
    return js;
}

String WebServerManager::getOTAJavaScript() {
    String js = "";
    
    // 基础工具函数
    js += "function showToast(message, type) {\n";
    js += "    const toast = document.getElementById('toast');\n";
    js += "    const toastMessage = document.getElementById('toastMessage');\n";
    js += "    toastMessage.textContent = message;\n";
    js += "    toast.className = 'toast show ' + (type || 'info');\n";
    js += "    setTimeout(() => { toast.className = 'toast hidden'; }, 3000);\n";
    js += "}\n\n";
    
    js += "function formatBytes(bytes) {\n";
    js += "    if (bytes === 0) return '0 字节';\n";
    js += "    const k = 1024;\n";
    js += "    const sizes = ['字节', 'KB', 'MB', 'GB'];\n";
    js += "    const i = Math.floor(Math.log(bytes) / Math.log(k));\n";
    js += "    return parseFloat((bytes / Math.pow(k, i)).toFixed(2)) + ' ' + sizes[i];\n";
    js += "}\n\n";
    
    js += "let otaStatusInterval;\n";
    js += "let selectedFile = null;\n\n";
    
    js += "document.addEventListener('DOMContentLoaded', function() {\n";
    js += "    initOTAHandlers();\n";
    js += "    initTabSwitching();\n";
    js += "});\n\n";
    
    js += "function initTabSwitching() {\n";
    js += "    const tabButtons = document.querySelectorAll('.tab-btn');\n";
    js += "    for (let i = 0; i < tabButtons.length; i++) {\n";
    js += "        tabButtons[i].addEventListener('click', function() {\n";
    js += "            const tabName = this.getAttribute('data-tab');\n";
    js += "            switchOTATab(tabName);\n";
    js += "        });\n";
    js += "    }\n";
    js += "}\n\n";
    
    js += "function switchOTATab(tabName) {\n";
    js += "    // 移除所有标签页按钮的active类\n";
    js += "    const tabButtons = document.querySelectorAll('.tab-btn');\n";
    js += "    for (let i = 0; i < tabButtons.length; i++) {\n";
    js += "        tabButtons[i].classList.remove('active');\n";
    js += "    }\n";
    js += "    \n";
    js += "    // 移除所有标签页内容的active类\n";
    js += "    const tabContents = document.querySelectorAll('.tab-content');\n";
    js += "    for (let i = 0; i < tabContents.length; i++) {\n";
    js += "        tabContents[i].classList.remove('active');\n";
    js += "    }\n";
    js += "    \n";
    js += "    // 激活选中的标签页按钮\n";
    js += "    const activeButton = document.querySelector('[data-tab=\"' + tabName + '\"]');\n";
    js += "    if (activeButton) {\n";
    js += "        activeButton.classList.add('active');\n";
    js += "    }\n";
    js += "    \n";
    js += "    // 激活对应的标签页内容\n";
    js += "    const activeContent = document.getElementById(tabName + 'Tab');\n";
    js += "    if (activeContent) {\n";
    js += "        activeContent.classList.add('active');\n";
    js += "    }\n";
    js += "}\n\n";
    
    js += "function initOTAHandlers() {\n";
    js += "    const selectFileBtn = document.getElementById('selectFileBtn');\n";
    js += "    const firmwareFile = document.getElementById('firmwareFile');\n";
    js += "    const uploadBtn = document.getElementById('uploadBtn');\n";
    js += "    const retryBtn = document.getElementById('retryBtn');\n";
    js += "    const checkVersionBtn = document.getElementById('checkVersionBtn');\n";
    js += "    const serverOTABtn = document.getElementById('serverOTABtn');\n";
    js += "    \n";
    js += "    if (selectFileBtn) selectFileBtn.addEventListener('click', () => firmwareFile.click());\n";
    js += "    if (firmwareFile) firmwareFile.addEventListener('change', handleFileSelect);\n";
    js += "    if (uploadBtn) uploadBtn.addEventListener('click', startOTAUpload);\n";
    js += "    if (retryBtn) retryBtn.addEventListener('click', resetOTAUI);\n";
    js += "    if (checkVersionBtn) checkVersionBtn.addEventListener('click', checkServerVersion);\n";
    js += "    if (serverOTABtn) serverOTABtn.addEventListener('click', startServerOTA);\n";
    js += "}\n\n";
    
    js += "function handleFileSelect(event) {\n";
    js += "    const file = event.target.files[0];\n";
    js += "    if (file) {\n";
    js += "        if (!file.name.endsWith('.bin')) {\n";
    js += "            showToast('请选择.bin格式的固件文件', 'error');\n";
    js += "            event.target.value = '';\n";
    js += "            return;\n";
    js += "        }\n";
    js += "        selectedFile = file;\n";
    js += "        document.getElementById('fileName').textContent = file.name;\n";
    js += "        document.getElementById('fileSize').textContent = formatBytes(file.size);\n";
    js += "        document.getElementById('fileInfo').classList.remove('hidden');\n";
    js += "        document.getElementById('uploadBtn').disabled = false;\n";
    js += "    }\n";
    js += "}\n\n";
    
    js += "async function startOTAUpload() {\n";
    js += "    if (!selectedFile) { showToast('请先选择固件文件', 'error'); return; }\n";
    js += "    const uploadBtn = document.getElementById('uploadBtn');\n";
    js += "    uploadBtn.disabled = true;\n";
    js += "    // 隐藏按钮文字，显示loading状态\n";
    js += "    uploadBtn.querySelector('.btn-text').style.display = 'none';\n";
    js += "    const btnLoading = uploadBtn.querySelector('.btn-loading');\n";
    js += "    btnLoading.classList.remove('hidden');\n";
    js += "    btnLoading.innerHTML = '<div class=\"spinner-sm\"></div><span>升级中...</span>';\n";
    js += "    document.getElementById('otaProgress').classList.remove('hidden');\n";
    js += "    // 初始化进度显示\n";
    js += "    document.getElementById('progressSize').textContent = '0 / ' + formatBytes(selectedFile.size);\n";
    js += "    try {\n";
    js += "        const formData = new FormData();\n";
    js += "        formData.append('firmware', selectedFile);\n";
    js += "        const xhr = new XMLHttpRequest();\n";
            js += "        xhr.upload.addEventListener('progress', (e) => {\n";
        js += "            if (e.lengthComputable) {\n";
        js += "                const percent = Math.round((e.loaded / e.total) * 100);\n";
        js += "                document.getElementById('progressFill').style.width = percent + '%';\n";
        js += "                document.getElementById('progressPercent').textContent = percent + '%';\n";
        js += "                document.getElementById('progressSize').textContent = formatBytes(e.loaded) + ' / ' + formatBytes(e.total);\n";
        js += "            }\n";
        js += "        });\n";
    js += "        otaStatusInterval = setInterval(updateOTAStatus, 300);\n";
    js += "        xhr.open('POST', '/ota-upload');\n";
    js += "        xhr.send(formData);\n";
    js += "    } catch (error) {\n";
    js += "        showOTAResult(false, 'OTA升级失败: ' + error.message);\n";
    js += "    }\n";
    js += "}\n\n";
    
    js += "async function updateOTAStatus() {\n";
    js += "    try {\n";
    js += "        const response = await fetch('/ota-status');\n";
    js += "        const status = await response.json();\n";
    js += "        document.getElementById('otaStatusTitle').textContent = status.message || '升级中...';\n";
    js += "        const progress = status.progress || 0;\n";
    js += "        document.getElementById('progressFill').style.width = progress + '%';\n";
    js += "        document.getElementById('progressPercent').textContent = progress.toFixed(1) + '%';\n";
    js += "        // 更新大小信息\n";
    js += "        if (status.totalSize && status.writtenSize !== undefined) {\n";
    js += "            document.getElementById('progressSize').textContent = formatBytes(status.writtenSize) + ' / ' + formatBytes(status.totalSize);\n";
    js += "        }\n";
    js += "        if (status.status === 'failed') {\n";
    js += "            clearInterval(otaStatusInterval);\n";
    js += "            showOTAResult(false, status.error || '升级失败');\n";
    js += "        } else if (status.status === 'success') {\n";
    js += "            clearInterval(otaStatusInterval);\n";
    js += "            showOTAResult(true, '固件升级成功！');\n";
    js += "        }\n";
    js += "    } catch (error) { console.error('获取OTA状态失败:', error); }\n";
    js += "}\n\n";
    
    js += "function showOTAResult(success, message) {\n";
    js += "    document.getElementById('otaProgress').classList.add('hidden');\n";
    js += "    document.getElementById('otaResult').classList.remove('hidden');\n";
    js += "    if (success) {\n";
    js += "        document.getElementById('successResult').classList.remove('hidden');\n";
    js += "        setTimeout(() => {\n";
    js += "            fetch('/ota-reboot', {method: 'POST'}).then(() => {\n";
    js += "                // 等待设备重启并检测重连\n";
    js += "                waitForDeviceReboot();\n";
    js += "            }).catch(() => {\n";
    js += "                // 重启请求失败也要等待重连\n";
    js += "                waitForDeviceReboot();\n";
    js += "            });\n";
    js += "        }, 3000);\n";
    js += "    } else {\n";
    js += "        document.getElementById('errorResult').classList.remove('hidden');\n";
    js += "        document.getElementById('errorMessage').textContent = message;\n";
    js += "    }\n";
    js += "}\n\n";
    
    js += "function resetOTAUI() {\n";
    js += "    selectedFile = null;\n";
    js += "    document.getElementById('firmwareFile').value = '';\n";
    js += "    document.getElementById('fileInfo').classList.add('hidden');\n";
    js += "    const uploadBtn = document.getElementById('uploadBtn');\n";
    js += "    uploadBtn.disabled = true;\n";
    js += "    // 恢复按钮原始状态\n";
    js += "    uploadBtn.querySelector('.btn-text').style.display = '';\n";
    js += "    uploadBtn.querySelector('.btn-text').textContent = '选择文件开始升级';\n";
    js += "    uploadBtn.querySelector('.btn-loading').classList.add('hidden');\n";
    js += "    document.getElementById('otaProgress').classList.add('hidden');\n";
    js += "    document.getElementById('otaResult').classList.add('hidden');\n";
    js += "    document.getElementById('successResult').classList.add('hidden');\n";
    js += "    document.getElementById('errorResult').classList.add('hidden');\n";
    js += "    if (otaStatusInterval) clearInterval(otaStatusInterval);\n";
    js += "}\n\n";
    
    js += "function waitForDeviceReboot() {\n";
    js += "    // 更新成功页面显示\n";
    js += "    const successResult = document.getElementById('successResult');\n";
    js += "    if (successResult) {\n";
    js += "        successResult.innerHTML = '<h3>设备重启中...</h3><p>请稍候，设备重启完成后将自动跳转到首页。</p>';\n";
    js += "    }\n";
    js += "    \n";
    js += "    let retryCount = 0;\n";
    js += "    const maxRetries = 30; // 最大等待30次，每次2秒\n";
    js += "    \n";
    js += "    function checkDeviceStatus() {\n";
    js += "        retryCount++;\n";
    js += "        \n";
    js += "        fetch('/status', { method: 'GET', timeout: 5000 })\n";
    js += "            .then(response => {\n";
    js += "                if (response.ok) {\n";
    js += "                    // 设备恢复正常，跳转到首页\n";
    js += "                    showToast('升级成功，正在跳转到首页...', 'success');\n";
    js += "                    setTimeout(() => {\n";
    js += "                        window.location.href = '/';\n";
    js += "                    }, 1000);\n";
    js += "                } else {\n";
    js += "                    // 继续等待\n";
    js += "                    scheduleNextCheck();\n";
    js += "                }\n";
    js += "            })\n";
    js += "            .catch(() => {\n";
    js += "                // 设备还未恢复，继续等待\n";
    js += "                scheduleNextCheck();\n";
    js += "            });\n";
    js += "    }\n";
    js += "    \n";
    js += "    function scheduleNextCheck() {\n";
    js += "        if (retryCount < maxRetries) {\n";
    js += "            setTimeout(checkDeviceStatus, 2000); // 每2秒检查一次\n";
    js += "        } else {\n";
    js += "            // 超时，手动跳转\n";
    js += "            if (successResult) {\n";
    js += "                successResult.innerHTML = '<h3>升级完成</h3><p>设备重启时间较长，请手动刷新页面或<a href=\"/\">点击这里返回首页</a>。</p>';\n";
    js += "            }\n";
    js += "        }\n";
    js += "    }\n";
    js += "    \n";
    js += "    // 等待5秒后开始检测（给设备重启留时间）\n";
    js += "    setTimeout(checkDeviceStatus, 5000);\n";
    js += "}\n\n";
    
    // 服务器OTA相关函数
    js += "async function checkServerVersion() {\n";
    js += "    const checkVersionBtn = document.getElementById('checkVersionBtn');\n";
    js += "    const versionInfo = document.getElementById('versionInfo');\n";
    js += "    const currentVersion = document.getElementById('currentVersion');\n";
    js += "    const serverVersion = document.getElementById('serverVersion');\n";
    js += "    \n";
    js += "    checkVersionBtn.disabled = true;\n";
    js += "    checkVersionBtn.textContent = '检查中...';\n";
    js += "    \n";
    js += "    try {\n";
    js += "        const response = await fetch('/api/ota/firmware-version');\n";
    js += "        const data = await response.json();\n";
    js += "        \n";
    js += "        if (data.success) {\n";
    js += "            versionInfo.classList.remove('hidden');\n";
    js += "            currentVersion.textContent = data.currentVersion || 'v5.4.0';\n";
    js += "            \n";
    js += "            if (data.serverVersion && data.serverVersion.version) {\n";
    js += "                serverVersion.textContent = data.serverVersion.version;\n";
    js += "                serverVersion.style.color = '#10b981';\n";
    js += "            } else {\n";
    js += "                serverVersion.textContent = '获取失败';\n";
    js += "                serverVersion.style.color = '#ef4444';\n";
    js += "            }\n";
    js += "            \n";
    js += "            showToast('版本信息获取成功', 'success');\n";
    js += "        } else {\n";
    js += "            showToast('版本信息获取失败: ' + data.message, 'error');\n";
    js += "            versionInfo.classList.remove('hidden');\n";
    js += "            serverVersion.textContent = '获取失败';\n";
    js += "            serverVersion.style.color = '#ef4444';\n";
    js += "        }\n";
    js += "    } catch (error) {\n";
    js += "        showToast('网络错误: ' + error.message, 'error');\n";
    js += "        versionInfo.classList.remove('hidden');\n";
    js += "        serverVersion.textContent = '网络错误';\n";
    js += "        serverVersion.style.color = '#ef4444';\n";
    js += "    } finally {\n";
    js += "        checkVersionBtn.disabled = false;\n";
    js += "        checkVersionBtn.textContent = '检查版本';\n";
    js += "    }\n";
    js += "}\n\n";
    
    js += "async function startServerOTA() {\n";
    js += "    const serverOTABtn = document.getElementById('serverOTABtn');\n";
    js += "    \n";
    js += "    if (!confirm('确定要开始服务器OTA升级吗？\\n\\n注意：升级过程中请勿断开电源或WiFi连接。')) {\n";
    js += "        return;\n";
    js += "    }\n";
    js += "    \n";
    js += "    serverOTABtn.disabled = true;\n";
    js += "    serverOTABtn.textContent = '启动中...';\n";
    js += "    \n";
    js += "    try {\n";
    js += "        const formData = new FormData();\n";
    js += "        formData.append('serverUrl', 'http://egota.yingdl.com');\n";
    js += "        formData.append('firmwareFile', 'firmware.bin');\n";
    js += "        \n";
    js += "        const response = await fetch('/api/ota/server-start', {\n";
    js += "            method: 'POST',\n";
    js += "            body: formData\n";
    js += "        });\n";
    js += "        \n";
    js += "        const data = await response.json();\n";
    js += "        \n";
    js += "        if (data.success) {\n";
    js += "            showToast('服务器OTA升级已启动', 'success');\n";
    js += "            \n";
    js += "            document.getElementById('otaProgress').classList.remove('hidden');\n";
    js += "            document.getElementById('otaStatusTitle').textContent = '服务器OTA升级进行中...';\n";
    js += "            \n";
    js += "            otaStatusInterval = setInterval(updateOTAStatus, 1000);\n";
    js += "            \n";
    js += "        } else {\n";
    js += "            showToast('服务器OTA升级启动失败: ' + data.message, 'error');\n";
    js += "            serverOTABtn.disabled = false;\n";
    js += "            serverOTABtn.textContent = '开始服务器升级';\n";
    js += "        }\n";
    js += "    } catch (error) {\n";
    js += "        showToast('网络错误: ' + error.message, 'error');\n";
    js += "        serverOTABtn.disabled = false;\n";
    js += "        serverOTABtn.textContent = '开始服务器升级';\n";
    js += "    }\n";
    js += "}\n\n";
    
    return js;
}

String WebServerManager::getOTAPageCSS() {
    return R"(
        /* OTA页面专用样式 */
        .ota-section {
            display: flex;
            flex-direction: column;
            gap: 24px;
        }
        
        /* OTA选项卡样式 */
        .ota-tabs {
            display: flex;
            background: #f8fafc;
            border-radius: 12px;
            padding: 4px;
            margin-bottom: 24px;
            border: 1px solid #e2e8f0;
        }
        
        .tab-btn {
            flex: 1;
            background: none;
            border: none;
            padding: 12px 16px;
            border-radius: 8px;
            font-size: 1rem;
            font-weight: 600;
            color: #64748b;
            cursor: pointer;
            transition: all 0.3s ease;
            position: relative;
        }
        
        .tab-btn.active {
            background: white;
            color: #3b82f6;
            box-shadow: 0 2px 8px rgba(59, 130, 246, 0.2);
        }
        
        .tab-btn:hover:not(.active) {
            color: #3b82f6;
            background: rgba(59, 130, 246, 0.05);
        }
        
        /* 选项卡内容样式 */
        .tab-content {
            display: none;
            animation: fadeIn 0.3s ease-in-out;
        }
        
        .tab-content.active {
            display: block;
        }
        
        @keyframes fadeIn {
            from { opacity: 0; transform: translateY(10px); }
            to { opacity: 1; transform: translateY(0); }
        }
        
        /* 服务器OTA样式 */
        .server-ota-section {
            display: flex;
            flex-direction: column;
            gap: 24px;
        }
        
        .server-info {
            text-align: center;
            padding: 24px;
            background: linear-gradient(135deg, #e0f2fe, #b3e5fc);
            border-radius: 16px;
            border: 1px solid #0ea5e9;
        }
        
        .server-text {
            font-size: 1.2rem;
            font-weight: 600;
            color: #0c4a6e;
            margin-bottom: 8px;
        }
        
        .server-url {
            color: #0369a1;
            font-weight: 500;
            font-family: monospace;
            background: rgba(255, 255, 255, 0.7);
            padding: 8px 16px;
            border-radius: 8px;
            display: inline-block;
            margin: 0;
        }
        
        .server-actions {
            display: flex;
            gap: 16px;
            justify-content: center;
            flex-wrap: wrap;
        }
        
        .server-btn {
            background: linear-gradient(135deg, #06b6d4, #0891b2);
            color: white;
            border: none;
            padding: 14px 28px;
            border-radius: 12px;
            font-size: 1rem;
            font-weight: 600;
            cursor: pointer;
            transition: all 0.3s ease;
            box-shadow: 0 4px 16px rgba(6, 182, 212, 0.3);
            position: relative;
            overflow: hidden;
        }
        
        .server-btn:hover {
            transform: translateY(-2px);
            box-shadow: 0 6px 20px rgba(6, 182, 212, 0.4);
        }
        
        .server-btn:active {
            transform: translateY(0);
        }
        
        .server-btn.version-btn {
            background: linear-gradient(135deg, #8b5cf6, #7c3aed);
            box-shadow: 0 4px 16px rgba(139, 92, 246, 0.3);
        }
        
        .server-btn.version-btn:hover {
            box-shadow: 0 6px 20px rgba(139, 92, 246, 0.4);
        }
        
        .server-btn.ota-btn {
            background: linear-gradient(135deg, #10b981, #059669);
            box-shadow: 0 4px 16px rgba(16, 185, 129, 0.3);
        }
        
        .server-btn.ota-btn:hover {
            box-shadow: 0 6px 20px rgba(16, 185, 129, 0.4);
        }
        
        .version-info {
            background: white;
            border-radius: 12px;
            padding: 20px;
            border: 1px solid #e2e8f0;
            box-shadow: 0 2px 8px rgba(0, 0, 0, 0.05);
        }
        
        .version-info h4 {
            color: #1f2937;
            margin-bottom: 16px;
            font-size: 1.1rem;
            font-weight: 600;
        }
        
        .version-info p {
            margin-bottom: 8px;
            color: #4b5563;
            font-size: 0.95rem;
        }
        
        .version-info p:last-child {
            margin-bottom: 0;
        }
        
        .version-info strong {
            color: #1f2937;
            font-weight: 600;
        }
        
        .ota-info {
            text-align: center;
            padding: 24px;
            background: linear-gradient(135deg, #f8fafc, #e2e8f0);
            border-radius: 16px;
            border: 1px solid #cbd5e1;
            position: relative;
            overflow: hidden;
        }
        
        .ota-info::before {
            content: '';
            position: absolute;
            top: 0;
            left: -100%;
            width: 100%;
            height: 100%;
            background: linear-gradient(90deg, transparent, rgba(59, 130, 246, 0.1), transparent);
            animation: infoShine 3s infinite;
        }
        
        @keyframes infoShine {
            0% { left: -100%; }
            100% { left: 100%; }
        }
        
        .info-text {
            font-size: 1.2rem;
            font-weight: 600;
            color: #1f2937;
            margin-bottom: 12px;
            display: flex;
            align-items: center;
            justify-content: center;
            gap: 8px;
        }
        

        
        .warning-text {
            color: #dc2626;
            font-weight: 500;
            font-size: 0.95rem;
            display: flex;
            align-items: center;
            justify-content: center;
            gap: 8px;
            padding: 8px 16px;
            background: rgba(220, 38, 38, 0.1);
            border-radius: 8px;
            border: 1px solid rgba(220, 38, 38, 0.2);
            margin: 0 auto;
            max-width: fit-content;
        }
        

        
        /* 文件上传区域 */
        .file-upload-section {
            display: flex;
            flex-direction: column;
            align-items: center;
            gap: 20px;
            padding: 32px;
            background: white;
            border-radius: 20px;
            border: 2px dashed #cbd5e1;
            transition: all 0.3s ease;
            position: relative;
            overflow: hidden;
        }
        
        .file-upload-section:hover {
            border-color: #3b82f6;
            background: #f8fafc;
            transform: translateY(-2px);
            box-shadow: 0 8px 30px rgba(59, 130, 246, 0.1);
        }
        
        .file-select-btn {
            background: linear-gradient(135deg, #3b82f6, #1d4ed8);
            color: white;
            border: none;
            padding: 16px 32px;
            border-radius: 16px;
            font-size: 1.1rem;
            font-weight: 600;
            cursor: pointer;
            transition: all 0.3s cubic-bezier(0.4, 0, 0.2, 1);
            display: flex;
            align-items: center;
            justify-content: center;
            gap: 10px;
            box-shadow: 0 4px 20px rgba(59, 130, 246, 0.3);
            position: relative;
            overflow: hidden;
        }
        

        
        .file-select-btn::after {
            content: '';
            position: absolute;
            top: 0;
            left: -100%;
            width: 100%;
            height: 100%;
            background: linear-gradient(90deg, transparent, rgba(255,255,255,0.2), transparent);
            transition: left 0.5s;
        }
        
        .file-select-btn:hover::after {
            left: 100%;
        }
        
        .file-select-btn:hover {
            transform: translateY(-3px) scale(1.02);
            box-shadow: 0 8px 30px rgba(59, 130, 246, 0.4);
        }
        
        .file-select-btn:active {
            transform: translateY(-1px) scale(0.98);
        }
        
        /* 文件信息显示 */
        .file-info {
            background: linear-gradient(135deg, #f0f9ff, #e0f2fe);
            border: 1px solid #0ea5e9;
            border-radius: 12px;
            padding: 20px;
            width: 100%;
            max-width: 400px;
            animation: slideInDown 0.5s ease-out;
        }
        
        @keyframes slideInDown {
            from {
                opacity: 0;
                transform: translateY(-20px);
            }
            to {
                opacity: 1;
                transform: translateY(0);
            }
        }
        
        .file-info p {
            margin-bottom: 8px;
            font-size: 0.95rem;
            color: #0f172a;
        }
        
        .file-info p:last-child {
            margin-bottom: 0;
        }
        
        .file-info strong {
            color: #0369a1;
            font-weight: 600;
        }
        
        /* 上传按钮区域 */
        .upload-section {
            display: flex;
            justify-content: center;
            align-items: center;
        }
        
        .upload-btn {
            background: linear-gradient(135deg, #10b981, #059669);
            color: white;
            border: none;
            padding: 18px 40px;
            border-radius: 16px;
            font-size: 1.2rem;
            font-weight: 700;
            cursor: pointer;
            transition: all 0.3s cubic-bezier(0.4, 0, 0.2, 1);
            display: flex;
            align-items: center;
            justify-content: center;
            gap: 12px;
            box-shadow: 0 6px 25px rgba(16, 185, 129, 0.3);
            position: relative;
            overflow: hidden;
            min-width: 200px;
            min-height: 56px;
        }
        
        .upload-btn::before {
            content: '';
            position: absolute;
            top: 0;
            left: -100%;
            width: 100%;
            height: 100%;
            background: linear-gradient(90deg, transparent, rgba(255,255,255,0.2), transparent);
            transition: left 0.5s;
        }
        
        .upload-btn:hover:not(:disabled)::before {
            left: 100%;
        }
        
        .upload-btn:hover:not(:disabled) {
            transform: translateY(-3px) scale(1.05);
            box-shadow: 0 10px 40px rgba(16, 185, 129, 0.4);
        }
        
        .upload-btn:disabled {
            background: linear-gradient(135deg, #9ca3af, #6b7280);
            cursor: not-allowed;
            transform: none;
            box-shadow: 0 2px 8px rgba(156, 163, 175, 0.2);
        }
        
        .btn-loading {
            display: flex;
            align-items: center;
            gap: 8px;
        }
        
        /* 进度显示区域 */
        .ota-progress {
            background: white;
            border-radius: 20px;
            padding: 32px;
            border: 1px solid #e2e8f0;
            box-shadow: 0 4px 20px rgba(0,0,0,0.08);
            animation: fadeInUp 0.5s ease-out;
        }
        
        @keyframes fadeInUp {
            from {
                opacity: 0;
                transform: translateY(20px);
            }
            to {
                opacity: 1;
                transform: translateY(0);
            }
        }
        
        .progress-info {
            text-align: center;
            margin-bottom: 24px;
        }
        
        .progress-info h3 {
            font-size: 1.4rem;
            font-weight: 700;
            color: #1f2937;
            margin-bottom: 8px;
            display: flex;
            align-items: center;
            justify-content: center;
            gap: 10px;
        }
        

        
        @keyframes rotate {
            from { transform: rotate(0deg); }
            to { transform: rotate(360deg); }
        }
        
        .progress-info p {
            color: #6b7280;
            font-size: 1rem;
        }
        
        /* 进度条美化 */
        .progress-bar {
            width: 100%;
            height: 12px;
            background: #f1f5f9;
            border-radius: 6px;
            overflow: hidden;
            margin-bottom: 16px;
            position: relative;
            box-shadow: inset 0 2px 4px rgba(0,0,0,0.1);
        }
        
        .progress-fill {
            height: 100%;
            background: linear-gradient(90deg, #10b981, #059669, #34d399);
            border-radius: 6px;
            width: 0%;
            transition: width 0.3s ease;
            position: relative;
            overflow: hidden;
        }
        
        .progress-fill::after {
            content: '';
            position: absolute;
            top: 0;
            left: 0;
            bottom: 0;
            right: 0;
            background: linear-gradient(
                90deg,
                transparent,
                rgba(255,255,255,0.4),
                transparent
            );
            animation: progressShine 2s infinite;
        }
        
        @keyframes progressShine {
            0% { transform: translateX(-100%); }
            100% { transform: translateX(100%); }
        }
        
        .progress-text {
            display: flex;
            justify-content: space-between;
            align-items: center;
            font-size: 0.9rem;
            color: #4b5563;
            font-weight: 500;
        }
        
        /* 结果显示区域 */
        .ota-result {
            background: white;
            border-radius: 20px;
            padding: 40px;
            text-align: center;
            box-shadow: 0 8px 30px rgba(0,0,0,0.1);
            animation: bounceIn 0.6s ease-out;
        }
        
        @keyframes bounceIn {
            0% {
                opacity: 0;
                transform: scale(0.3);
            }
            50% {
                opacity: 1;
                transform: scale(1.05);
            }
            70% {
                transform: scale(0.9);
            }
            100% {
                opacity: 1;
                transform: scale(1);
            }
        }
        
        .result-success {
            color: #059669;
        }
        
        .result-success h3 {
            font-size: 1.8rem;
            font-weight: 700;
            margin-bottom: 16px;
            display: flex;
            align-items: center;
            justify-content: center;
            gap: 12px;
        }
        

        
        .result-success p {
            font-size: 1.1rem;
            line-height: 1.6;
        }
        
        @keyframes pulse {
            0%, 100% { transform: scale(1); }
            50% { transform: scale(1.1); }
        }
        
        .result-error {
            color: #dc2626;
        }
        
        .result-error h3 {
            font-size: 1.6rem;
            font-weight: 700;
            margin-bottom: 16px;
            display: flex;
            align-items: center;
            justify-content: center;
            gap: 12px;
        }
        

        
        .result-error p {
            font-size: 1rem;
            margin-bottom: 20px;
            line-height: 1.6;
        }
        
        .retry-btn {
            background: linear-gradient(135deg, #f59e0b, #d97706);
            color: white;
            border: none;
            padding: 12px 24px;
            border-radius: 12px;
            font-size: 1rem;
            font-weight: 600;
            cursor: pointer;
            transition: all 0.3s ease;
        }
        
        .retry-btn:hover {
            transform: translateY(-2px);
            box-shadow: 0 8px 25px rgba(245, 158, 11, 0.4);
        }
        
        /* 加载动画 */
        .spinner-sm {
            width: 20px;
            height: 20px;
            border: 2px solid rgba(255,255,255,0.3);
            border-radius: 50%;
            border-top-color: white;
            animation: spin 1s ease-in-out infinite;
        }
        
        @keyframes spin {
            to { transform: rotate(360deg); }
        }
        
        /* 隐藏元素 */
        .hidden {
            display: none !important;
        }
        
        /* 响应式设计 */
        @media (max-width: 768px) {
            .file-upload-section {
                padding: 24px 16px;
            }
            
            .file-select-btn {
                padding: 14px 24px;
                font-size: 1rem;
            }
            
            .upload-btn {
                padding: 16px 32px;
                font-size: 1.1rem;
                min-width: 180px;
            }
            
            .ota-progress, .ota-result {
                padding: 24px 20px;
            }
            
            .result-success h3, .result-error h3 {
                font-size: 1.4rem;
            }
        }
        
        @media (max-width: 480px) {
            .container {
                padding: 16px;
            }
            
            .card {
                padding: 20px;
                padding-top: 60px;
            }
            
            .file-upload-section {
                padding: 20px 12px;
            }
            
            .upload-btn {
                min-width: 160px;
                padding: 14px 24px;
            }
        }
    )";
}
