/*
 * WebServerManager_ScreenSettings.cpp - Web服务器屏幕设置页面实现
 * ESP32S3监控项目 - 屏幕设置和显示管理模块
 */

#include "WebServerManager.h"

String WebServerManager::getScreenSettingsHTML() {
    String html = "<!DOCTYPE html>\n";
    html += "<html lang=\"zh-CN\">\n";
    html += "<head>\n";
    html += "    <meta charset=\"UTF-8\">\n";
    html += "    <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\n";
    html += "    <title>屏幕设置 - ESP32S3 Monitor</title>\n";
    html += "    <style>\n";
    html += getCSS();
    html += getScreenSettingsCSS();
    html += "    </style>\n";
    html += "</head>\n";
    html += "<body>\n";
    html += "    <div class=\"container\">\n";
    html += "        <header class=\"header\">\n";
    html += "            <h1>小屏幕配置</h1>\n";
    html += "            <div class=\"subtitle\">屏幕设置</div>\n";
    html += "        </header>\n";
    html += "        \n";
    html += "        <div class=\"card\">\n";
    html += "            <button onclick=\"window.location.href='/'\" class=\"back-home-btn\">\n";
    html += "                返回首页\n";
    html += "            </button>\n";
    html += "            \n";
    html += "            <!-- 屏幕模式设置部分 -->\n";
    html += "            <div class=\"settings-section\">\n";
    html += "                <h2>屏幕模式配置</h2>\n";
    html += "                <div class=\"screen-mode-config\">\n";
    html += "                    <div class=\"mode-selector\">\n";
    html += "                        <h3>选择屏幕模式</h3>\n";
    html += "                        <div class=\"mode-slider-container\">\n";
    html += "                            <div class=\"mode-slider\" id=\"modeSlider\">\n";
    html += "                                <div class=\"slider-track\">\n";
    html += "                                    <div class=\"slider-indicator\" id=\"sliderIndicator\"></div>\n";
    html += "                                </div>\n";
    html += "                                <div class=\"slider-options\">\n";
    html += "                                    <div class=\"slider-option\" data-mode=\"0\">\n";
    html += "                                        <div class=\"option-label\">亮屏</div>\n";
    html += "                                    </div>\n";
    html += "                                    <div class=\"slider-option\" data-mode=\"1\">\n";
    html += "                                        <div class=\"option-label\">定时</div>\n";
    html += "                                    </div>\n";
    html += "                                    <div class=\"slider-option\" data-mode=\"2\">\n";
    html += "                                        <div class=\"option-label\">延时</div>\n";
    html += "                                    </div>\n";
    html += "                                    <div class=\"slider-option\" data-mode=\"3\">\n";
    html += "                                        <div class=\"option-label\">熄屏</div>\n";
    html += "                                    </div>\n";
    html += "                                </div>\n";
    html += "                            </div>\n";
    html += "                            <div class=\"mode-description\" id=\"modeDescription\">\n";
    html += "                                <span class=\"description-text\">始终保持屏幕点亮</span>\n";
    html += "                            </div>\n";
    html += "                        </div>\n";
    html += "                        <input type=\"hidden\" id=\"selectedMode\" value=\"0\">\n";
    html += "                    </div>\n";
    html += "                    \n";
    html += "                    <!-- 定时模式设置 -->\n";
    html += "                    <div class=\"mode-settings\" id=\"scheduledSettings\" style=\"display: none;\">\n";
    html += "                        <h3>定时模式设置</h3>\n";
    html += "                        <div class=\"setting-description\">\n";
    html += "                            在指定时间段内保持屏幕点亮，其他时间自动关闭屏幕。\n";
    html += "                        </div>\n";
    html += "                        <div class=\"time-range-config\">\n";
    html += "                            <div class=\"time-input-group\">\n";
    html += "                                <label for=\"startTime\">开始时间:</label>\n";
    html += "                                <input type=\"time\" id=\"startTime\" value=\"08:00\" class=\"time-picker\">\n";
    html += "                                <!-- 隐藏的数字输入框保持兼容性 -->\n";
    html += "                                <input type=\"hidden\" id=\"startHour\" value=\"8\">\n";
    html += "                                <input type=\"hidden\" id=\"startMinute\" value=\"0\">\n";
    html += "                            </div>\n";
    html += "                            \n";
    html += "                            <div class=\"time-input-group\">\n";
    html += "                                <label for=\"endTime\">结束时间:</label>\n";
    html += "                                <input type=\"time\" id=\"endTime\" value=\"22:00\" class=\"time-picker\">\n";
    html += "                                <!-- 隐藏的数字输入框保持兼容性 -->\n";
    html += "                                <input type=\"hidden\" id=\"endHour\" value=\"22\">\n";
    html += "                                <input type=\"hidden\" id=\"endMinute\" value=\"0\">\n";
    html += "                            </div>\n";
    html += "                        </div>\n";
    html += "                    </div>\n";
    html += "                    \n";
    html += "                    <!-- 延时模式设置 -->\n";
    html += "                    <div class=\"mode-settings\" id=\"timeoutSettings\" style=\"display: none;\">\n";
    html += "                        <h3>延时模式设置</h3>\n";
    html += "                        <div class=\"timeout-config\">\n";
    html += "                            <div class=\"timeout-input-group\">\n";
    html += "                                <label for=\"timeoutMinutes\">延时时间（分钟）:</label>\n";
    html += "                                <div class=\"timeout-slider\">\n";
    html += "                                    <input type=\"range\" id=\"timeoutSlider\" min=\"1\" max=\"60\" value=\"10\" class=\"slider\">\n";
    html += "                                    <div class=\"timeout-value\">\n";
    html += "                                        <span id=\"timeoutValue\">10</span> 分钟\n";
    html += "                                    </div>\n";
    html += "                                </div>\n";
    html += "                                <input type=\"number\" id=\"timeoutMinutes\" min=\"1\" max=\"1440\" value=\"10\" class=\"timeout-input\">\n";
    html += "                            </div>\n";
    html += "                        </div>\n";
    html += "                    </div>\n";
    html += "                    \n";
    html += "                    <!-- 保存按钮 -->\n";
    html += "                    <div class=\"settings-actions\">\n";
    html += "                        <button onclick=\"saveScreenSettings()\" class=\"save-btn primary-btn\">\n";
    html += "                            <span class=\"btn-text\">保存设置</span>\n";
    html += "                            <div class=\"btn-loading hidden\">\n";
    html += "                                <div class=\"spinner-sm\"></div>\n";
    html += "                                <span>保存中...</span>\n";
    html += "                            </div>\n";
    html += "                        </button>\n";
    html += "                        <button onclick=\"resetSettings()\" class=\"reset-btn warning-btn\">\n";
    html += "                            重置为默认\n";
    html += "                        </button>\n";
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
    html += getScreenSettingsJavaScript();
    html += "    </script>\n";
    html += "</body>\n";
    html += "</html>\n";
    
    return html;
}

