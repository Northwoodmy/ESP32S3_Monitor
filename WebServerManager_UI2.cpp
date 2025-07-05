/*
 * WebServerManager_UI2.cpp - Web服务器UI设置页面实现
 * ESP32S3监控项目 - 设置页面和扩展功能模块
 */

#include "WebServerManager.h"



String WebServerManager::getSystemSettingsHTML() {
    String html = "<!DOCTYPE html>\n";
    html += "<html lang=\"zh-CN\">\n";
    html += "<head>\n";
    html += "    <meta charset=\"UTF-8\">\n";
    html += "    <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\n";
    html += "    <title>时间设置 - ESP32S3 Monitor</title>\n";
    html += "    <style>\n";
    html += getCSS();
    html += getSystemSettingsCSS();
    html += "    </style>\n";
    html += "</head>\n";
    html += "<body>\n";
    html += "    <div class=\"container\">\n";
    html += "        <header class=\"header\">\n";
    html += "            <h1>小屏幕配置</h1>\n";
    html += "            <div class=\"subtitle\">时间设置</div>\n";
    html += "        </header>\n";
    html += "        \n";
    html += "        <div class=\"card\">\n";
    html += "            <button onclick=\"window.location.href='/'\" class=\"back-home-btn\">\n";
    html += "                返回首页\n";
    html += "            </button>\n";
    html += "            \n";
    html += "            <!-- 时间配置部分 -->\n";
    html += "            <div class=\"settings-section\">\n";
    html += "                <h2>时间配置</h2>\n";
    html += "                <div class=\"time-config\">\n";
    html += "                    <div class=\"current-time-display\">\n";
    html += "                        <div class=\"time-card\">\n";
    html += "                            <h3>当前时间</h3>\n";
    html += "                            <div class=\"time-display\" id=\"currentTime\">\n";
    html += "                                <div class=\"time-loading\">\n";
    html += "                                    <div class=\"spinner\"></div>\n";
    html += "                                    <span>加载中...</span>\n";
    html += "                                </div>\n";
    html += "                            </div>\n";
    html += "                            <div class=\"time-info\" id=\"timeInfo\">\n";
    html += "                                加载时间信息中...\n";
    html += "                            </div>\n";
    html += "                        </div>\n";
    html += "                    </div>\n";
    html += "                    \n";
    html += "                    <div class=\"time-settings\">\n";
    html += "                        <div class=\"setting-group\">\n";
    html += "                            <h3>时区设置</h3>\n";
    html += "                            <div class=\"timezone-selector\">\n";
    html += "                                <label for=\"timezoneSelect\">选择时区:</label>\n";
    html += "                                <select id=\"timezoneSelect\" class=\"timezone-select\">\n";
    html += "                                    <option value=\"UTC+8\" selected>北京时间 (UTC+8)</option>\n";
    html += "                                    <option value=\"UTC+0\">协调世界时 (UTC+0)</option>\n";
    html += "                                    <option value=\"UTC+9\">东京时间 (UTC+9)</option>\n";
    html += "                                    <option value=\"UTC-5\">纽约时间 (UTC-5)</option>\n";
    html += "                                    <option value=\"UTC-8\">洛杉矶时间 (UTC-8)</option>\n";
    html += "                                </select>\n";
    html += "                                <button onclick=\"setTimezone()\" class=\"setting-btn primary-btn\">\n";
    html += "                                    应用时区\n";
    html += "                                </button>\n";
    html += "                            </div>\n";
    html += "                        </div>\n";
    html += "                        \n";
    html += "                        <div class=\"setting-group\">\n";
    html += "                            <h3>NTP同步</h3>\n";
    html += "                            <div class=\"ntp-controls\">\n";
    html += "                                <p class=\"setting-description\">\n";
    html += "                                    通过网络时间协议(NTP)自动同步时间，需要连接WiFi网络。\n";
    html += "                                </p>\n";
    html += "                                <div class=\"ntp-status\" id=\"ntpStatus\">\n";
    html += "                                    <span class=\"status-indicator\" id=\"wifiStatusIndicator\">\n";
    html += "                                        <div class=\"status-dot\"></div>\n";
    html += "                                    </span>\n";
    html += "                                    <span id=\"wifiStatusText\">检查WiFi状态中...</span>\n";
    html += "                                </div>\n";
    html += "                                <button onclick=\"syncTime()\" class=\"setting-btn success-btn\" id=\"syncTimeBtn\">\n";
    html += "                                    <span class=\"btn-text\">立即同步时间</span>\n";
    html += "                                    <div class=\"btn-loading hidden\">\n";
    html += "                                        <div class=\"spinner-sm\"></div>\n";
    html += "                                        <span>同步中...</span>\n";
    html += "                                    </div>\n";
    html += "                                </button>\n";
    html += "                            </div>\n";
    html += "                        </div>\n";
    html += "                        \n";
    html += "                        <div class=\"setting-group\">\n";
    html += "                            <h3>手动时间设置</h3>\n";
    html += "                            <div class=\"manual-time-setting\">\n";
    html += "                                <p class=\"setting-description\">\n";
    html += "                                    在没有网络连接时，可以手动设置当前时间。\n";
    html += "                                </p>\n";
    html += "                                <div class=\"datetime-input\">\n";
    html += "                                    <label for=\"manualDateTime\">设置时间:</label>\n";
    html += "                                    <input type=\"datetime-local\" id=\"manualDateTime\" class=\"datetime-picker\">\n";
    html += "                                    <button onclick=\"setManualTime()\" class=\"setting-btn warning-btn\">\n";
    html += "                                        设置时间\n";
    html += "                                    </button>\n";
    html += "                                </div>\n";
    html += "                            </div>\n";
    html += "                        </div>\n";
    html += "                    </div>\n";
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
    html += getSystemSettingsJavaScript();
    html += "    </script>\n";
    html += "</body>\n";
    html += "</html>\n";
    
    return html;
}



