/*
 * WebServerManager_WeatherSettings.cpp - Web服务器天气设置页面实现
 * ESP32S3监控项目 - 天气设置页面和功能模块
 */

#include "WebServerManager.h"

String WebServerManager::getWeatherSettingsHTML() {
    String html = "<!DOCTYPE html>\n";
    html += "<html lang=\"zh-CN\">\n";
    html += "<head>\n";
    html += "    <meta charset=\"UTF-8\">\n";
    html += "    <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\n";
    html += "    <title>天气设置 - ESP32S3 Monitor</title>\n";
    html += "    <style>\n";
    html += getCSS();
    html += getWeatherSettingsCSS();
    html += "    </style>\n";
    html += "</head>\n";
    html += "<body>\n";
    html += "    <div class=\"container\">\n";
    html += "        <header class=\"header\">\n";
    html += "            <h1>小屏幕配置</h1>\n";
    html += "            <div class=\"subtitle\">天气设置</div>\n";
    html += "        </header>\n";
    html += "        \n";
    html += "        <div class=\"card\">\n";
    html += "            <button onclick=\"window.location.href='/'\" class=\"back-home-btn\">\n";
    html += "                返回首页\n";
    html += "            </button>\n";
    html += "            \n";
    html += "            <!-- 当前天气显示 -->\n";
    html += "            <div class=\"settings-section\">\n";
    html += "                <h2>当前天气</h2>\n";
    html += "                <div class=\"weather-display\">\n";
    html += "                    <div class=\"weather-card\">\n";
    html += "                        <div class=\"weather-info\" id=\"currentWeatherInfo\">\n";
    html += "                            <div class=\"weather-loading\">\n";
    html += "                                <div class=\"spinner\"></div>\n";
    html += "                                <span>加载天气信息中...</span>\n";
    html += "                            </div>\n";
    html += "                        </div>\n";
    html += "                    </div>\n";
    html += "                </div>\n";
    html += "            </div>\n";
    html += "            \n";
    html += "            <!-- 天气配置 -->\n";
    html += "            <div class=\"settings-section\">\n";
    html += "                <h2>天气配置</h2>\n";
    html += "                <div class=\"weather-config\">\n";
    html += "                    <div class=\"setting-group\">\n";
    html += "                        <h3>API配置</h3>\n";
    html += "                        <div class=\"api-config\">\n";
    html += "                            <p class=\"setting-description\">\n";
    html += "                                配置高德地图天气API密钥，获取实时天气数据。\n";
    html += "                                <a href=\"https://lbs.amap.com/api/webservice/guide/api/weatherinfo\" target=\"_blank\">\n";
    html += "                                    获取API密钥 →\n";
    html += "                                </a>\n";
    html += "                            </p>\n";
    html += "                            <div class=\"form-group\">\n";
    html += "                                <label for=\"apiKey\">API密钥:</label>\n";
    html += "                                <input type=\"text\" id=\"apiKey\" class=\"setting-input\" placeholder=\"请输入高德地图API密钥\">\n";
    html += "                                <button onclick=\"saveApiKey()\" class=\"setting-btn primary-btn\">\n";
    html += "                                    保存API密钥\n";
    html += "                                </button>\n";
    html += "                            </div>\n";
    html += "                        </div>\n";
    html += "                    </div>\n";
    html += "                    \n";
    html += "                    <div class=\"setting-group\">\n";
    html += "                        <h3>定位信息</h3>\n";
    html += "                        <div class=\"location-display\">\n";
    html += "                            <p class=\"setting-description\">\n";
    html += "                                显示当前定位的省市信息和对应的城市编码，用于获取天气数据。\n";
    html += "                            </p>\n";
    html += "                            <div class=\"current-location-info\">\n";
    html += "                                <div class=\"location-item\">\n";
    html += "                                    <span class=\"location-label\">当前位置:</span>\n";
    html += "                                    <span id=\"currentLocationText\" class=\"location-text\">获取中...</span>\n";
    html += "                                </div>\n";
    html += "                                <div class=\"location-item\">\n";
    html += "                                    <span class=\"location-label\">城市编码:</span>\n";
    html += "                                    <span id=\"currentCityCode\" class=\"location-code\">-</span>\n";
    html += "                                </div>\n";
    html += "                            </div>\n";
    html += "                        </div>\n";
    html += "                    </div>\n";
    html += "                    \n";
    html += "                    <div class=\"setting-group\">\n";
    html += "                        <h3>更新设置</h3>\n";
    html += "                        <div class=\"update-config\">\n";
    html += "                            <p class=\"setting-description\">\n";
    html += "                                配置天气数据自动更新的相关设置。\n";
    html += "                            </p>\n";
    html += "                            <div class=\"form-group\">\n";
    html += "                                <label class=\"switch-label\">\n";
    html += "                                    <input type=\"checkbox\" id=\"autoUpdate\" class=\"switch-input\">\n";
    html += "                                    <span class=\"switch-slider\"></span>\n";
    html += "                                    自动更新天气数据\n";
    html += "                                </label>\n";
    html += "                            </div>\n";
    html += "                            <div class=\"form-group\">\n";
    html += "                                <label for=\"updateInterval\">更新间隔 (分钟):</label>\n";
    html += "                                <input type=\"number\" id=\"updateInterval\" class=\"setting-input\" min=\"5\" max=\"1440\" value=\"30\">\n";
    html += "                            </div>\n";
    html += "                            <button onclick=\"saveUpdateConfig()\" class=\"setting-btn warning-btn\">\n";
    html += "                                保存更新设置\n";
    html += "                            </button>\n";
    html += "                        </div>\n";
    html += "                    </div>\n";
    html += "                    \n";
    html += "                    <div class=\"setting-group\">\n";
    html += "                        <h3>操作功能</h3>\n";
    html += "                        <div class=\"weather-actions\">\n";
    html += "                            <button onclick=\"testWeatherApi()\" class=\"setting-btn primary-btn\">\n";
    html += "                                <span class=\"btn-text\">测试天气API</span>\n";
    html += "                                <div class=\"btn-loading hidden\">\n";
    html += "                                    <div class=\"spinner-sm\"></div>\n";
    html += "                                    <span>测试中...</span>\n";
    html += "                                </div>\n";
    html += "                            </button>\n";
    html += "                            <button onclick=\"updateWeatherNow()\" class=\"setting-btn success-btn\">\n";
    html += "                                <span class=\"btn-text\">立即更新天气</span>\n";
    html += "                                <div class=\"btn-loading hidden\">\n";
    html += "                                    <div class=\"spinner-sm\"></div>\n";
    html += "                                    <span>更新中...</span>\n";
    html += "                                </div>\n";
    html += "                            </button>\n";
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
    html += getWeatherSettingsJavaScript();
    html += "    </script>\n";
    html += "</body>\n";
    html += "</html>\n";
    
    return html;
}