String WebServerManager::getScreenSettingsCSS() {
    return R"(
        /* 屏幕设置页面样式 - 与时间设置页面保持一致 */
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
        
        /* 屏幕模式配置区域 */
        .screen-mode-config {
            background: linear-gradient(145deg, #ffffff, #f8fafc);
            border-radius: 16px;
            padding: 24px;
            border: 1px solid #e2e8f0;
            box-shadow: 0 4px 20px rgba(0,0,0,0.08);
            transition: all 0.3s ease;
            animation: fadeInUp 0.6s ease-out forwards;
            opacity: 0;
            animation-delay: 0.1s;
        }
        
        .screen-mode-config:hover {
            transform: translateY(-2px);
            box-shadow: 0 8px 30px rgba(0,0,0,0.12);
        }
        
        .mode-selector h3 {
            font-size: 1.3rem;
            font-weight: 600;
            color: #374151;
            margin-bottom: 16px;
            display: flex;
            align-items: center;
            gap: 10px;
        }
        
        /* 四段式滑块样式 */
        .mode-slider-container {
            margin-bottom: 32px;
        }
        
        .mode-slider {
            position: relative;
            background: #f1f5f9;
            border-radius: 20px;
            padding: 8px;
            margin-bottom: 16px;
            box-shadow: inset 0 2px 4px rgba(0, 0, 0, 0.1);
        }
        
        .slider-track {
            position: relative;
            height: 60px;
            border-radius: 16px;
            overflow: hidden;
        }
        
        .slider-indicator {
            position: absolute;
            top: 0;
            left: 0;
            width: 25%;
            height: 100%;
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            border-radius: 16px;
            transition: all 0.4s cubic-bezier(0.4, 0, 0.2, 1);
            box-shadow: 0 4px 12px rgba(102, 126, 234, 0.4);
            z-index: 2;
        }
        
        .slider-options {
            position: absolute;
            top: 8px;
            left: 8px;
            right: 8px;
            bottom: 8px;
            display: flex;
            z-index: 3;
        }
        
        .slider-option {
            flex: 1;
            display: flex;
            align-items: center;
            justify-content: center;
            cursor: pointer;
            transition: all 0.3s ease;
            border-radius: 12px;
            position: relative;
            user-select: none;
        }
        
        .slider-option:hover {
            background: rgba(255, 255, 255, 0.1);
        }
        
        .slider-option.active {
            color: white;
        }
        
        .option-label {
            font-size: 0.9rem;
            font-weight: 600;
            transition: all 0.3s ease;
        }
        
        .slider-option.active .option-label {
            text-shadow: 0 1px 2px rgba(0, 0, 0, 0.3);
        }
        
        /* 模式描述 */
        .mode-description {
            text-align: center;
            padding: 16px 20px;
            background: #f8fafc;
            border-radius: 12px;
            border: 1px solid #e2e8f0;
            min-height: 50px;
            display: flex;
            align-items: center;
            justify-content: center;
        }
        
        .description-text {
            color: #374151;
            font-size: 1rem;
            font-weight: 500;
            line-height: 1.4;
            transition: all 0.3s ease;
        }
        
        /* 模式设置组样式 */
        .mode-settings {
            background: linear-gradient(145deg, #ffffff, #f8fafc);
            border-radius: 16px;
            padding: 24px;
            margin-top: 24px;
            border: 1px solid #e2e8f0;
            box-shadow: 0 4px 20px rgba(0,0,0,0.08);
            transition: all 0.3s ease;
            animation: fadeIn 0.5s ease;
        }
        
        .mode-settings:hover {
            transform: translateY(-2px);
            box-shadow: 0 8px 30px rgba(0,0,0,0.12);
        }
        
        .mode-settings h3 {
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
        
        /* 时间范围设置样式 */
        .time-range-config {
            display: flex;
            flex-direction: column;
            gap: 20px;
        }
        
        .time-input-group {
            display: flex;
            align-items: center;
            gap: 16px;
        }
        
        .time-input-group label {
            font-weight: 500;
            color: #374151;
            min-width: 80px;
            display: flex;
            align-items: center;
            gap: 8px;
        }
        
        .time-inputs {
            display: flex;
            align-items: center;
            gap: 8px;
        }
        
        .time-input {
            width: 60px;
            padding: 12px 16px;
            border: 2px solid #e5e7eb;
            border-radius: 12px;
            text-align: center;
            font-size: 1rem;
            font-weight: 500;
            background: white;
            transition: all 0.3s ease;
        }
        
        .time-input:focus {
            outline: none;
            border-color: #3b82f6;
            box-shadow: 0 0 0 3px rgba(59, 130, 246, 0.1);
            background-color: #fefefe;
        }
        
        .time-separator {
            font-size: 1.2rem;
            font-weight: 600;
            color: #4a5568;
        }
        
        /* 时间选择器样式 */
        .time-picker {
            padding: 12px 16px;
            border: 2px solid #e5e7eb;
            border-radius: 12px;
            font-size: 1rem;
            font-weight: 500;
            background: white;
            transition: all 0.3s ease;
            min-width: 140px;
            cursor: pointer;
            color: #374151;
        }
        
        .time-picker:focus {
            outline: none;
            border-color: #3b82f6;
            box-shadow: 0 0 0 3px rgba(59, 130, 246, 0.1);
            background-color: #fefefe;
        }
        
        .time-picker:hover {
            border-color: #cbd5e1;
            background-color: #f8fafc;
        }
        
        /* 自定义时间选择器的样式 */
        .time-picker::-webkit-calendar-picker-indicator {
            cursor: pointer;
            border-radius: 4px;
            margin-left: 8px;
            opacity: 0.7;
            transition: opacity 0.3s ease;
        }
        
        .time-picker::-webkit-calendar-picker-indicator:hover {
            opacity: 1;
        }
        
        /* 延时设置样式 */
        .timeout-config {
            display: flex;
            flex-direction: column;
            gap: 20px;
        }
        
        .timeout-input-group {
            display: flex;
            flex-direction: column;
            gap: 16px;
        }
        
        .timeout-input-group label {
            font-weight: 500;
            color: #374151;
            display: flex;
            align-items: center;
            gap: 8px;
        }
        
        .timeout-slider {
            display: flex;
            align-items: center;
            gap: 16px;
        }
        
        .slider {
            flex: 1;
            height: 6px;
            border-radius: 3px;
            background: #e2e8f0;
            outline: none;
            -webkit-appearance: none;
        }
        
        .slider::-webkit-slider-thumb {
            -webkit-appearance: none;
            appearance: none;
            width: 24px;
            height: 24px;
            border-radius: 50%;
            background: linear-gradient(135deg, #667eea, #764ba2);
            cursor: pointer;
            box-shadow: 0 2px 8px rgba(102, 126, 234, 0.3);
            transition: all 0.3s ease;
        }
        
        .slider::-webkit-slider-thumb:hover {
            transform: scale(1.2);
            box-shadow: 0 4px 15px rgba(102, 126, 234, 0.4);
        }
        
        .slider::-moz-range-thumb {
            width: 24px;
            height: 24px;
            border-radius: 50%;
            background: linear-gradient(135deg, #667eea, #764ba2);
            cursor: pointer;
            border: none;
            box-shadow: 0 2px 8px rgba(102, 126, 234, 0.3);
        }
        
        .timeout-value {
            font-size: 1.1rem;
            font-weight: 600;
            color: #4a5568;
            min-width: 80px;
            text-align: center;
        }
        
        .timeout-input {
            width: 100px;
            padding: 12px 16px;
            border: 2px solid #e5e7eb;
            border-radius: 12px;
            text-align: center;
            font-size: 1rem;
            font-weight: 500;
            background: white;
            transition: all 0.3s ease;
        }
        
        .timeout-input:focus {
            outline: none;
            border-color: #3b82f6;
            box-shadow: 0 0 0 3px rgba(59, 130, 246, 0.1);
            background-color: #fefefe;
        }
        
        /* 按钮样式 - 与时间设置页面保持一致 */
        .settings-actions {
            display: flex;
            gap: 16px;
            justify-content: center;
            margin-top: 32px;
        }
        
        .save-btn, .reset-btn {
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
        
        .save-btn::before, .reset-btn::before {
            content: '';
            position: absolute;
            top: 0;
            left: -100%;
            width: 100%;
            height: 100%;
            background: linear-gradient(90deg, transparent, rgba(255,255,255,0.2), transparent);
            transition: left 0.5s;
        }
        
        .save-btn:hover::before, .reset-btn:hover::before {
            left: 100%;
        }
        
        .save-btn {
            background: linear-gradient(135deg, #10b981, #059669);
            color: white;
            box-shadow: 0 4px 15px rgba(16, 185, 129, 0.3);
        }
        
        .save-btn:hover:not(:disabled) {
            transform: translateY(-2px);
            box-shadow: 0 8px 25px rgba(16, 185, 129, 0.4);
        }
        
        .save-btn:disabled {
            background: linear-gradient(135deg, #9ca3af, #6b7280);
            cursor: not-allowed;
            transform: none;
            box-shadow: none;
        }
        
        .reset-btn {
            background: linear-gradient(135deg, #f59e0b, #d97706);
            color: white;
            box-shadow: 0 4px 15px rgba(245, 158, 11, 0.3);
        }
        
        .reset-btn:hover {
            transform: translateY(-2px);
            box-shadow: 0 8px 25px rgba(245, 158, 11, 0.4);
        }
        
        .btn-loading {
            display: flex;
            align-items: center;
            gap: 8px;
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
        
        .hidden {
            display: none !important;
        }
        
        /* Toast消息样式 */
        .toast {
            position: fixed;
            top: 20px;
            right: 20px;
            background: #1f2937;
            color: white;
            padding: 16px 24px;
            border-radius: 12px;
            font-weight: 500;
            z-index: 1000;
            transform: translateX(100%);
            transition: all 0.3s ease;
        }
        
        .toast.show {
            transform: translateX(0);
        }
        
        .toast.info {
            background: #3b82f6;
        }
        
        .toast.success {
            background: #10b981;
        }
        
        .toast.error {
            background: #ef4444;
        }
        
        .toast.warning {
            background: #f59e0b;
        }
        
        /* 动画效果 */
        @keyframes fadeIn {
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
                transform: translateY(20px);
            }
            to {
                opacity: 1;
                transform: translateY(0);
            }
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
            
            .mode-slider {
                padding: 6px;
            }
            
            .slider-track {
                height: 50px;
            }
            
            .option-label {
                font-size: 0.8rem;
            }
            
            .time-input-group {
                flex-direction: column;
                align-items: stretch;
                gap: 8px;
            }
            
            .timeout-slider {
                flex-direction: column;
                gap: 12px;
            }
            
            .settings-actions {
                flex-direction: column;
                gap: 12px;
            }
            
            .save-btn, .reset-btn {
                width: 100%;
                justify-content: center;
            }
            
            .screen-mode-config {
                padding: 20px;
            }
            
            .mode-settings {
                padding: 20px;
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
            
            .screen-mode-config {
                padding: 16px;
            }
            
            .mode-settings {
                padding: 16px;
            }
        }
    )";
}

String WebServerManager::getScreenSettingsJavaScript() {
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
    js += "    initScreenSettings();\n";
    js += "    loadScreenSettings();\n";
    js += "    setupEventListeners();\n";
    js += "});\n\n";
    
    js += "function initScreenSettings() {\n";
    js += "    console.log('初始化屏幕设置页面');\n";
    js += "    selectMode(0);\n";
    js += "}\n\n";
    
    js += "function setupEventListeners() {\n";
    js += "    const sliderOptions = document.querySelectorAll('.slider-option');\n";
    js += "    sliderOptions.forEach(option => {\n";
    js += "        option.addEventListener('click', function() {\n";
    js += "            const mode = parseInt(this.getAttribute('data-mode'));\n";
    js += "            selectMode(mode);\n";
    js += "        });\n";
    js += "    });\n\n";
    
    js += "    const timeoutSlider = document.getElementById('timeoutSlider');\n";
    js += "    const timeoutInput = document.getElementById('timeoutMinutes');\n";
    js += "    const timeoutValue = document.getElementById('timeoutValue');\n";
    js += "    if (timeoutSlider && timeoutInput && timeoutValue) {\n";
    js += "        timeoutSlider.addEventListener('input', function() {\n";
    js += "            timeoutValue.textContent = this.value;\n";
    js += "            timeoutInput.value = this.value;\n";
    js += "        });\n";
    js += "        timeoutInput.addEventListener('input', function() {\n";
    js += "            if (this.value >= 1 && this.value <= 1440) {\n";
    js += "                timeoutSlider.value = Math.min(this.value, 60);\n";
    js += "                timeoutValue.textContent = timeoutSlider.value;\n";
    js += "            }\n";
    js += "        });\n";
    js += "    }\n\n";
    
    js += "    const startTimePicker = document.getElementById('startTime');\n";
    js += "    const endTimePicker = document.getElementById('endTime');\n";
    js += "    if (startTimePicker && endTimePicker) {\n";
    js += "        startTimePicker.addEventListener('change', function() {\n";
    js += "            updateTimeFromPicker('start', this.value);\n";
    js += "        });\n";
    js += "        endTimePicker.addEventListener('change', function() {\n";
    js += "            updateTimeFromPicker('end', this.value);\n";
    js += "        });\n";
    js += "    }\n";
    js += "}\n\n";
    
    js += "function updateTimeFromPicker(type, timeValue) {\n";
    js += "    const [hours, minutes] = timeValue.split(':').map(Number);\n";
    js += "    const hourElement = document.getElementById(type + 'Hour');\n";
    js += "    const minuteElement = document.getElementById(type + 'Minute');\n";
    js += "    if (hourElement && minuteElement) {\n";
    js += "        hourElement.value = hours;\n";
    js += "        minuteElement.value = minutes;\n";
    js += "    }\n";
    js += "}\n\n";
    
    js += "function updatePickerFromTime(type, hours, minutes) {\n";
    js += "    const timeValue = String(hours).padStart(2, '0') + ':' + String(minutes).padStart(2, '0');\n";
    js += "    const pickerElement = document.getElementById(type + 'Time');\n";
    js += "    if (pickerElement) pickerElement.value = timeValue;\n";
    js += "}\n\n";
    
    js += "function selectMode(mode) {\n";
    js += "    const selectedModeInput = document.getElementById('selectedMode');\n";
    js += "    if (selectedModeInput) selectedModeInput.value = mode;\n";
    js += "    const sliderIndicator = document.getElementById('sliderIndicator');\n";
    js += "    if (sliderIndicator) sliderIndicator.style.left = (mode * 25) + '%';\n";
    js += "    const allOptions = document.querySelectorAll('.slider-option');\n";
    js += "    allOptions.forEach((option, index) => {\n";
    js += "        if (index === mode) option.classList.add('active');\n";
    js += "        else option.classList.remove('active');\n";
    js += "    });\n";
    js += "    const descriptions = ['始终保持屏幕点亮', '指定时间段内点亮屏幕', '无操作后延时熄灭屏幕', '始终关闭屏幕'];\n";
    js += "    const descriptionElement = document.querySelector('.description-text');\n";
    js += "    if (descriptionElement && descriptions[mode]) descriptionElement.textContent = descriptions[mode];\n";
    js += "    handleModeChange(mode.toString());\n";
    js += "}\n\n";
    
    js += "function handleModeChange(mode) {\n";
    js += "    const scheduledSettings = document.getElementById('scheduledSettings');\n";
    js += "    const timeoutSettings = document.getElementById('timeoutSettings');\n";
    js += "    if (scheduledSettings) scheduledSettings.style.display = 'none';\n";
    js += "    if (timeoutSettings) timeoutSettings.style.display = 'none';\n";
    js += "    if (mode === '1' && scheduledSettings) scheduledSettings.style.display = 'block';\n";
    js += "    else if (mode === '2' && timeoutSettings) timeoutSettings.style.display = 'block';\n";
    js += "}\n\n";
    
    js += "function loadScreenSettings() {\n";
    js += "    fetch('/api/screen/settings')\n";
    js += "        .then(response => response.json())\n";
    js += "        .then(data => {\n";
    js += "            if (data.success) {\n";
    js += "                console.log('加载屏幕设置:', data);\n";
    js += "                selectMode(data.mode || 0);\n";
    js += "                const startHour = data.startHour || 8;\n";
    js += "                const startMinute = data.startMinute || 0;\n";
    js += "                const endHour = data.endHour || 22;\n";
    js += "                const endMinute = data.endMinute || 0;\n";
    js += "                const timeoutMinutes = data.timeoutMinutes || 10;\n";
    js += "                const startHourElement = document.getElementById('startHour');\n";
    js += "                const startMinuteElement = document.getElementById('startMinute');\n";
    js += "                const endHourElement = document.getElementById('endHour');\n";
    js += "                const endMinuteElement = document.getElementById('endMinute');\n";
    js += "                const timeoutInput = document.getElementById('timeoutMinutes');\n";
    js += "                const timeoutSlider = document.getElementById('timeoutSlider');\n";
    js += "                const timeoutValue = document.getElementById('timeoutValue');\n";
    js += "                if (startHourElement && startMinuteElement && endHourElement && endMinuteElement) {\n";
    js += "                    startHourElement.value = startHour;\n";
    js += "                    startMinuteElement.value = startMinute;\n";
    js += "                    endHourElement.value = endHour;\n";
    js += "                    endMinuteElement.value = endMinute;\n";
    js += "                    updatePickerFromTime('start', startHour, startMinute);\n";
    js += "                    updatePickerFromTime('end', endHour, endMinute);\n";
    js += "                }\n";
    js += "                if (timeoutInput && timeoutSlider && timeoutValue) {\n";
    js += "                    timeoutInput.value = timeoutMinutes;\n";
    js += "                    timeoutSlider.value = Math.min(timeoutMinutes, 60);\n";
    js += "                    timeoutValue.textContent = timeoutSlider.value;\n";
    js += "                }\n";
    js += "                console.log('屏幕设置加载成功');\n";
    js += "            } else {\n";
    js += "                console.error('加载屏幕设置失败:', data.message);\n";
    js += "                showToast('加载配置失败: ' + data.message, 'error');\n";
    js += "            }\n";
    js += "        })\n";
    js += "        .catch(error => showToast('加载配置出错', 'error'));\n";
    js += "}\n\n";
    
    js += "function saveScreenSettings() {\n";
    js += "    const saveBtn = document.querySelector('.save-btn');\n";
    js += "    const btnText = saveBtn.querySelector('.btn-text');\n";
    js += "    const btnLoading = saveBtn.querySelector('.btn-loading');\n";
    js += "    if (btnText && btnLoading) {\n";
    js += "        btnText.style.display = 'none';\n";
    js += "        btnLoading.style.display = 'flex';\n";
    js += "        saveBtn.disabled = true;\n";
    js += "    }\n";
    js += "    const selectedModeInput = document.getElementById('selectedMode');\n";
    js += "    if (!selectedModeInput) {\n";
    js += "        showToast('请选择屏幕模式', 'error');\n";
    js += "        resetSaveButton();\n";
    js += "        return;\n";
    js += "    }\n";
    js += "    const mode = parseInt(selectedModeInput.value);\n";
    js += "    const startHour = parseInt(document.getElementById('startHour')?.value) || 8;\n";
    js += "    const startMinute = parseInt(document.getElementById('startMinute')?.value) || 0;\n";
    js += "    const endHour = parseInt(document.getElementById('endHour')?.value) || 22;\n";
    js += "    const endMinute = parseInt(document.getElementById('endMinute')?.value) || 0;\n";
    js += "    const timeoutMinutes = parseInt(document.getElementById('timeoutMinutes')?.value) || 10;\n";
    js += "    const formData = new FormData();\n";
    js += "    formData.append('mode', mode.toString());\n";
    js += "    formData.append('startHour', startHour.toString());\n";
    js += "    formData.append('startMinute', startMinute.toString());\n";
    js += "    formData.append('endHour', endHour.toString());\n";
    js += "    formData.append('endMinute', endMinute.toString());\n";
    js += "    formData.append('timeoutMinutes', timeoutMinutes.toString());\n";
    js += "    fetch('/api/screen/settings', { method: 'POST', body: formData })\n";
    js += "        .then(response => response.json())\n";
    js += "        .then(data => {\n";
    js += "            if (data.success) showToast('屏幕设置保存成功', 'success');\n";
    js += "            else showToast('保存失败: ' + data.message, 'error');\n";
    js += "        })\n";
    js += "        .catch(error => showToast('保存出错', 'error'))\n";
    js += "        .finally(() => resetSaveButton());\n";
    js += "}\n\n";
    
    js += "function resetSaveButton() {\n";
    js += "    const saveBtn = document.querySelector('.save-btn');\n";
    js += "    const btnText = saveBtn.querySelector('.btn-text');\n";
    js += "    const btnLoading = saveBtn.querySelector('.btn-loading');\n";
    js += "    if (btnText && btnLoading) {\n";
    js += "        btnText.style.display = 'inline';\n";
    js += "        btnLoading.style.display = 'none';\n";
    js += "        saveBtn.disabled = false;\n";
    js += "    }\n";
    js += "}\n\n";
    
    js += "function resetSettings() {\n";
    js += "    if (confirm('确定要重置为默认设置吗？')) {\n";
    js += "        selectMode(0);\n";
    js += "        const startHour = document.getElementById('startHour');\n";
    js += "        const startMinute = document.getElementById('startMinute');\n";
    js += "        const endHour = document.getElementById('endHour');\n";
    js += "        const endMinute = document.getElementById('endMinute');\n";
    js += "        const timeoutMinutes = document.getElementById('timeoutMinutes');\n";
    js += "        const timeoutSlider = document.getElementById('timeoutSlider');\n";
    js += "        const timeoutValue = document.getElementById('timeoutValue');\n";
    js += "        if (startHour) startHour.value = 8;\n";
    js += "        if (startMinute) startMinute.value = 0;\n";
    js += "        if (endHour) endHour.value = 22;\n";
    js += "        if (endMinute) endMinute.value = 0;\n";
    js += "        if (timeoutMinutes) timeoutMinutes.value = 10;\n";
    js += "        if (timeoutSlider) timeoutSlider.value = 10;\n";
    js += "        if (timeoutValue) timeoutValue.textContent = 10;\n";
    js += "        updatePickerFromTime('start', 8, 0);\n";
    js += "        updatePickerFromTime('end', 22, 0);\n";
    js += "        const scheduledSettings = document.getElementById('scheduledSettings');\n";
    js += "        const timeoutSettings = document.getElementById('timeoutSettings');\n";
    js += "        if (scheduledSettings) scheduledSettings.style.display = 'none';\n";
    js += "        if (timeoutSettings) timeoutSettings.style.display = 'none';\n";
    js += "        showToast('已重置为默认设置', 'info');\n";
    js += "    }\n";
    js += "}\n";
    
    return js;
} 