String WebServerManager::getSystemSettingsJavaScript() {
    String js = "";
    
    // 基础工具函数
    js += "function showToast(message, type) {\n";
    js += "    const toast = document.getElementById('toast');\n";
    js += "    const toastMessage = document.getElementById('toastMessage');\n";
    js += "    toastMessage.textContent = message;\n";
    js += "    toast.className = 'toast show ' + (type || 'info');\n";
    js += "    setTimeout(() => { toast.className = 'toast hidden'; }, 3000);\n";
    js += "}\n\n";
    
    js += "function formatUptime(milliseconds) {\n";
    js += "    const seconds = Math.floor(milliseconds / 1000);\n";
    js += "    const days = Math.floor(seconds / 86400);\n";
    js += "    const hours = Math.floor((seconds % 86400) / 3600);\n";
    js += "    const minutes = Math.floor((seconds % 3600) / 60);\n";
    js += "    const secs = seconds % 60;\n";
    js += "    if (days > 0) return `${days}天 ${hours}小时 ${minutes}分钟`;\n";
    js += "    if (hours > 0) return `${hours}小时 ${minutes}分钟 ${secs}秒`;\n";
    js += "    if (minutes > 0) return `${minutes}分钟 ${secs}秒`;\n";
    js += "    return `${secs}秒`;\n";
    js += "}\n\n";
    
    js += "let timeUpdateInterval;\n\n";
    
    js += "document.addEventListener('DOMContentLoaded', function() {\n";
    js += "    initSystemSettings();\n";
    js += "    loadTimeConfig();\n";
    js += "    checkWiFiStatus();\n";
    js += "    startTimeUpdate();\n";
    js += "});\n\n";
    
    js += "function initSystemSettings() {\n";
    js += "    // 设置当前时间到手动时间输入框\n";
    js += "    const now = new Date();\n";
    js += "    const localISOTime = now.getFullYear() + '-' +\n";
    js += "        String(now.getMonth() + 1).padStart(2, '0') + '-' +\n";
    js += "        String(now.getDate()).padStart(2, '0') + 'T' +\n";
    js += "        String(now.getHours()).padStart(2, '0') + ':' +\n";
    js += "        String(now.getMinutes()).padStart(2, '0');\n";
    js += "    document.getElementById('manualDateTime').value = localISOTime;\n";
    js += "}\n\n";
    
    js += "async function loadTimeConfig() {\n";
    js += "    try {\n";
    js += "        const response = await fetch('/api/time/config');\n";
    js += "        const data = await response.json();\n";
    js += "        if (data.success) {\n";
    js += "            updateTimeDisplay(data);\n";
    js += "        } else {\n";
    js += "            showTimeError('加载时间配置失败');\n";
    js += "        }\n";
    js += "    } catch (error) {\n";
    js += "        showTimeError('连接服务器失败');\n";
    js += "    }\n";
    js += "}\n\n";
    
    js += "async function checkWiFiStatus() {\n";
    js += "    try {\n";
    js += "        const response = await fetch('/info');\n";
    js += "        const data = await response.json();\n";
    js += "        \n";
    js += "        // 更新WiFi状态\n";
    js += "        const wifiConnected = data.wifi && data.wifi.status === 'connected';\n";
    js += "        updateWiFiStatus(wifiConnected, data.wifi);\n";
    js += "        \n";
    js += "    } catch (error) {\n";
    js += "        updateWiFiStatus(false);\n";
    js += "    }\n";
    js += "}\n\n";
    
    js += "function updateTimeDisplay(data) {\n";
    js += "    const timeEl = document.getElementById('currentTime');\n";
    js += "    const infoEl = document.getElementById('timeInfo');\n";
    js += "    \n";
    js += "    if (data.currentTime) {\n";
    js += "        timeEl.innerHTML = `<div class=\"time-text\">${data.currentTime.trim()}</div>`;\n";
    js += "        infoEl.innerHTML = `时区: ${data.timezone} | NTP: ${data.ntpEnabled ? '启用' : '禁用'}`;\n";
    js += "    } else {\n";
    js += "        showTimeError('时间数据无效');\n";
    js += "    }\n";
    js += "}\n\n";
    
    js += "function showTimeError(message) {\n";
    js += "    const timeEl = document.getElementById('currentTime');\n";
    js += "    const infoEl = document.getElementById('timeInfo');\n";
    js += "    timeEl.innerHTML = `<div class=\"time-error\">${message}</div>`;\n";
    js += "    infoEl.textContent = '时间信息获取失败';\n";
    js += "}\n\n";
    
    js += "function updateWiFiStatus(connected, wifiData) {\n";
    js += "    const indicator = document.getElementById('wifiStatusIndicator');\n";
    js += "    const text = document.getElementById('wifiStatusText');\n";
    js += "    const syncBtn = document.getElementById('syncTimeBtn');\n";
    js += "    \n";
    js += "    if (connected) {\n";
    js += "        indicator.className = 'status-indicator connected';\n";
    js += "        text.textContent = `已连接到 ${wifiData.ssid} (${wifiData.ip})`;\n";
    js += "        syncBtn.disabled = false;\n";
    js += "    } else {\n";
    js += "        indicator.className = 'status-indicator disconnected';\n";
    js += "        text.textContent = '未连接WiFi，无法使用NTP同步';\n";
    js += "        syncBtn.disabled = true;\n";
    js += "    }\n";
    js += "}\n\n";
    
    js += "function startTimeUpdate() {\n";
    js += "    timeUpdateInterval = setInterval(function() {\n";
    js += "        loadTimeConfig();\n";
    js += "        checkWiFiStatus();\n";
    js += "    }, 30000); // 每30秒更新一次时间和WiFi状态\n";
    js += "}\n\n";
    

    
    js += "async function setTimezone() {\n";
    js += "    const timezone = document.getElementById('timezoneSelect').value;\n";
    js += "    try {\n";
    js += "        const formData = new FormData();\n";
    js += "        formData.append('timezone', timezone);\n";
    js += "        \n";
    js += "        const response = await fetch('/api/time/config', {\n";
    js += "            method: 'POST',\n";
    js += "            body: formData\n";
    js += "        });\n";
    js += "        \n";
    js += "        const data = await response.json();\n";
    js += "        if (data.success) {\n";
    js += "            showToast('时区设置成功', 'success');\n";
    js += "            setTimeout(loadTimeConfig, 1000); // 1秒后重新加载时间配置\n";
    js += "        } else {\n";
    js += "            showToast('时区设置失败: ' + data.message, 'error');\n";
    js += "        }\n";
    js += "    } catch (error) {\n";
    js += "        showToast('时区设置失败: 网络错误', 'error');\n";
    js += "    }\n";
    js += "}\n\n";
    
    js += "async function syncTime() {\n";
    js += "    const syncBtn = document.getElementById('syncTimeBtn');\n";
    js += "    const btnText = syncBtn.querySelector('.btn-text');\n";
    js += "    const btnLoading = syncBtn.querySelector('.btn-loading');\n";
    js += "    \n";
    js += "    // 显示加载状态\n";
    js += "    syncBtn.disabled = true;\n";
    js += "    btnText.style.display = 'none';\n";
    js += "    btnLoading.classList.remove('hidden');\n";
    js += "    \n";
    js += "    try {\n";
    js += "        const response = await fetch('/api/time/sync', { method: 'POST' });\n";
    js += "        const data = await response.json();\n";
    js += "        \n";
    js += "        if (data.success) {\n";
    js += "            showToast('时间同步成功', 'success');\n";
    js += "            setTimeout(loadTimeConfig, 1000); // 1秒后重新加载时间配置\n";
    js += "        } else {\n";
    js += "            showToast('时间同步失败: ' + data.message, 'error');\n";
    js += "        }\n";
    js += "    } catch (error) {\n";
    js += "        showToast('时间同步失败: 网络错误', 'error');\n";
    js += "    } finally {\n";
    js += "        // 恢复按钮状态\n";
    js += "        syncBtn.disabled = false;\n";
    js += "        btnText.style.display = '';\n";
    js += "        btnLoading.classList.add('hidden');\n";
    js += "    }\n";
    js += "}\n\n";
    
    js += "function setManualTime() {\n";
    js += "    const manualTime = document.getElementById('manualDateTime').value;\n";
    js += "    if (!manualTime) {\n";
    js += "        showToast('请选择要设置的时间', 'warning');\n";
    js += "        return;\n";
    js += "    }\n";
    js += "    \n";
    js += "    // 这里可以添加手动设置时间的逻辑\n";
    js += "    showToast('手动时间设置功能待实现', 'info');\n";
    js += "}\n\n";
    
    js += "// 页面卸载时清理定时器\n";
    js += "window.addEventListener('beforeunload', function() {\n";
    js += "    if (timeUpdateInterval) clearInterval(timeUpdateInterval);\n";
    js += "});\n";
    
    return js;
}