String WebServerManager::getWeatherSettingsJavaScript() {
    return R"(
// 基础工具函数
function showToast(message, type) {
    const toast = document.getElementById('toast');
    const toastMessage = document.getElementById('toastMessage');
    toastMessage.textContent = message;
    toast.className = 'toast show ' + (type || 'info');
    setTimeout(() => { toast.className = 'toast hidden'; }, 3000);
}

let weatherUpdateInterval;

document.addEventListener('DOMContentLoaded', function() {
    initWeatherSettings();
    loadWeatherConfig();
    loadCurrentWeather();
    startWeatherUpdate();
});

function initWeatherSettings() {
    // 初始化页面
    console.log('初始化天气设置页面');
}


async function loadWeatherConfig() {
    try {
        const response = await fetch('/api/weather/config');
        const data = await response.json();
        if (data.success) {
            updateConfigDisplay(data.config);
        } else {
            showToast('加载天气配置失败', 'error');
        }
    } catch (error) {
        console.error('加载天气配置失败:', error);
        showToast('连接服务器失败', 'error');
    }
}

function updateConfigDisplay(config) {
    document.getElementById('apiKey').value = config.apiKey || '';
    document.getElementById('autoUpdate').checked = config.autoUpdate || false;
    document.getElementById('updateInterval').value = config.updateInterval || 30;
    // enableForecast 开关已从界面移除，但保留配置处理以兼容后端
    
    // 更新定位信息显示
    if (config.cityCode && config.cityName) {
        const locationText = config.fullLocation || config.cityName;
        document.getElementById('currentLocationText').textContent = locationText;
        document.getElementById('currentCityCode').textContent = config.cityCode;
    } else {
        document.getElementById('currentLocationText').textContent = '未配置';
        document.getElementById('currentCityCode').textContent = '-';
    }
}



async function loadCurrentWeather() {
    try {
        const response = await fetch('/api/weather/current');
        const data = await response.json();
        if (data.success && data.weather.isValid) {
            updateWeatherDisplay(data.weather);
        } else {
            showWeatherError(data.message || '暂无天气数据');
        }
    } catch (error) {
        console.error('加载天气数据失败:', error);
        showWeatherError('获取天气数据失败');
    }
}

function updateWeatherDisplay(weather) {
    const weatherInfo = document.getElementById('currentWeatherInfo');
    weatherInfo.innerHTML = `
        <div class="weather-main">
            <h3>${weather.city} - ${weather.weather}</h3>
            <div class="temperature">${weather.temperature}°C</div>
        </div>
        <div class="weather-details">
            <div class="weather-item">
                <div class="label">湿度</div>
                <div class="value">${weather.humidity}%</div>
            </div>
            <div class="weather-item">
                <div class="label">风向</div>
                <div class="value">${weather.winddirection}</div>
            </div>
            <div class="weather-item">
                <div class="label">风力</div>
                <div class="value">${weather.windpower}级</div>
            </div>
            <div class="weather-item">
                <div class="label">更新时间</div>
                <div class="value">${weather.reporttime}</div>
            </div>
        </div>
    `;
}

function showWeatherError(message) {
    const weatherInfo = document.getElementById('currentWeatherInfo');
    weatherInfo.innerHTML = `<div class="weather-error">${message}</div>`;
}

function startWeatherUpdate() {
    weatherUpdateInterval = setInterval(() => {
        loadCurrentWeather();
    }, 60000); // 每分钟更新一次
}

async function saveApiKey() {
    const apiKey = document.getElementById('apiKey').value.trim();
    if (!apiKey) {
        showToast('请输入API密钥', 'warning');
        return;
    }
    
    try {
        const formData = new FormData();
        formData.append('apiKey', apiKey);
        
        const response = await fetch('/api/weather/api-key', {
            method: 'POST',
            body: formData
        });
        
        const data = await response.json();
        if (data.success) {
            showToast('API密钥保存成功', 'success');
        } else {
            showToast('API密钥保存失败: ' + data.message, 'error');
        }
    } catch (error) {
        showToast('保存API密钥失败', 'error');
    }
}



async function saveUpdateConfig() {
    const autoUpdate = document.getElementById('autoUpdate').checked;
    const updateInterval = parseInt(document.getElementById('updateInterval').value);
    
    if (updateInterval < 5 || updateInterval > 1440) {
        showToast('更新间隔必须在5-1440分钟之间', 'warning');
        return;
    }
    
    try {
        const formData = new FormData();
        formData.append('autoUpdate', autoUpdate);
        formData.append('updateInterval', updateInterval);
        formData.append('enableForecast', false); // 默认关闭天气预报
        
        const response = await fetch('/api/weather/update-config', {
            method: 'POST',
            body: formData
        });
        
        const data = await response.json();
        if (data.success) {
            showToast('更新设置保存成功', 'success');
        } else {
            showToast('更新设置保存失败: ' + data.message, 'error');
        }
    } catch (error) {
        showToast('保存更新设置失败', 'error');
    }
}

async function testWeatherApi() {
    const btn = event.target.closest('button');
    const btnText = btn.querySelector('.btn-text');
    const btnLoading = btn.querySelector('.btn-loading');
    
    btn.disabled = true;
    btnText.style.display = 'none';
    btnLoading.classList.remove('hidden');
    
    try {
        const response = await fetch('/api/weather/test', { method: 'POST' });
        const data = await response.json();
        
        if (data.success) {
            showToast('天气API测试成功', 'success');
            setTimeout(loadCurrentWeather, 1000);
        } else {
            showToast('天气API测试失败: ' + data.message, 'error');
        }
    } catch (error) {
        showToast('天气API测试失败', 'error');
    } finally {
        btn.disabled = false;
        btnText.style.display = '';
        btnLoading.classList.add('hidden');
    }
}

async function updateWeatherNow() {
    const btn = event.target.closest('button');
    const btnText = btn.querySelector('.btn-text');
    const btnLoading = btn.querySelector('.btn-loading');
    
    btn.disabled = true;
    btnText.style.display = 'none';
    btnLoading.classList.remove('hidden');
    
    try {
        const response = await fetch('/api/weather/update', { method: 'POST' });
        const data = await response.json();
        
        if (data.success) {
            showToast('天气数据更新成功', 'success');
            setTimeout(() => {
                loadCurrentWeather();
            }, 1000);
        } else {
            showToast('天气数据更新失败: ' + data.message, 'error');
        }
    } catch (error) {
        showToast('天气数据更新失败', 'error');
    } finally {
        btn.disabled = false;
        btnText.style.display = '';
        btnLoading.classList.add('hidden');
    }
}

// 页面卸载时清理定时器
window.addEventListener('beforeunload', function() {
    if (weatherUpdateInterval) clearInterval(weatherUpdateInterval);
});
)";
}

