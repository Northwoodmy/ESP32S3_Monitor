/*
 * WebServerManager_ServerSettings.cpp - Web服务器服务器设置页面实现
 * ESP32S3监控项目 - 服务器设置和连接管理模块
 */

#include "WebServerManager.h"

String WebServerManager::getServerSettingsHTML() {
    String html = "<!DOCTYPE html>\n";
    html += "<html lang=\"zh-CN\">\n";
    html += "<head>\n";
    html += "    <meta charset=\"UTF-8\">\n";
    html += "    <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\n";
    html += "    <title>服务器设置 - ESP32S3 Monitor</title>\n";
    html += "    <style>\n";
    html += getCSS();
    html += getServerSettingsCSS();
    html += "    </style>\n";
    html += "</head>\n";
    html += "<body>\n";
    html += "    <div class=\"container\">\n";
    html += "        <header class=\"header\">\n";
    html += "            <h1>小屏幕配置</h1>\n";
    html += "            <div class=\"subtitle\">服务器设置</div>\n";
    html += "        </header>\n";
    html += "        \n";
    html += "        <div class=\"card\">\n";
    html += "            <button onclick=\"window.location.href='/'\" class=\"back-home-btn\">\n";
    html += "                返回首页\n";
    html += "            </button>\n";
    html += "            \n";
    html += "            <!-- 服务器状态显示 -->\n";
    html += "            <div class=\"settings-section\">\n";
    html += "                <h2>服务器状态</h2>\n";
    html += "                <div class=\"server-status\">\n";
    html += "                    <div class=\"status-card\">\n";
    html += "                        <div class=\"server-info\" id=\"serverStatusInfo\">\n";
    html += "                            <div class=\"status-loading\">\n";
    html += "                                <div class=\"spinner\"></div>\n";
    html += "                                <span>检查服务器状态中...</span>\n";
    html += "                            </div>\n";
    html += "                        </div>\n";
    html += "                    </div>\n";
    html += "                </div>\n";
    html += "            </div>\n";
    html += "            \n";
    html += "            <!-- 服务器配置 -->\n";
    html += "            <div class=\"settings-section\">\n";
    html += "                <h2>服务器配置</h2>\n";
    html += "                <div class=\"server-config\">\n";
    html += "                    <div class=\"setting-group\">\n";
    html += "                        <h3>基础配置</h3>\n";
    html += "                        <div class=\"server-basic-config\">\n";
    html += "                            <p class=\"setting-description\">\n";
    html += "                                配置监控服务器地址，用于获取实时数据和指标信息。\n";
    html += "                            </p>\n";
    html += "                            <div class=\"form-group\">\n";
    html += "                                <label for=\"serverUrl\">服务器IP地址:</label>\n";
    html += "                                <input type=\"text\" id=\"serverUrl\" class=\"setting-input\" placeholder=\"10.10.168.168\">\n";
    html += "                                <div class=\"scan-button-container\">\n";
    html += "                                    <button type=\"button\" onclick=\"startMDNSScan()\" class=\"setting-btn info-btn scan-btn\">\n";
    html += "                                        <span class=\"btn-text\">扫描cp02服务器</span>\n";
    html += "                                        <div class=\"btn-loading hidden\">\n";
    html += "                                            <div class=\"spinner-sm\"></div>\n";
    html += "                                            <span>扫描中...</span>\n";
    html += "                                        </div>\n";
    html += "                                    </button>\n";
    html += "                                </div>\n";
    html += "                                <div class=\"url-help\">\n";
    html += "                                    <small>请输入服务器IP地址或点击扫描按钮自动发现cp02服务器</small>\n";
    html += "                                </div>\n";
    html += "                            </div>\n";
    html += "                            \n";
    html += "                            <!-- mDNS扫描结果 -->\n";
    html += "                            <div class=\"mdns-scan-results\" id=\"mdnsScanResults\" style=\"display: none;\">\n";
    html += "                                <h4>cp02服务器扫描结果</h4>\n";
    html += "                                <div class=\"device-list\" id=\"deviceList\">\n";
    html += "                                    <!-- 扫描结果将显示在这里 -->\n";
    html += "                                </div>\n";
    html += "                            </div>\n";
    html += "                        </div>\n";
    html += "                    </div>\n";
    html += "                    \n";
    html += "                    <div class=\"setting-group\">\n";
    html += "                        <h3>请求设置</h3>\n";
    html += "                        <div class=\"request-config\">\n";
    html += "                            <p class=\"setting-description\">\n";
    html += "                                配置数据请求的相关参数，包括请求间隔和超时设置。\n";
    html += "                            </p>\n";
    html += "                            <div class=\"form-row\">\n";
    html += "                                <div class=\"form-group\">\n";
    html += "                                    <label for=\"requestInterval\">请求间隔 (毫秒):</label>\n";
    html += "                                    <input type=\"number\" id=\"requestInterval\" class=\"setting-input\" min=\"100\" max=\"1000\" value=\"250\">\n";
    html += "                                    <small>建议设置为100-1000毫秒</small>\n";
    html += "                                </div>\n";
    html += "                                <div class=\"form-group\">\n";
    html += "                                    <label for=\"connectionTimeout\">连接超时 (毫秒):</label>\n";
    html += "                                    <input type=\"number\" id=\"connectionTimeout\" class=\"setting-input\" min=\"1000\" max=\"60000\" value=\"1000\">\n";
    html += "                                    <small>默认1000毫秒，不建议修改</small>\n";
    html += "                                </div>\n";
    html += "                            </div>\n";
    html += "                            <div class=\"form-row\">\n";
    html += "                                <div class=\"form-group\">\n";
    html += "                                    <label class=\"switch-label\">\n";
    html += "                                        <input type=\"checkbox\" id=\"autoGetData\" class=\"switch-input\" checked>\n";
    html += "                                        <span class=\"switch-slider\"></span>\n";
    html += "                                        自动获取数据\n";
    html += "                                    </label>\n";
    html += "                                    <small>启用后将自动从服务器获取数据</small>\n";
    html += "                                </div>\n";
    html += "                                <div class=\"form-group\">\n";
    html += "                                    <label class=\"switch-label\">\n";
    html += "                                        <input type=\"checkbox\" id=\"autoScanServer\" class=\"switch-input\">\n";
    html += "                                        <span class=\"switch-slider\"></span>\n";
    html += "                                        自动扫描服务器\n";
    html += "                                    </label>\n";
    html += "                                    <small>启用后将自动扫描网络中的cp02服务器</small>\n";
    html += "                                </div>\n";
    html += "                            </div>\n";
    html += "                        </div>\n";
    html += "                    </div>\n";
    html += "                    \n";
    html += "                    <div class=\"setting-group\">\n";
    html += "                        <h3>操作功能</h3>\n";
    html += "                        <div class=\"server-actions\">\n";
    html += "                            <button onclick=\"testServerConnection()\" class=\"setting-btn primary-btn\">\n";
    html += "                                <span class=\"btn-text\">测试连接</span>\n";
    html += "                                <div class=\"btn-loading hidden\">\n";
    html += "                                    <div class=\"spinner-sm\"></div>\n";
    html += "                                    <span>测试中...</span>\n";
    html += "                                </div>\n";
    html += "                            </button>\n";
    html += "                            <button onclick=\"saveServerConfig()\" class=\"setting-btn success-btn\">\n";
    html += "                                <span class=\"btn-text\">保存配置</span>\n";
    html += "                                <div class=\"btn-loading hidden\">\n";
    html += "                                    <div class=\"spinner-sm\"></div>\n";
    html += "                                    <span>保存中...</span>\n";
    html += "                                </div>\n";
    html += "                            </button>\n";
    html += "                        </div>\n";
    html += "                    </div>\n";
    html += "                </div>\n";
    html += "            </div>\n";
    html += "            \n";
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
    html += getServerSettingsJavaScript();
    html += "    </script>\n";
    html += "</body>\n";
    html += "</html>\n";
    
    return html;
}