String WebServerManager::getSystemSettingsCSS() {
    return R"(
        /* 系统设置页面专用样式 */
        .settings-section {
            margin-bottom: 32px;
            position: relative;
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
        

        
        /* 时间配置区域 */
        .time-config {
            display: grid;
            gap: 24px;
        }
        
        .current-time-display .time-card {
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            border-radius: 20px;
            padding: 24px;
            color: white;
            text-align: center;
            box-shadow: 0 12px 40px rgba(102, 126, 234, 0.3);
            position: relative;
            overflow: hidden;
        }
        
        .time-card::before {
            content: '';
            position: absolute;
            top: -50%;
            right: -50%;
            width: 100%;
            height: 100%;
            background: linear-gradient(45deg, rgba(255,255,255,0.1), transparent);
            animation: shine 3s infinite;
        }
        
        @keyframes shine {
            0% { transform: translateX(-100%) translateY(-100%) rotate(45deg); }
            100% { transform: translateX(100%) translateY(100%) rotate(45deg); }
        }
        
        .time-card h3 {
            font-size: 1.2rem;
            margin-bottom: 16px;
            opacity: 0.9;
            display: flex;
            align-items: center;
            justify-content: center;
            gap: 8px;
        }
        

        
        .time-display {
            font-size: 2rem;
            font-weight: 600;
            margin-bottom: 12px;
            text-shadow: 0 2px 4px rgba(0,0,0,0.2);
        }
        
        .time-text {
            animation: timeGlow 2s ease-in-out infinite alternate;
        }
        
        @keyframes timeGlow {
            from { text-shadow: 0 2px 4px rgba(0,0,0,0.2); }
            to { text-shadow: 0 2px 4px rgba(0,0,0,0.2), 0 0 20px rgba(255,255,255,0.3); }
        }
        
        .time-info {
            font-size: 0.9rem;
            opacity: 0.8;
        }
        
        .time-loading {
            display: flex;
            align-items: center;
            justify-content: center;
            gap: 12px;
        }
        
        .time-error {
            color: #fca5a5;
            font-weight: 500;
        }
        
        /* 设置组样式 */
        .time-settings {
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
        
        /* 时区选择器样式 */
        .timezone-selector {
            display: flex;
            flex-direction: column;
            gap: 16px;
        }
        
        .timezone-selector label {
            font-weight: 500;
            color: #374151;
            display: flex;
            align-items: center;
            gap: 8px;
        }
        

        
        .timezone-select {
            width: 100%;
            padding: 12px 16px;
            border: 2px solid #e5e7eb;
            border-radius: 12px;
            font-size: 1rem;
            background: white;
            transition: all 0.3s ease;
            appearance: none;
            background-image: url("data:image/svg+xml,%3csvg xmlns='http://www.w3.org/2000/svg' fill='none' viewBox='0 0 20 20'%3e%3cpath stroke='%236b7280' stroke-linecap='round' stroke-linejoin='round' stroke-width='1.5' d='m6 8 4 4 4-4'/%3e%3c/svg%3e");
            background-position: right 12px center;
            background-repeat: no-repeat;
            background-size: 16px;
            padding-right: 40px;
        }
        
        .timezone-select:focus {
            outline: none;
            border-color: #3b82f6;
            box-shadow: 0 0 0 3px rgba(59, 130, 246, 0.1);
            background-color: #fefefe;
        }
        
        /* NTP状态样式 */
        .ntp-status {
            display: flex;
            align-items: center;
            gap: 12px;
            padding: 16px;
            background: #f8fafc;
            border-radius: 12px;
            margin-bottom: 16px;
            border: 1px solid #e2e8f0;
        }
        
        .status-indicator {
            position: relative;
            width: 32px;
            height: 32px;
            border-radius: 50%;
            display: flex;
            align-items: center;
            justify-content: center;
            transition: all 0.3s ease;
        }
        
        .status-indicator.connected {
            background: linear-gradient(135deg, #10b981, #059669);
            box-shadow: 0 4px 12px rgba(16, 185, 129, 0.3);
        }
        
        .status-indicator.disconnected {
            background: linear-gradient(135deg, #ef4444, #dc2626);
            box-shadow: 0 4px 12px rgba(239, 68, 68, 0.3);
        }
        
        .status-dot {
            width: 12px;
            height: 12px;
            background: white;
            border-radius: 50%;
            animation: statusPulse 2s infinite;
        }
        
        @keyframes statusPulse {
            0%, 100% { transform: scale(1); opacity: 1; }
            50% { transform: scale(1.2); opacity: 0.8; }
        }
        
        /* 按钮样式重新设计 */
        .setting-btn {
            padding: 14px 24px;
            border: none;
            border-radius: 12px;
            font-weight: 600;
            font-size: 0.95rem;
            cursor: pointer;
            transition: all 0.3s cubic-bezier(0.4, 0, 0.2, 1);
            display: inline-flex;
            align-items: center;
            justify-content: center;
            gap: 8px;
            position: relative;
            overflow: hidden;
            text-decoration: none;
        }
        
        .setting-btn::before {
            content: '';
            position: absolute;
            top: 0;
            left: -100%;
            width: 100%;
            height: 100%;
            background: linear-gradient(90deg, transparent, rgba(255,255,255,0.2), transparent);
            transition: left 0.5s;
        }
        
        .setting-btn:hover::before {
            left: 100%;
        }
        
        .primary-btn {
            background: linear-gradient(135deg, #3b82f6, #1d4ed8);
            color: white;
            box-shadow: 0 4px 15px rgba(59, 130, 246, 0.3);
        }
        
        .primary-btn:hover {
            transform: translateY(-2px);
            box-shadow: 0 8px 25px rgba(59, 130, 246, 0.4);
        }
        
        .success-btn {
            background: linear-gradient(135deg, #10b981, #059669);
            color: white;
            box-shadow: 0 4px 15px rgba(16, 185, 129, 0.3);
        }
        
        .success-btn:hover:not(:disabled) {
            transform: translateY(-2px);
            box-shadow: 0 8px 25px rgba(16, 185, 129, 0.4);
        }
        
        .success-btn:disabled {
            background: linear-gradient(135deg, #9ca3af, #6b7280);
            cursor: not-allowed;
            transform: none;
            box-shadow: none;
        }
        
        .warning-btn {
            background: linear-gradient(135deg, #f59e0b, #d97706);
            color: white;
            box-shadow: 0 4px 15px rgba(245, 158, 11, 0.3);
        }
        
        .warning-btn:hover {
            transform: translateY(-2px);
            box-shadow: 0 8px 25px rgba(245, 158, 11, 0.4);
        }
        
        /* 日期时间输入框样式 */
        .datetime-input {
            display: flex;
            flex-direction: column;
            gap: 16px;
        }
        
        .datetime-input label {
            font-weight: 500;
            color: #374151;
            display: flex;
            align-items: center;
            gap: 8px;
        }
        

        
        .datetime-picker {
            width: 100%;
            padding: 12px 16px;
            border: 2px solid #e5e7eb;
            border-radius: 12px;
            font-size: 1rem;
            background: white;
            transition: all 0.3s ease;
        }
        
        .datetime-picker:focus {
            outline: none;
            border-color: #f59e0b;
            box-shadow: 0 0 0 3px rgba(245, 158, 11, 0.1);
            background-color: #fefefe;
        }
        
        /* 系统信息区域 */
        .system-status {
            display: grid;
            gap: 16px;
        }
        
        .status-item {
            display: flex;
            justify-content: space-between;
            align-items: center;
            padding: 16px;
            background: linear-gradient(135deg, #f8fafc, #f1f5f9);
            border-radius: 12px;
            border: 1px solid #e2e8f0;
            transition: all 0.3s ease;
        }
        
        .status-item:hover {
            transform: translateX(4px);
            box-shadow: 0 4px 12px rgba(0,0,0,0.05);
        }
        
        .status-item .label {
            font-weight: 500;
            color: #4b5563;
            display: flex;
            align-items: center;
            gap: 8px;
        }
        

        
        .status-item .value {
            font-weight: 600;
            color: #1f2937;
            background: white;
            padding: 6px 12px;
            border-radius: 8px;
            box-shadow: 0 2px 4px rgba(0,0,0,0.05);
        }
        
        /* 加载状态 */
        .spinner {
            width: 20px;
            height: 20px;
            border: 2px solid rgba(255,255,255,0.3);
            border-radius: 50%;
            border-top-color: white;
            animation: spin 1s ease-in-out infinite;
        }
        
        .spinner-sm {
            width: 16px;
            height: 16px;
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
        
        .btn-loading {
            display: flex;
            align-items: center;
            gap: 8px;
        }
        
        /* 响应式设计 */
        @media (max-width: 768px) {
            .container {
                padding: 16px;
            }
            
            .card {
                padding: 20px;
                padding-top: 60px;
            }
            
            .settings-section h2 {
                font-size: 1.5rem;
            }
            
            .time-display {
                font-size: 1.6rem;
            }
            
            .setting-group {
                padding: 20px;
            }
        }
        
        /* 时间设置页面的特殊动画 */
        .settings-section {
            animation: slideIn 0.6s ease-out;
        }
        
        @keyframes slideIn {
            from {
                opacity: 0;
                transform: translateY(20px);
            }
            to {
                opacity: 1;
                transform: translateY(0);
            }
        }
        
        .setting-group {
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
        
        @keyframes fadeInUp {
            from {
                opacity: 0;
                transform: translateY(30px);
            }
            to {
                opacity: 1;
                transform: translateY(0);
            }
        }
    )";
}

// 屏幕设置HTML已移至WebServerManager_ScreenSettings.cpp

// 屏幕设置CSS和JavaScript已移至WebServerManager_ScreenSettings.cpp

// 服务器设置HTML已移至WebServerManager_ServerSettings.cpp

// 服务器设置CSS和JavaScript已移至WebServerManager_ServerSettings.cpp