String WebServerManager::getWeatherSettingsCSS() {
    return R"(
        /* 天气设置页面专用样式 */
        .weather-display {
            margin-bottom: 24px;
        }
        
        .weather-card {
            background: linear-gradient(135deg, #0891b2 0%, #0e7490 50%, #164e63 100%);
            border-radius: 24px;
            padding: 32px;
            color: white;
            text-align: center;
            box-shadow: 
                0 20px 60px rgba(8, 145, 178, 0.4),
                0 8px 20px rgba(8, 145, 178, 0.2),
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
        
        .weather-card::before {
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
        
        .weather-card::after {
            content: '';
            position: absolute;
            bottom: 0;
            left: 0;
            right: 0;
            height: 1px;
            background: linear-gradient(90deg, transparent, rgba(255,255,255,0.3), transparent);
        }
        
        .weather-info {
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
        
        .weather-main {
            margin-bottom: 24px;
            position: relative;
            display: flex;
            flex-direction: column;
            align-items: center;
            justify-content: center;
            text-align: center;
            width: 100%;
        }
        
        .weather-main h3 {
            font-size: 1.6rem;
            font-weight: 700;
            margin-bottom: 16px;
            text-shadow: 0 2px 8px rgba(0, 0, 0, 0.3);
            letter-spacing: 0.5px;
            text-align: center;
        }
        
        .temperature {
            font-size: 4rem;
            font-weight: 900;
            margin: 16px 0;
            text-shadow: 0 4px 16px rgba(0, 0, 0, 0.4);
            background: linear-gradient(135deg, #ffffff, #f0f9ff);
            background-clip: text;
            -webkit-background-clip: text;
            -webkit-text-fill-color: transparent;
            filter: drop-shadow(0 2px 4px rgba(0, 0, 0, 0.3));
            line-height: 1;
            animation: temperatureGlow 3s ease-in-out infinite;
            text-align: center;
        }
        
        @keyframes temperatureGlow {
            0%, 100% { 
                filter: drop-shadow(0 2px 4px rgba(0, 0, 0, 0.3));
                transform: scale(1);
            }
            50% { 
                filter: drop-shadow(0 4px 12px rgba(255, 255, 255, 0.4));
                transform: scale(1.02);
            }
        }
        
        .weather-loading {
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
        
        .weather-loading .spinner {
            width: 32px;
            height: 32px;
            border: 3px solid rgba(255, 255, 255, 0.3);
            border-radius: 50%;
            border-top-color: #ffffff;
            animation: weatherSpin 1.2s ease-in-out infinite;
        }
        
        .weather-error {
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
        
        @keyframes weatherSpin {
            to { transform: rotate(360deg); }
        }
        
        @keyframes shine {
            0% { 
                transform: translateX(-100%) translateY(-100%) rotate(45deg);
                opacity: 0;
            }
            50% {
                opacity: 1;
            }
            100% { 
                transform: translateX(100%) translateY(100%) rotate(45deg);
                opacity: 0;
            }
        }
        
        /* 响应式优化 */
        @media (max-width: 640px) {
            .weather-card {
                padding: 24px 20px;
                border-radius: 20px;
            }
            
            .temperature {
                font-size: 3.2rem;
                margin: 12px 0;
                text-align: center;
            }
            
            .weather-main h3 {
                font-size: 1.4rem;
                margin-bottom: 12px;
                text-align: center;
            }
            
            .weather-details {
                gap: 16px;
                margin-top: 20px;
                justify-content: center;
            }
            
            .weather-item {
                padding: 16px 12px;
                text-align: center;
            }
            
            .weather-item .value {
                font-size: 1.1rem;
                text-align: center;
            }
        }
        
        @media (max-width: 480px) {
            .weather-details {
                grid-template-columns: 1fr;
                gap: 12px;
                justify-content: center;
            }
            
            .temperature {
                font-size: 2.8rem;
                text-align: center;
            }
        }
        
        .weather-details {
            display: grid;
            grid-template-columns: repeat(2, 1fr);
            gap: 20px;
            margin-top: 24px;
            position: relative;
            z-index: 2;
            width: 100%;
            justify-content: center;
            align-items: center;
        }
        
        .weather-item {
            background: rgba(255, 255, 255, 0.15);
            border-radius: 16px;
            padding: 18px 16px;
            text-align: center;
            border: 1px solid rgba(255, 255, 255, 0.2);
            backdrop-filter: blur(8px);
            transition: all 0.3s cubic-bezier(0.4, 0, 0.2, 1);
            position: relative;
            overflow: hidden;
            display: flex;
            flex-direction: column;
            align-items: center;
            justify-content: center;
        }
        
        .weather-item::before {
            content: '';
            position: absolute;
            top: 0;
            left: -100%;
            width: 100%;
            height: 100%;
            background: linear-gradient(90deg, transparent, rgba(255,255,255,0.1), transparent);
            transition: left 0.5s;
        }
        
        .weather-item:hover::before {
            left: 100%;
        }
        
        .weather-item:hover {
            background: rgba(255, 255, 255, 0.25);
            transform: translateY(-2px);
            box-shadow: 0 8px 20px rgba(255, 255, 255, 0.1);
        }
        
        .weather-item .label {
            font-size: 0.85rem;
            opacity: 0.9;
            margin-bottom: 8px;
            font-weight: 500;
            text-transform: uppercase;
            letter-spacing: 0.5px;
            color: rgba(255, 255, 255, 0.8);
        }
        
        .weather-item .value {
            font-size: 1.3rem;
            font-weight: 700;
            color: #ffffff;
            text-shadow: 0 2px 4px rgba(0, 0, 0, 0.3);
            line-height: 1.2;
        }
        
        /* 天气配置区域 */
        .weather-config {
            display: grid;
            gap: 20px;
        }
        
        /* 输入框样式 */
        .setting-input {
            width: 100%;
            padding: 12px 16px;
            border: 2px solid #e5e7eb;
            border-radius: 12px;
            font-size: 1rem;
            background: white;
            transition: all 0.3s ease;
            margin-bottom: 16px;
        }
        
        .setting-input:focus {
            outline: none;
            border-color: #14b8a6;
            box-shadow: 0 0 0 3px rgba(20, 184, 166, 0.1);
            background-color: #fefefe;
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
            border-radius: 50%;
            transition: all 0.3s ease;
            box-shadow: 0 2px 4px rgba(0,0,0,0.2);
        }
        
        .switch-input:checked + .switch-slider {
            background: #14b8a6;
        }
        
        .switch-input:checked + .switch-slider::before {
            transform: translateX(26px);
        }
        
        /* 天气操作按钮 */
        .weather-actions {
            display: flex;
            gap: 16px;
            flex-wrap: wrap;
        }
        
        .weather-actions .setting-btn {
            flex: 1;
            min-width: 200px;
        }
        

        
        /* API配置特殊样式 */
        .api-config .setting-description a {
            color: #14b8a6;
            text-decoration: none;
            font-weight: 600;
            transition: color 0.3s ease;
        }
        
        .api-config .setting-description a:hover {
            color: #0d9488;
            text-decoration: underline;
        }
        
        /* 定位信息显示样式 */
        .location-display {
            background: #f8fafc;
            border-radius: 16px;
            padding: 24px;
            margin-bottom: 20px;
            border: 1px solid #e2e8f0;
        }
        
        .current-location-info {
            background: white;
            border-radius: 12px;
            padding: 20px;
            border: 1px solid #e2e8f0;
            box-shadow: 0 2px 8px rgba(0, 0, 0, 0.04);
        }
        
        .location-item {
            display: flex;
            align-items: center;
            gap: 12px;
            margin-bottom: 12px;
            padding: 12px 0;
            border-bottom: 1px solid #f1f5f9;
        }
        
        .location-item:last-child {
            border-bottom: none;
            margin-bottom: 0;
        }
        
        .location-label {
            font-weight: 600;
            color: #374151;
            min-width: 80px;
        }
        
        .location-text {
            color: #14b8a6;
            font-weight: 600;
            font-size: 1.1rem;
        }
        
        .location-code {
            color: #f59e0b;
            font-weight: 700;
            font-family: 'Courier New', monospace;
            background: #fef3c7;
            padding: 4px 8px;
            border-radius: 6px;
            font-size: 0.95rem;
        }
        
        /* 响应式设计 */
        @media (max-width: 768px) {
            .weather-details {
                grid-template-columns: repeat(2, 1fr);
            }
            
            .weather-actions {
                flex-direction: column;
            }
            
            .weather-actions .setting-btn {
                min-width: auto;
            }
        }
        
        @media (max-width: 480px) {
            .weather-details {
                grid-template-columns: 1fr;
            }
        }
    )";
} 