String WebServerManager::getServerSettingsCSS() {
    return R"(
        /* 服务器设置页面样式 - 与时间设置页面保持一致 */
        .settings-section {
            margin-bottom: 32px;
            position: relative;
            animation: slideIn 0.6s ease-out;
        }
        
        .settings-section h2 {
            font-size: 1.8rem;
            font-weight: 700;
            color: #1f2937;
            margin-bottom: 24px;
            display: flex;
            align-items: center;
            gap: 12px;
        }
        
        /* 服务器状态显示区域 */
        .server-status {
            margin-bottom: 24px;
        }
        
        .status-card {
            background: linear-gradient(135deg, #10b981 0%, #059669 50%, #047857 100%);
            border-radius: 24px;
            padding: 32px;
            color: white;
            text-align: center;
            box-shadow: 
                0 20px 60px rgba(16, 185, 129, 0.4),
                0 8px 20px rgba(16, 185, 129, 0.2),
                inset 0 1px 0 rgba(255, 255, 255, 0.2);
            position: relative;
            overflow: hidden;
            border: 1px solid rgba(255, 255, 255, 0.1);
            backdrop-filter: blur(10px);
            display: flex;
            flex-direction: column;
            align-items: center;
            justify-content: center;
        }
        
        .status-card::before {
            content: '';
            position: absolute;
            top: -50%;
            right: -50%;
            width: 100%;
            height: 100%;
            background: linear-gradient(45deg, rgba(255,255,255,0.15), transparent);
            animation: shine 4s infinite;
            opacity: 0.8;
        }
        
        .server-info {
            font-size: 1.3rem;
            font-weight: 600;
            position: relative;
            z-index: 2;
            text-align: center;
            width: 100%;
            display: flex;
            flex-direction: column;
            align-items: center;
            justify-content: center;
        }
        
        .status-loading {
            display: flex;
            align-items: center;
            justify-content: center;
            gap: 16px;
            padding: 40px 20px;
            font-size: 1.1rem;
            font-weight: 500;
            text-align: center;
            width: 100%;
        }
        
        .status-loading .spinner {
            width: 32px;
            height: 32px;
            border: 3px solid rgba(255, 255, 255, 0.3);
            border-radius: 50%;
            border-top-color: #ffffff;
            animation: statusSpin 1.2s ease-in-out infinite;
        }
        
        @keyframes statusSpin {
            to { transform: rotate(360deg); }
        }
        
        .status-success {
            padding: 32px 20px;
            text-align: center;
            font-size: 1.1rem;
            font-weight: 500;
            color: rgba(255, 255, 255, 0.95);
            display: flex;
            flex-direction: column;
            align-items: center;
            justify-content: center;
            width: 100%;
            gap: 12px;
        }
        
        .status-error {
            padding: 32px 20px;
            text-align: center;
            font-size: 1.1rem;
            font-weight: 500;
            color: rgba(255, 255, 255, 0.9);
            background: rgba(239, 68, 68, 0.2);
            border-radius: 12px;
            border: 1px solid rgba(239, 68, 68, 0.3);
            display: flex;
            align-items: center;
            justify-content: center;
            width: 100%;
        }
        
        /* 服务器配置区域 */
        .server-config {
            display: grid;
            gap: 20px;
        }
        
        .setting-group {
            background: linear-gradient(145deg, #ffffff, #f8fafc);
            border-radius: 16px;
            padding: 24px;
            border: 1px solid #e2e8f0;
            box-shadow: 0 4px 20px rgba(0,0,0,0.08);
            transition: all 0.3s ease;
            position: relative;
            animation: fadeInUp 0.6s ease-out forwards;
            opacity: 0;
        }
        
        .setting-group:nth-child(1) {
            animation-delay: 0.1s;
        }
        
        .setting-group:nth-child(2) {
            animation-delay: 0.2s;
        }
        
        .setting-group:nth-child(3) {
            animation-delay: 0.3s;
        }
        
        .setting-group:hover {
            transform: translateY(-2px);
            box-shadow: 0 8px 30px rgba(0,0,0,0.12);
        }
        
        .setting-group h3 {
            font-size: 1.3rem;
            font-weight: 600;
            color: #374151;
            margin-bottom: 16px;
            display: flex;
            align-items: center;
            gap: 10px;
        }
        
        .setting-description {
            color: #6b7280;
            font-size: 0.9rem;
            line-height: 1.5;
            margin-bottom: 16px;
            padding: 12px;
            background: #f1f5f9;
            border-radius: 8px;
            border-left: 4px solid #3b82f6;
        }
        
        /* 表单样式 */
        .form-group {
            margin-bottom: 20px;
        }
        
        .form-row {
            display: grid;
            grid-template-columns: repeat(auto-fit, minmax(200px, 1fr));
            gap: 16px;
            margin-bottom: 20px;
        }
        
        .form-group label {
            display: block;
            font-weight: 500;
            color: #374151;
            margin-bottom: 8px;
            display: flex;
            align-items: center;
            gap: 8px;
        }
        
        .setting-input {
            width: 100%;
            padding: 12px 16px;
            border: 2px solid #e5e7eb;
            border-radius: 12px;
            font-size: 1rem;
            background: white;
            transition: all 0.3s ease;
            margin-bottom: 8px;
        }
        
        .setting-input:focus {
            outline: none;
            border-color: #14b8a6;
            box-shadow: 0 0 0 3px rgba(20, 184, 166, 0.1);
            background-color: #fefefe;
        }
        
        .url-help small {
            color: #6b7280;
            font-size: 0.85rem;
            line-height: 1.4;
        }
        
        /* 开关样式 */
        .switch-label {
            display: flex;
            align-items: center;
            gap: 12px;
            cursor: pointer;
            font-weight: 500;
            color: #374151;
            margin-bottom: 16px;
        }
        
        .switch-input {
            display: none;
        }
        
        .switch-slider {
            position: relative;
            width: 50px;
            height: 24px;
            background: #e5e7eb;
            border-radius: 12px;
            transition: all 0.3s ease;
        }
        
        .switch-slider::before {
            content: '';
            position: absolute;
            top: 2px;
            left: 2px;
            width: 20px;
            height: 20px;
            background: white;
            border-radius: 10px;
            transition: all 0.3s ease;
            box-shadow: 0 2px 4px rgba(0,0,0,0.2);
        }
        
        .switch-input:checked + .switch-slider {
            background: #14b8a6;
        }
        
        .switch-input:checked + .switch-slider::before {
            transform: translateX(26px);
        }
        
        /* 扫描按钮样式 */
        .scan-button-container {
            margin-top: 8px;
        }
        
        .scan-btn {
            width: 100%;
            justify-content: center;
            margin-bottom: 8px;
        }
        
        .info-btn {
            background: linear-gradient(135deg, #3b82f6 0%, #2563eb 100%);
            color: white;
            border: none;
        }
        
        .info-btn:hover {
            background: linear-gradient(135deg, #2563eb 0%, #1d4ed8 100%);
        }
        
        /* mDNS扫描结果样式 */
        .mdns-scan-results {
            margin-top: 16px;
            padding: 16px;
            background: #f8fafc;
            border-radius: 12px;
            border: 1px solid #e2e8f0;
        }
        
        .mdns-scan-results h4 {
            font-size: 1.1rem;
            font-weight: 600;
            color: #374151;
            margin-bottom: 12px;
        }
        
        .device-list {
            display: grid;
            gap: 8px;
        }
        
        .device-item {
            display: flex;
            align-items: center;
            justify-content: space-between;
            padding: 12px;
            background: white;
            border-radius: 8px;
            border: 1px solid #e2e8f0;
            cursor: pointer;
            transition: all 0.2s ease;
        }
        
        .device-item:hover {
            background: #f1f5f9;
            border-color: #3b82f6;
        }
        
        .device-info {
            flex-grow: 1;
        }
        
        .device-name {
            font-weight: 500;
            color: #374151;
            font-size: 0.9rem;
        }
        
        .device-ip {
            color: #6b7280;
            font-size: 0.85rem;
            margin-top: 2px;
        }
        
        .device-details {
            color: #9ca3af;
            font-size: 0.8rem;
            margin-top: 2px;
        }
        
        .device-select {
            display: flex;
            align-items: center;
            padding: 4px 8px;
            background: #3b82f6;
            color: white;
            border-radius: 4px;
            font-size: 0.8rem;
            font-weight: 500;
        }
        
        .no-devices {
            text-align: center;
            padding: 20px;
            color: #6b7280;
            font-style: italic;
        }
        
        /* 按钮样式 */
        .server-actions {
            display: flex;
            gap: 12px;
            margin-top: 16px;
        }
        
        .setting-btn {
            flex: 1;
            padding: 12px 24px;
            border: none;
            border-radius: 12px;
            font-weight: 500;
            cursor: pointer;
            transition: all 0.3s ease;
            position: relative;
            overflow: hidden;
            display: flex;
            align-items: center;
            justify-content: center;
            gap: 8px;
            min-height: 44px;
        }
        
        .primary-btn {
            background: linear-gradient(135deg, #3b82f6 0%, #2563eb 100%);
            color: white;
        }
        
        .primary-btn:hover {
            background: linear-gradient(135deg, #2563eb 0%, #1d4ed8 100%);
            transform: translateY(-2px);
        }
        
        .success-btn {
            background: linear-gradient(135deg, #10b981 0%, #059669 100%);
            color: white;
        }
        
        .success-btn:hover {
            background: linear-gradient(135deg, #059669 0%, #047857 100%);
            transform: translateY(-2px);
        }
        
        .btn-loading {
            display: flex;
            align-items: center;
            gap: 8px;
        }
        
        .spinner-sm {
            width: 16px;
            height: 16px;
            border: 2px solid rgba(255, 255, 255, 0.3);
            border-radius: 50%;
            border-top-color: #ffffff;
            animation: spin 1s linear infinite;
        }
        
        @keyframes spin {
            to { transform: rotate(360deg); }
        }
        
        .hidden {
            display: none;
        }
        
        /* 响应式设计 */
        @media (max-width: 768px) {
            .form-row {
                grid-template-columns: 1fr;
            }
            
            .server-actions {
                flex-direction: column;
            }
            
            .setting-btn {
                width: 100%;
            }
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
        
        @keyframes slideIn {
            from {
                opacity: 0;
                transform: translateX(-20px);
            }
            to {
                opacity: 1;
                transform: translateX(0);
            }
        }
        
        @keyframes shine {
            0% {
                transform: translateX(-100%) translateY(-100%) rotate(45deg);
            }
            100% {
                transform: translateX(100%) translateY(100%) rotate(45deg);
            }
                 }
     )";
}

String WebServerManager::getServerSettingsJavaScript() {
    String js = "";
    
    // 基础工具函数
    js += "function showToast(message, type) {\n";
    js += "    const toast = document.getElementById('toast');\n";
    js += "    const toastMessage = document.getElementById('toastMessage');\n";
    js += "    toastMessage.textContent = message;\n";
    js += "    toast.className = 'toast show ' + (type || 'info');\n";
    js += "    setTimeout(() => { toast.className = 'toast hidden'; }, 3000);\n";
    js += "}\n\n";
    
    // 页面初始化
    js += "document.addEventListener('DOMContentLoaded', function() {\n";
    js += "    initServerSettings();\n";
    js += "    loadServerConfig();\n";
    js += "    checkServerStatus();\n";
    js += "});\n\n";
    
    js += "function initServerSettings() {\n";
    js += "    console.log('初始化服务器设置页面');\n";
    js += "}\n\n";
    
    js += "function loadServerConfig() {\n";
    js += "    fetch('/api/server/config')\n";
    js += "        .then(response => response.json())\n";
    js += "        .then(data => {\n";
    js += "            if (data.success && data.config) {\n";
    js += "                const serverUrl = document.getElementById('serverUrl');\n";
    js += "                const requestInterval = document.getElementById('requestInterval');\n";
    js += "                const connectionTimeout = document.getElementById('connectionTimeout');\n";
    js += "                \n";
    js += "                // 从完整URL中提取IP地址\n";
    js += "                if (serverUrl) {\n";
    js += "                    const fullUrl = data.config.serverUrl || '';\n";
    js += "                    if (fullUrl.includes('://')) {\n";
    js += "                        const urlParts = fullUrl.split('://');\n";
    js += "                        if (urlParts.length > 1) {\n";
    js += "                            const hostPart = urlParts[1].split('/')[0];\n";
    js += "                            serverUrl.value = hostPart;\n";
    js += "                        } else {\n";
    js += "                            serverUrl.value = fullUrl;\n";
    js += "                        }\n";
    js += "                    } else {\n";
    js += "                        serverUrl.value = fullUrl;\n";
    js += "                    }\n";
    js += "                }\n";
    js += "                if (requestInterval) requestInterval.value = data.config.requestInterval || 250;\n";
    js += "                if (connectionTimeout) connectionTimeout.value = data.config.connectionTimeout || 1000;\n";
    js += "                \n";
    js += "                // 设置开关状态\n";
    js += "                const autoGetData = document.getElementById('autoGetData');\n";
    js += "                if (autoGetData) autoGetData.checked = data.config.autoGetData !== undefined ? data.config.autoGetData : true;\n";
    js += "                \n";
    js += "                const autoScanServer = document.getElementById('autoScanServer');\n";
    js += "                if (autoScanServer) autoScanServer.checked = data.config.autoScanServer !== undefined ? data.config.autoScanServer : false;\n";
    js += "                \n";
    js += "                console.log('服务器配置加载成功:', data.config);\n";
    js += "            } else {\n";
    js += "                console.log('使用默认配置:', data.message);\n";
    js += "            }\n";
    js += "        })\n";
    js += "        .catch(error => {\n";
    js += "            console.error('加载配置失败:', error);\n";
    js += "            showToast('加载配置失败', 'error');\n";
    js += "        });\n";
    js += "}\n\n";
    
    js += "function checkServerStatus() {\n";
    js += "    const statusInfo = document.getElementById('serverStatusInfo');\n";
    js += "    if (!statusInfo) return;\n";
    js += "    \n";
    js += "    // 显示加载状态\n";
    js += "    statusInfo.innerHTML = `\n";
    js += "        <div class=\"status-loading\">\n";
    js += "            <div class=\"spinner\"></div>\n";
    js += "            <span>检查服务器状态中...</span>\n";
    js += "        </div>\n";
    js += "    `;\n";
    js += "    \n";
    js += "    // 延时1秒后执行状态检查\n";
    js += "    setTimeout(() => {\n";
    js += "        statusInfo.innerHTML = `\n";
    js += "            <div class=\"status-success\">\n";
    js += "                <h3>服务器设置已就绪</h3>\n";
    js += "                <p>可以配置和测试服务器连接</p>\n";
    js += "            </div>\n";
    js += "        `;\n";
    js += "    }, 1000);\n";
    js += "}\n\n";
    
    js += "function saveServerConfig() {\n";
    js += "    const saveBtn = document.querySelector('.success-btn');\n";
    js += "    const btnText = saveBtn.querySelector('.btn-text');\n";
    js += "    const btnLoading = saveBtn.querySelector('.btn-loading');\n";
    js += "    \n";
    js += "    // 显示加载状态\n";
    js += "    btnText.style.display = 'none';\n";
    js += "    btnLoading.classList.remove('hidden');\n";
    js += "    \n";
    js += "    const serverIp = document.getElementById('serverUrl').value.trim();\n";
    js += "    const requestInterval = parseInt(document.getElementById('requestInterval').value) || 250;\n";
    js += "    const connectionTimeout = parseInt(document.getElementById('connectionTimeout').value) || 1000;\n";
    js += "    const autoGetData = document.getElementById('autoGetData').checked;\n";
    js += "    const autoScanServer = document.getElementById('autoScanServer').checked;\n";
    js += "    const enabled = true;\n";
    js += "    \n";
    js += "    // 基本验证\n";
    js += "    if (!serverIp) {\n";
    js += "        showToast('请输入服务器IP地址', 'warning');\n";
    js += "        resetSaveButton();\n";
    js += "        return;\n";
    js += "    }\n";
    js += "    \n";
    js += "    // IP地址格式验证\n";
    js += "    const ipPattern = /^(\\d{1,3}\\.){3}\\d{1,3}$/;\n";
    js += "    if (!ipPattern.test(serverIp)) {\n";
    js += "        showToast('请输入有效的IP地址格式', 'warning');\n";
    js += "        resetSaveButton();\n";
    js += "        return;\n";
    js += "    }\n";
    js += "    \n";
    js += "    // 自动拼接完整URL\n";
    js += "    const serverUrl = 'http://' + serverIp + '/metrics.json';\n";
    js += "    \n";
    js += "    const formData = new FormData();\n";
    js += "    formData.append('serverUrl', serverUrl);\n";
    js += "    formData.append('requestInterval', requestInterval.toString());\n";
    js += "    formData.append('connectionTimeout', connectionTimeout.toString());\n";
    js += "    formData.append('autoGetData', autoGetData.toString());\n";
    js += "    formData.append('autoScanServer', autoScanServer.toString());\n";
    js += "    formData.append('enabled', enabled.toString());\n";
    js += "    \n";
    js += "    fetch('/api/server/config', {\n";
    js += "        method: 'POST',\n";
    js += "        body: formData\n";
    js += "    })\n";
    js += "    .then(response => response.json())\n";
    js += "    .then(data => {\n";
    js += "        if (data.success) {\n";
    js += "            showToast('服务器配置保存成功', 'success');\n";
    js += "        } else {\n";
    js += "            showToast('保存失败: ' + data.message, 'error');\n";
    js += "        }\n";
    js += "    })\n";
    js += "    .catch(error => {\n";
    js += "        console.error('保存配置失败:', error);\n";
    js += "        showToast('保存配置失败', 'error');\n";
    js += "    })\n";
    js += "    .finally(() => {\n";
    js += "        resetSaveButton();\n";
    js += "    });\n";
    js += "}\n\n";
    
    js += "function testServerConnection() {\n";
    js += "    const testBtn = document.querySelector('.primary-btn');\n";
    js += "    const btnText = testBtn.querySelector('.btn-text');\n";
    js += "    const btnLoading = testBtn.querySelector('.btn-loading');\n";
    js += "    const statusInfo = document.getElementById('serverStatusInfo');\n";
    js += "    \n";
    js += "    // 显示加载状态\n";
    js += "    btnText.style.display = 'none';\n";
    js += "    btnLoading.classList.remove('hidden');\n";
    js += "    \n";
    js += "    // 更新状态显示\n";
    js += "    if (statusInfo) {\n";
    js += "        statusInfo.innerHTML = `\n";
    js += "            <div class=\"status-loading\">\n";
    js += "                <div class=\"spinner\"></div>\n";
    js += "                <span>正在测试服务器连接...</span>\n";
    js += "            </div>\n";
    js += "        `;\n";
    js += "    }\n";
    js += "    \n";
    js += "    const serverIp = document.getElementById('serverUrl').value.trim();\n";
    js += "    \n";
    js += "    const formData = new FormData();\n";
    js += "    if (serverIp) {\n";
    js += "        const serverUrl = 'http://' + serverIp + '/metrics.json';\n";
    js += "        formData.append('serverUrl', serverUrl);\n";
    js += "    }\n";
    js += "    \n";
    js += "    fetch('/api/server/test', {\n";
    js += "        method: 'POST',\n";
    js += "        body: formData\n";
    js += "    })\n";
    js += "    .then(response => response.json())\n";
    js += "    .then(data => {\n";
    js += "        if (data.success) {\n";
    js += "            showToast('服务器连接测试成功', 'success');\n";
    js += "            \n";
    js += "            if (statusInfo) {\n";
    js += "                statusInfo.innerHTML = `\n";
    js += "                    <div class=\"status-success\">\n";
    js += "                        <h3>连接成功</h3>\n";
    js += "                        <p style=\"color: #10b981;\">✓ 服务器连接正常</p>\n";
    js += "                    </div>\n";
    js += "                `;\n";
    js += "            }\n";
    js += "        } else {\n";
    js += "            showToast('连接测试失败: ' + data.message, 'error');\n";
    js += "            \n";
    js += "            if (statusInfo) {\n";
    js += "                statusInfo.innerHTML = `\n";
    js += "                    <div class=\"status-error\">\n";
    js += "                        <h3>连接失败</h3>\n";
    js += "                        <p>错误: ${data.message}</p>\n";
    js += "                    </div>\n";
    js += "                `;\n";
    js += "            }\n";
    js += "        }\n";
    js += "    })\n";
    js += "    .catch(error => {\n";
    js += "        console.error('测试连接失败:', error);\n";
    js += "        showToast('测试连接失败', 'error');\n";
    js += "        \n";
    js += "        if (statusInfo) {\n";
    js += "            statusInfo.innerHTML = `\n";
    js += "                <div class=\"status-error\">\n";
    js += "                    <h3>连接错误</h3>\n";
    js += "                    <p>网络或服务器错误</p>\n";
    js += "                </div>\n";
    js += "            `;\n";
    js += "        }\n";
    js += "    })\n";
    js += "    .finally(() => {\n";
    js += "        resetTestButton();\n";
    js += "    });\n";
    js += "}\n\n";
    
    js += "function resetSaveButton() {\n";
    js += "    const saveBtn = document.querySelector('.success-btn');\n";
    js += "    const btnText = saveBtn.querySelector('.btn-text');\n";
    js += "    const btnLoading = saveBtn.querySelector('.btn-loading');\n";
    js += "    \n";
    js += "    btnText.style.display = '';\n";
    js += "    btnLoading.classList.add('hidden');\n";
    js += "}\n\n";
    
    js += "function resetTestButton() {\n";
    js += "    const testBtn = document.querySelector('.primary-btn');\n";
    js += "    const btnText = testBtn.querySelector('.btn-text');\n";
    js += "    const btnLoading = testBtn.querySelector('.btn-loading');\n";
    js += "    \n";
    js += "    btnText.style.display = '';\n";
    js += "    btnLoading.classList.add('hidden');\n";
    js += "}\n\n";
    
    // mDNS扫描相关函数
    js += "function startMDNSScan() {\n";
    js += "    const scanBtn = document.querySelector('.scan-btn');\n";
    js += "    const btnText = scanBtn.querySelector('.btn-text');\n";
    js += "    const btnLoading = scanBtn.querySelector('.btn-loading');\n";
    js += "    const scanResults = document.getElementById('mdnsScanResults');\n";
    js += "    const deviceList = document.getElementById('deviceList');\n";
    js += "    \n";
    js += "    btnText.style.display = 'none';\n";
    js += "    btnLoading.classList.remove('hidden');\n";
    js += "    scanResults.style.display = 'none';\n";
    js += "    \n";
    js += "    fetch('/api/server/mdns-scan')\n";
    js += "        .then(response => response.json())\n";
    js += "        .then(data => {\n";
    js += "            if (data.success) {\n";
    js += "                displayScanResults(data.devices);\n";
    js += "                if (data.devices.length > 0) {\n";
    js += "                    showToast('扫描完成，发现 ' + data.devices.length + ' 个cp02服务器', 'success');\n";
    js += "                } else {\n";
    js += "                    showToast('未发现任何cp02服务器', 'info');\n";
    js += "                }\n";
    js += "            } else {\n";
    js += "                showToast('扫描失败: ' + data.message, 'error');\n";
    js += "            }\n";
    js += "        })\n";
    js += "        .catch(error => {\n";
    js += "            showToast('cp02服务器扫描失败', 'error');\n";
    js += "        })\n";
    js += "        .finally(() => {\n";
    js += "            resetScanButton();\n";
    js += "        });\n";
    js += "}\n\n";
    
    js += "function displayScanResults(devices) {\n";
    js += "    const scanResults = document.getElementById('mdnsScanResults');\n";
    js += "    const deviceList = document.getElementById('deviceList');\n";
    js += "    \n";
    js += "    if (!devices || devices.length === 0) {\n";
    js += "        deviceList.innerHTML = '<div class=\"no-devices\">未发现任何cp02服务器</div>';\n";
    js += "        scanResults.style.display = 'block';\n";
    js += "        return;\n";
    js += "    }\n";
    js += "    \n";
    js += "    let html = '';\n";
    js += "    devices.forEach((device, index) => {\n";
    js += "        const displayName = device.name || device.hostname || 'Unknown Device';\n";
    js += "        const serviceInfo = device.serviceType ? ' (' + device.serviceType + ')' : '';\n";
    js += "        \n";
    js += "        html += `\n";
    js += "            <div class=\"device-item\" onclick=\"selectDevice('${device.ip}', '${displayName}')\">\n";
    js += "                <div class=\"device-info\">\n";
    js += "                    <div class=\"device-name\">${displayName}${serviceInfo}</div>\n";
    js += "                    <div class=\"device-ip\">${device.ip}:${device.port}</div>\n";
    js += "                    <div class=\"device-details\">${device.customInfo || ''}</div>\n";
    js += "                </div>\n";
    js += "                <div class=\"device-select\">\n";
    js += "                    <span class=\"select-text\">选择</span>\n";
    js += "                </div>\n";
    js += "            </div>\n";
    js += "        `;\n";
    js += "    });\n";
    js += "    \n";
    js += "    deviceList.innerHTML = html;\n";
    js += "    scanResults.style.display = 'block';\n";
    js += "}\n\n";
    
    js += "function selectDevice(ip, name) {\n";
    js += "    const serverUrlInput = document.getElementById('serverUrl');\n";
    js += "    const scanResults = document.getElementById('mdnsScanResults');\n";
    js += "    \n";
    js += "    serverUrlInput.value = ip;\n";
    js += "    scanResults.style.display = 'none';\n";
    js += "    showToast('已选择cp02服务器: ' + name + ' (' + ip + ')', 'success');\n";
    js += "}\n\n";
    
    js += "function resetScanButton() {\n";
    js += "    const scanBtn = document.querySelector('.scan-btn');\n";
    js += "    const btnText = scanBtn.querySelector('.btn-text');\n";
    js += "    const btnLoading = scanBtn.querySelector('.btn-loading');\n";
    js += "    \n";
    js += "    btnText.style.display = '';\n";
    js += "    btnLoading.classList.add('hidden');\n";
    js += "}\n";
    
    return js;
} 