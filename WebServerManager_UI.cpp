/*
 * WebServerManager_UI.cpp - Web服务器UI部分实现
 * ESP32S3监控项目 - Web界面模块
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
    html += "                        <div class=\"loading\" id=\"scanLoading\">\n";
    html += "                            <div class=\"spinner\"></div>\n";
    html += "                            <span>扫描中...</span>\n";
    html += "                        </div>\n";
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
    html += "                            <div class=\"password-toggle\" onclick=\"togglePassword()\">👁</div>\n";
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
    html += "                        <span class=\"value\" id=\"firmwareVersion\">加载中...</span>\n";
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
    html += "                        <span class=\"label\">总内存:</span>\n";
    html += "                        <span class=\"value\" id=\"totalHeap\">加载中...</span>\n";
    html += "                    </div>\n";
    html += "                    <div class=\"info-item\">\n";
    html += "                        <span class=\"label\">可用内存:</span>\n";
    html += "                        <span class=\"value\" id=\"freeHeap\">加载中...</span>\n";
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
    html += "                    <button onclick=\"refreshInfo()\" class=\"primary-btn\">\n";
    html += "                        刷新信息\n";
    html += "                    </button>\n";
    html += "                    <button onclick=\"resetConfig()\" class=\"danger-btn\">\n";
    html += "                        恢复默认配置\n";
    html += "                    </button>\n";
    html += "                    <button onclick=\"rebootDevice()\" class=\"warning-btn\">\n";
    html += "                        重启设备\n";
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
            margin-bottom: 10px;
            text-shadow: 0 2px 4px rgba(0,0,0,0.3);
        }
        
        .subtitle {
            font-size: 1.1rem;
            opacity: 0.9;
        }
        
        .card {
            background: white;
            border-radius: 16px;
            padding: 24px;
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
        .tab-nav {
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
        }
        
        .saved-wifi-ssid {
            font-weight: 600;
            color: #1f2937;
            font-size: 1.1rem;
            margin-bottom: 4px;
        }
        
        .saved-wifi-details {
            color: #6b7280;
            font-size: 0.875rem;
        }
        
        .saved-wifi-actions {
            display: flex;
            gap: 8px;
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
            background: #3b82f6;
            color: white;
            padding: 4px 8px;
            border-radius: 6px;
            font-size: 0.75rem;
            font-weight: 500;
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
        }
        
        .action-buttons {
            display: grid;
            grid-template-columns: 1fr 1fr 1fr;
            gap: 12px;
        }
        
        .primary-btn, .danger-btn, .warning-btn, .secondary-btn {
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
            position: absolute;
            top: 50%;
            left: 50%;
            transform: translate(-50%, -50%);
        }
        
        .btn-loading .spinner-sm {
            border-color: rgba(255,255,255,0.3);
            border-top-color: white;
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
    js += "        document.getElementById('freeHeap').textContent = formatBytes(data.system.freeHeap);\n";
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
    js += "        document.getElementById('totalHeap').textContent = formatBytes(data.totalHeap);\n";
    js += "        document.getElementById('freeHeap').textContent = formatBytes(data.freeHeap);\n";
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
    js += "    const scanLoading = document.getElementById('scanLoading');\n";
    js += "    scanBtn.disabled = true;\n";
    js += "    scanBtn.innerHTML = '<div class=\"spinner-sm\"></div> 扫描中...';\n";
    js += "    networkList.classList.remove('hidden');\n";
    js += "    scanLoading.classList.remove('hidden');\n";
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
    js += "        scanLoading.classList.add('hidden');\n";
    js += "    }\n";
    js += "}\n\n";
    
    js += "function displayNetworks(networks) {\n";
    js += "    const networkList = document.getElementById('networkList');\n";
    js += "    const scanLoading = document.getElementById('scanLoading');\n";
    js += "    scanLoading.classList.add('hidden');\n";
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
    js += "    for (let i = 1; i <= 4; i++) {\n";
    js += "        const height = i * 4 + 4;\n";
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
    
    js += "function togglePassword() {\n";
    js += "    const passwordInput = document.getElementById('password');\n";
    js += "    const toggleBtn = document.querySelector('.password-toggle');\n";
    js += "    if (passwordInput.type === 'password') {\n";
    js += "        passwordInput.type = 'text';\n";
    js += "        toggleBtn.textContent = '🙈';\n";
    js += "    } else {\n";
    js += "        passwordInput.type = 'password';\n";
    js += "        toggleBtn.textContent = '👁';\n";
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
    js += "    let html = '';\n";
    js += "    for (let i = 0; i < configs.length; i++) {\n";
    js += "        const config = configs[i];\n";
    js += "        html += '<div class=\"saved-wifi-item\">';\n";
    js += "        html += '<div class=\"saved-wifi-info\">';\n";
    js += "        html += '<div class=\"saved-wifi-ssid\">' + config.ssid + '</div>';\n";
    js += "        html += '<div class=\"saved-wifi-details\">';\n";
    js += "        html += '<span class=\"priority-badge\">优先级 ' + (i + 1) + '</span>';\n";
    js += "        html += '<span style=\"margin-left: 10px;\">密码: ' + config.password + '</span>';\n";
    js += "        html += '</div>';\n";
    js += "        html += '</div>';\n";
    js += "        html += '<div class=\"saved-wifi-actions\">';\n";
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
    
    js += "window.addEventListener('beforeunload', function() {\n";
    js += "    if (statusInterval) { clearInterval(statusInterval); }\n";
    js += "});";
    
    return js;
} 