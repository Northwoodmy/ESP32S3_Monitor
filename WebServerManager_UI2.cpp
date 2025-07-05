/*
 * WebServerManager_UI2.cpp - Web服务器UI设置页面实现
 * ESP32S3监控项目 - 设置页面和扩展功能模块
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
    html += "                        <h3>城市设置</h3>\n";
    html += "                        <div class=\"city-config\">\n";
    html += "                            <p class=\"setting-description\">\n";
    html += "                                选择您的省市区，系统将自动生成对应的城市编码获取天气信息。\n";
    html += "                            </p>\n";
    html += "                            <div class=\"location-selector\">\n";
    html += "                                <div class=\"form-row\">\n";
    html += "                                    <div class=\"form-group\">\n";
    html += "                                        <label for=\"provinceSelect\">省份:</label>\n";
    html += "                                        <select id=\"provinceSelect\" class=\"location-select\" onchange=\"onProvinceChange()\">\n";
    html += "                                            <option value=\"\">请选择省份</option>\n";
    html += "                                        </select>\n";
    html += "                                    </div>\n";
    html += "                                    <div class=\"form-group\">\n";
    html += "                                        <label for=\"citySelect\">城市:</label>\n";
    html += "                                        <select id=\"citySelect\" class=\"location-select\" onchange=\"onCityChange()\" disabled>\n";
    html += "                                            <option value=\"\">请先选择省份</option>\n";
    html += "                                        </select>\n";
    html += "                                    </div>\n";
    html += "                                    <div class=\"form-group\">\n";
    html += "                                        <label for=\"districtSelect\">区县:</label>\n";
    html += "                                        <select id=\"districtSelect\" class=\"location-select\" onchange=\"onDistrictChange()\" disabled>\n";
    html += "                                            <option value=\"\">请先选择城市</option>\n";
    html += "                                        </select>\n";
    html += "                                    </div>\n";
    html += "                                </div>\n";
    html += "                                <div class=\"selected-location\">\n";
    html += "                                    <div class=\"location-info\">\n";
    html += "                                        <span class=\"location-label\">当前选择:</span>\n";
    html += "                                        <span id=\"selectedLocation\" class=\"location-text\">未选择</span>\n";
    html += "                                    </div>\n";
    html += "                                    <div class=\"location-code\">\n";
    html += "                                        <span class=\"code-label\">城市编码:</span>\n";
    html += "                                        <span id=\"generatedCode\" class=\"code-text\">-</span>\n";
    html += "                                    </div>\n";
    html += "                                </div>\n";
    html += "                            </div>\n";
    html += "                            <button onclick=\"saveCityConfig()\" class=\"setting-btn success-btn\" id=\"saveCityBtn\" disabled>\n";
    html += "                                保存城市设置\n";
    html += "                            </button>\n";
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
    html += "                            <div class=\"form-group\">\n";
    html += "                                <label class=\"switch-label\">\n";
    html += "                                    <input type=\"checkbox\" id=\"enableForecast\" class=\"switch-input\">\n";
    html += "                                    <span class=\"switch-slider\"></span>\n";
    html += "                                    启用天气预报\n";
    html += "                                </label>\n";
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
    html += "            \n";
    html += "            <!-- 天气统计信息 -->\n";
    html += "            <div class=\"settings-section\">\n";
    html += "                <h2>统计信息</h2>\n";
    html += "                <div class=\"weather-stats\">\n";
    html += "                    <div class=\"stat-item\">\n";
    html += "                        <span class=\"label\">总请求次数:</span>\n";
    html += "                        <span class=\"value\" id=\"totalRequests\">-</span>\n";
    html += "                    </div>\n";
    html += "                    <div class=\"stat-item\">\n";
    html += "                        <span class=\"label\">成功请求:</span>\n";
    html += "                        <span class=\"value\" id=\"successRequests\">-</span>\n";
    html += "                    </div>\n";
    html += "                    <div class=\"stat-item\">\n";
    html += "                        <span class=\"label\">失败请求:</span>\n";
    html += "                        <span class=\"value\" id=\"failedRequests\">-</span>\n";
    html += "                    </div>\n";
    html += "                    <div class=\"stat-item\">\n";
    html += "                        <span class=\"label\">上次更新:</span>\n";
    html += "                        <span class=\"value\" id=\"lastUpdate\">-</span>\n";
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
    loadWeatherStats();
    startWeatherUpdate();
    initLocationData();
});

function initWeatherSettings() {
    // 初始化页面
    console.log('初始化天气设置页面');
}

// 省市区数据
const locationData = {
    '北京市': {
        code: '110000',
        cities: {
            '北京市': {
                code: '110100',
                districts: {
                    '东城区': '110101', '西城区': '110102', '朝阳区': '110105',
                    '丰台区': '110106', '石景山区': '110107', '海淀区': '110108',
                    '门头沟区': '110109', '房山区': '110111', '通州区': '110112',
                    '顺义区': '110113', '昌平区': '110114', '大兴区': '110115',
                    '怀柔区': '110116', '平谷区': '110117', '密云区': '110118', '延庆区': '110119'
                }
            }
        }
    },
    '上海市': {
        code: '310000',
        cities: {
            '上海市': {
                code: '310100',
                districts: {
                    '黄浦区': '310101', '徐汇区': '310104', '长宁区': '310105',
                    '静安区': '310106', '普陀区': '310107', '虹口区': '310109',
                    '杨浦区': '310110', '闵行区': '310112', '宝山区': '310113',
                    '嘉定区': '310114', '浦东新区': '310115', '金山区': '310116',
                    '松江区': '310117', '青浦区': '310118', '奉贤区': '310120', '崇明区': '310151'
                }
            }
        }
    },
    '天津市': {
        code: '120000',
        cities: {
            '天津市': {
                code: '120100',
                districts: {
                    '和平区': '120101', '河东区': '120102', '河西区': '120103',
                    '南开区': '120104', '河北区': '120105', '红桥区': '120106',
                    '东丽区': '120110', '西青区': '120111', '津南区': '120112',
                    '北辰区': '120113', '武清区': '120114', '宝坻区': '120115',
                    '滨海新区': '120116', '宁河区': '120117', '静海区': '120118', '蓟州区': '120119'
                }
            }
        }
    },
    '重庆市': {
        code: '500000',
        cities: {
            '重庆市': {
                code: '500100',
                districts: {
                    '万州区': '500101', '涪陵区': '500102', '渝中区': '500103',
                    '大渡口区': '500104', '江北区': '500105', '沙坪坝区': '500106',
                    '九龙坡区': '500107', '南岸区': '500108', '北碚区': '500109',
                    '綦江区': '500110', '大足区': '500111', '渝北区': '500112',
                    '巴南区': '500113', '黔江区': '500114', '长寿区': '500115',
                    '江津区': '500116', '合川区': '500117', '永川区': '500118',
                    '南川区': '500119', '璧山区': '500120', '铜梁区': '500121', '潼南区': '500122'
                }
            }
        }
    },
    '广东省': {
        code: '440000',
        cities: {
            '广州市': {
                code: '440100',
                districts: {
                    '荔湾区': '440103', '越秀区': '440104', '海珠区': '440105',
                    '天河区': '440106', '白云区': '440111', '黄埔区': '440112',
                    '番禺区': '440113', '花都区': '440114', '南沙区': '440115',
                    '从化区': '440117', '增城区': '440118'
                }
            },
            '深圳市': {
                code: '440300',
                districts: {
                    '罗湖区': '440303', '福田区': '440304', '南山区': '440305',
                    '宝安区': '440306', '龙岗区': '440307', '盐田区': '440308',
                    '龙华区': '440309', '坪山区': '440310', '光明区': '440311'
                }
            },
            '珠海市': { code: '440400', districts: {'香洲区': '440402', '斗门区': '440403', '金湾区': '440404'} },
            '汕头市': { code: '440500', districts: {'龙湖区': '440507', '金平区': '440511', '濠江区': '440512', '潮阳区': '440513', '潮南区': '440514', '澄海区': '440515', '南澳县': '440523'} },
            '佛山市': { code: '440600', districts: {'禅城区': '440604', '南海区': '440605', '顺德区': '440606', '三水区': '440607', '高明区': '440608'} },
            '韶关市': { code: '440200', districts: {'武江区': '440203', '浈江区': '440204', '曲江区': '440205'} },
            '湛江市': { code: '440800', districts: {'赤坎区': '440802', '霞山区': '440803', '坡头区': '440804', '麻章区': '440811'} },
            '肇庆市': { code: '441200', districts: {'端州区': '441202', '鼎湖区': '441203', '高要区': '441204'} },
            '江门市': { code: '440700', districts: {'蓬江区': '440703', '江海区': '440704', '新会区': '440705'} },
            '茂名市': { code: '440900', districts: {'茂南区': '440902', '电白区': '440904'} },
            '惠州市': { code: '441300', districts: {'惠城区': '441302', '惠阳区': '441303'} },
            '梅州市': { code: '441400', districts: {'梅江区': '441402', '梅县区': '441403'} },
            '汕尾市': { code: '441500', districts: {'城区': '441502'} },
            '河源市': { code: '441600', districts: {'源城区': '441602'} },
            '阳江市': { code: '441700', districts: {'江城区': '441702'} },
            '清远市': { code: '441800', districts: {'清城区': '441802', '清新区': '441803'} },
            '东莞市': { code: '441900', districts: {'东莞市': '441900'} },
            '中山市': { code: '442000', districts: {'中山市': '442000'} },
            '潮州市': { code: '445100', districts: {'湘桥区': '445102'} },
            '揭阳市': { code: '445200', districts: {'榕城区': '445202'} },
            '云浮市': { code: '445300', districts: {'云城区': '445302', '云安区': '445303'} }
        }
    },
    '江苏省': {
        code: '320000',
        cities: {
            '南京市': { code: '320100', districts: {'玄武区': '320102', '秦淮区': '320104', '建邺区': '320105', '鼓楼区': '320106', '浦口区': '320111', '栖霞区': '320113', '雨花台区': '320114', '江宁区': '320115', '六合区': '320116', '溧水区': '320117', '高淳区': '320118'} },
            '无锡市': { code: '320200', districts: {'锡山区': '320205', '惠山区': '320206', '滨湖区': '320211', '梁溪区': '320213', '新吴区': '320214'} },
            '徐州市': { code: '320300', districts: {'鼓楼区': '320302', '云龙区': '320303', '贾汪区': '320305', '泉山区': '320311', '铜山区': '320312'} },
            '常州市': { code: '320400', districts: {'天宁区': '320402', '钟楼区': '320404', '新北区': '320411', '武进区': '320412', '金坛区': '320413'} },
            '苏州市': { code: '320500', districts: {'虎丘区': '320505', '吴中区': '320506', '相城区': '320507', '姑苏区': '320508', '吴江区': '320509'} },
            '南通市': { code: '320600', districts: {'崇川区': '320602', '港闸区': '320611', '通州区': '320612'} },
            '连云港市': { code: '320700', districts: {'连云区': '320703', '海州区': '320706', '赣榆区': '320707'} },
            '淮安市': { code: '320800', districts: {'淮安区': '320803', '淮阴区': '320804', '清江浦区': '320812', '洪泽区': '320813'} },
            '盐城市': { code: '320900', districts: {'亭湖区': '320902', '盐都区': '320903', '大丰区': '320904'} },
            '扬州市': { code: '321000', districts: {'广陵区': '321002', '邗江区': '321003', '江都区': '321012'} },
            '镇江市': { code: '321100', districts: {'京口区': '321102', '润州区': '321111', '丹徒区': '321112'} },
            '泰州市': { code: '321200', districts: {'海陵区': '321202', '高港区': '321203', '姜堰区': '321204'} },
            '宿迁市': { code: '321300', districts: {'宿城区': '321302', '宿豫区': '321311'} }
        }
    },
    '浙江省': {
        code: '330000',
        cities: {
            '杭州市': { code: '330100', districts: {'上城区': '330102', '下城区': '330103', '江干区': '330104', '拱墅区': '330105', '西湖区': '330106', '滨江区': '330108', '萧山区': '330109', '余杭区': '330110', '富阳区': '330111', '临安区': '330112'} },
            '宁波市': { code: '330200', districts: {'海曙区': '330203', '江北区': '330205', '北仑区': '330206', '镇海区': '330211', '鄞州区': '330212', '奉化区': '330213'} },
            '温州市': { code: '330300', districts: {'鹿城区': '330302', '龙湾区': '330303', '瓯海区': '330304', '洞头区': '330305'} },
            '嘉兴市': { code: '330400', districts: {'南湖区': '330402', '秀洲区': '330411'} },
            '湖州市': { code: '330500', districts: {'吴兴区': '330502', '南浔区': '330503'} },
            '绍兴市': { code: '330600', districts: {'越城区': '330602', '柯桥区': '330603', '上虞区': '330604'} },
            '金华市': { code: '330700', districts: {'婺城区': '330702', '金东区': '330703'} },
            '衢州市': { code: '330800', districts: {'柯城区': '330802', '衢江区': '330803'} },
            '舟山市': { code: '330900', districts: {'定海区': '330902', '普陀区': '330903'} },
            '台州市': { code: '331000', districts: {'椒江区': '331002', '黄岩区': '331003', '路桥区': '331004'} },
            '丽水市': { code: '331100', districts: {'莲都区': '331102'} }
        }
    },
    '山东省': {
        code: '370000',
        cities: {
            '济南市': { code: '370100', districts: {'历下区': '370102', '市中区': '370103', '槐荫区': '370104', '天桥区': '370105', '历城区': '370112', '长清区': '370113', '章丘区': '370114'} },
            '青岛市': { code: '370200', districts: {'市南区': '370202', '市北区': '370203', '黄岛区': '370211', '崂山区': '370212', '李沧区': '370213', '城阳区': '370214', '即墨区': '370215'} },
            '淄博市': { code: '370300', districts: {'淄川区': '370302', '张店区': '370303', '博山区': '370304', '临淄区': '370305', '周村区': '370306'} },
            '枣庄市': { code: '370400', districts: {'市中区': '370402', '薛城区': '370403', '峄城区': '370404', '台儿庄区': '370405', '山亭区': '370406'} },
            '东营市': { code: '370500', districts: {'东营区': '370502', '河口区': '370503'} },
            '烟台市': { code: '370600', districts: {'芝罘区': '370602', '福山区': '370611', '牟平区': '370612', '莱山区': '370613'} },
            '潍坊市': { code: '370700', districts: {'潍城区': '370702', '寒亭区': '370703', '坊子区': '370704', '奎文区': '370705'} },
            '济宁市': { code: '370800', districts: {'任城区': '370811', '兖州区': '370812'} },
            '泰安市': { code: '370900', districts: {'泰山区': '370902', '岱岳区': '370911'} },
            '威海市': { code: '371000', districts: {'环翠区': '371002', '文登区': '371003'} },
            '日照市': { code: '371100', districts: {'东港区': '371102', '岚山区': '371103'} },
            '临沂市': { code: '371300', districts: {'兰山区': '371302', '罗庄区': '371311', '河东区': '371312'} },
            '德州市': { code: '371400', districts: {'德城区': '371402', '陵城区': '371403'} },
            '聊城市': { code: '371500', districts: {'东昌府区': '371502', '茌平区': '371503'} },
            '滨州市': { code: '371600', districts: {'滨城区': '371602', '沾化区': '371603'} },
            '菏泽市': { code: '371700', districts: {'牡丹区': '371702', '定陶区': '371703'} }
        }
    },
    '河北省': {
        code: '130000',
        cities: {
            '石家庄市': { code: '130100', districts: {'长安区': '130102', '桥西区': '130104', '新华区': '130105', '井陉矿区': '130107', '裕华区': '130108', '藁城区': '130109', '鹿泉区': '130110', '栾城区': '130111'} },
            '唐山市': { code: '130200', districts: {'路南区': '130202', '路北区': '130203', '古冶区': '130204', '开平区': '130205', '丰南区': '130207', '丰润区': '130208', '曹妃甸区': '130209'} },
            '秦皇岛市': { code: '130300', districts: {'海港区': '130302', '山海关区': '130303', '北戴河区': '130304', '抚宁区': '130306'} },
            '邯郸市': { code: '130400', districts: {'邯山区': '130402', '丛台区': '130403', '复兴区': '130404', '峰峰矿区': '130406'} },
            '邢台市': { code: '130500', districts: {'桥东区': '130502', '桥西区': '130503'} },
            '保定市': { code: '130600', districts: {'竞秀区': '130602', '莲池区': '130606', '满城区': '130607', '清苑区': '130608', '徐水区': '130609'} },
            '张家口市': { code: '130700', districts: {'桥东区': '130702', '桥西区': '130703', '宣化区': '130705', '下花园区': '130706'} },
            '承德市': { code: '130800', districts: {'双桥区': '130802', '双滦区': '130803', '鹰手营子矿区': '130804'} },
            '沧州市': { code: '130900', districts: {'新华区': '130902', '运河区': '130903'} },
            '廊坊市': { code: '131000', districts: {'安次区': '131002', '广阳区': '131003'} },
            '衡水市': { code: '131100', districts: {'桃城区': '131102', '冀州区': '131103'} }
        }
    },
    '四川省': {
        code: '510000',
        cities: {
            '成都市': { code: '510100', districts: {'锦江区': '510104', '青羊区': '510105', '金牛区': '510106', '武侯区': '510107', '成华区': '510108', '龙泉驿区': '510112', '青白江区': '510113', '新都区': '510114', '温江区': '510115', '双流区': '510116', '郫都区': '510117', '新津区': '510118'} },
            '自贡市': { code: '510300', districts: {'自流井区': '510302', '贡井区': '510303', '大安区': '510304', '沿滩区': '510311'} },
            '攀枝花市': { code: '510400', districts: {'东区': '510402', '西区': '510403', '仁和区': '510411', '米易县': '510421', '盐边县': '510422'} },
            '泸州市': { code: '510500', districts: {'江阳区': '510502', '纳溪区': '510503', '龙马潭区': '510504'} },
            '德阳市': { code: '510600', districts: {'旌阳区': '510603', '罗江区': '510604'} },
            '绵阳市': { code: '510700', districts: {'涪城区': '510703', '游仙区': '510704', '安州区': '510705'} },
            '广元市': { code: '510800', districts: {'利州区': '510802', '昭化区': '510811', '朝天区': '510812'} },
            '遂宁市': { code: '510900', districts: {'船山区': '510903', '安居区': '510904'} },
            '内江市': { code: '511000', districts: {'市中区': '511002', '东兴区': '511011'} },
            '乐山市': { code: '511100', districts: {'市中区': '511102', '沙湾区': '511111', '五通桥区': '511112', '金口河区': '511113'} },
            '南充市': { code: '511300', districts: {'顺庆区': '511302', '高坪区': '511303', '嘉陵区': '511304'} },
            '眉山市': { code: '511400', districts: {'东坡区': '511402', '彭山区': '511403'} },
            '宜宾市': { code: '511500', districts: {'翠屏区': '511502', '南溪区': '511503', '叙州区': '511504'} },
            '广安市': { code: '511600', districts: {'广安区': '511602', '前锋区': '511603'} },
            '达州市': { code: '511700', districts: {'通川区': '511702', '达川区': '511703'} },
            '雅安市': { code: '511800', districts: {'雨城区': '511802', '名山区': '511803'} },
            '巴中市': { code: '511900', districts: {'巴州区': '511902', '恩阳区': '511903'} },
            '资阳市': { code: '512000', districts: {'雁江区': '512002'} }
        }
    }
};

function initLocationData() {
    const provinceSelect = document.getElementById('provinceSelect');
    
    // 填充省份选项
    Object.keys(locationData).forEach(province => {
        const option = document.createElement('option');
        option.value = province;
        option.textContent = province;
        provinceSelect.appendChild(option);
    });
}

// 省份变化事件
function onProvinceChange() {
    const provinceSelect = document.getElementById('provinceSelect');
    const citySelect = document.getElementById('citySelect');
    const districtSelect = document.getElementById('districtSelect');
    const selectedLocation = document.getElementById('selectedLocation');
    const generatedCode = document.getElementById('generatedCode');
    const saveCityBtn = document.getElementById('saveCityBtn');
    
    const selectedProvince = provinceSelect.value;
    
    // 清空城市和区县选项
    citySelect.innerHTML = '<option value="">请选择城市</option>';
    districtSelect.innerHTML = '<option value="">请先选择城市</option>';
    citySelect.disabled = true;
    districtSelect.disabled = true;
    
    // 重置显示
    selectedLocation.textContent = '未选择';
    generatedCode.textContent = '-';
    saveCityBtn.disabled = true;
    
    if (selectedProvince && locationData[selectedProvince]) {
        const cities = locationData[selectedProvince].cities;
        
        // 填充城市选项
        Object.keys(cities).forEach(city => {
            const option = document.createElement('option');
            option.value = city;
            option.textContent = city;
            citySelect.appendChild(option);
        });
        
        citySelect.disabled = false;
        
        // 更新显示
        selectedLocation.textContent = selectedProvince;
        generatedCode.textContent = locationData[selectedProvince].code;
    }
}

// 城市变化事件
function onCityChange() {
    const provinceSelect = document.getElementById('provinceSelect');
    const citySelect = document.getElementById('citySelect');
    const districtSelect = document.getElementById('districtSelect');
    const selectedLocation = document.getElementById('selectedLocation');
    const generatedCode = document.getElementById('generatedCode');
    const saveCityBtn = document.getElementById('saveCityBtn');
    
    const selectedProvince = provinceSelect.value;
    const selectedCity = citySelect.value;
    
    // 清空区县选项
    districtSelect.innerHTML = '<option value="">请选择区县</option>';
    districtSelect.disabled = true;
    
    if (selectedProvince && selectedCity && locationData[selectedProvince] && locationData[selectedProvince].cities[selectedCity]) {
        const districts = locationData[selectedProvince].cities[selectedCity].districts;
        
        // 填充区县选项
        Object.keys(districts).forEach(district => {
            const option = document.createElement('option');
            option.value = district;
            option.textContent = district;
            districtSelect.appendChild(option);
        });
        
        districtSelect.disabled = false;
        
        // 更新显示
        selectedLocation.textContent = selectedProvince + ' ' + selectedCity;
        generatedCode.textContent = locationData[selectedProvince].cities[selectedCity].code;
        saveCityBtn.disabled = false; // 选择了城市就可以保存
    }
}

// 区县变化事件
function onDistrictChange() {
    const provinceSelect = document.getElementById('provinceSelect');
    const citySelect = document.getElementById('citySelect');
    const districtSelect = document.getElementById('districtSelect');
    const selectedLocation = document.getElementById('selectedLocation');
    const generatedCode = document.getElementById('generatedCode');
    const saveCityBtn = document.getElementById('saveCityBtn');
    
    const selectedProvince = provinceSelect.value;
    const selectedCity = citySelect.value;
    const selectedDistrict = districtSelect.value;
    
    if (selectedProvince && selectedCity && selectedDistrict && 
        locationData[selectedProvince] && 
        locationData[selectedProvince].cities[selectedCity] &&
        locationData[selectedProvince].cities[selectedCity].districts[selectedDistrict]) {
        
        // 更新显示
        selectedLocation.textContent = selectedProvince + ' ' + selectedCity + ' ' + selectedDistrict;
        generatedCode.textContent = locationData[selectedProvince].cities[selectedCity].districts[selectedDistrict];
        saveCityBtn.disabled = false;
    }
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
    document.getElementById('enableForecast').checked = config.enableForecast || false;
    
    // 更新城市信息 - 尝试根据城市代码匹配级联选择器
    if (config.cityCode) {
        const matchedLocation = findLocationByCode(config.cityCode);
        if (matchedLocation) {
            // 设置级联选择器
            document.getElementById('provinceSelect').value = matchedLocation.province;
            onProvinceChange();
            
            setTimeout(() => {
                document.getElementById('citySelect').value = matchedLocation.city;
                onCityChange();
                
                if (matchedLocation.district) {
                    setTimeout(() => {
                        document.getElementById('districtSelect').value = matchedLocation.district;
                        onDistrictChange();
                    }, 100);
                }
            }, 100);
        }
    }
}

// 根据城市代码查找对应的省市区
function findLocationByCode(code) {
    for (const province in locationData) {
        if (locationData[province].code === code) {
            return { province: province, city: null, district: null };
        }
        
        const cities = locationData[province].cities;
        for (const city in cities) {
            if (cities[city].code === code) {
                return { province: province, city: city, district: null };
            }
            
            const districts = cities[city].districts;
            for (const district in districts) {
                if (districts[district] === code) {
                    return { province: province, city: city, district: district };
                }
            }
        }
    }
    return null;
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

async function loadWeatherStats() {
    try {
        const response = await fetch('/api/weather/stats');
        const data = await response.json();
        if (data.success) {
            updateStatsDisplay(data.stats);
        }
    } catch (error) {
        console.error('加载统计信息失败:', error);
    }
}

function updateStatsDisplay(stats) {
    document.getElementById('totalRequests').textContent = stats.totalRequests || 0;
    document.getElementById('successRequests').textContent = stats.successRequests || 0;
    document.getElementById('failedRequests').textContent = stats.failedRequests || 0;
    const lastUpdate = stats.lastUpdateTime ? new Date(stats.lastUpdateTime).toLocaleString() : '从未更新';
    document.getElementById('lastUpdate').textContent = lastUpdate;
}

function startWeatherUpdate() {
    weatherUpdateInterval = setInterval(() => {
        loadCurrentWeather();
        loadWeatherStats();
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

async function saveCityConfig() {
    const provinceSelect = document.getElementById('provinceSelect');
    const citySelect = document.getElementById('citySelect');
    const districtSelect = document.getElementById('districtSelect');
    const generatedCode = document.getElementById('generatedCode');
    const selectedLocation = document.getElementById('selectedLocation');
    
    const selectedProvince = provinceSelect.value;
    const selectedCity = citySelect.value;
    const selectedDistrict = districtSelect.value;
    const cityCode = generatedCode.textContent;
    const locationText = selectedLocation.textContent;
    
    if (!selectedProvince || !selectedCity || cityCode === '-') {
        showToast('请选择省份和城市', 'warning');
        return;
    }
    
    try {
        const formData = new FormData();
        
        // 构建城市名称
        let cityName = '';
        if (selectedDistrict) {
            cityName = selectedDistrict;
        } else {
            cityName = selectedCity;
        }
        
        formData.append('cityName', cityName);
        formData.append('cityCode', cityCode);
        formData.append('fullLocation', locationText);
        
        const response = await fetch('/api/weather/city', {
            method: 'POST',
            body: formData
        });
        
        const data = await response.json();
        if (data.success) {
            showToast('城市设置保存成功', 'success');
            setTimeout(loadCurrentWeather, 1000);
        } else {
            showToast('城市设置保存失败: ' + data.message, 'error');
        }
    } catch (error) {
        showToast('保存城市设置失败', 'error');
    }
}

async function saveUpdateConfig() {
    const autoUpdate = document.getElementById('autoUpdate').checked;
    const updateInterval = parseInt(document.getElementById('updateInterval').value);
    const enableForecast = document.getElementById('enableForecast').checked;
    
    if (updateInterval < 5 || updateInterval > 1440) {
        showToast('更新间隔必须在5-1440分钟之间', 'warning');
        return;
    }
    
    try {
        const formData = new FormData();
        formData.append('autoUpdate', autoUpdate);
        formData.append('updateInterval', updateInterval);
        formData.append('enableForecast', enableForecast);
        
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
                loadWeatherStats();
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
        
        /* 统计信息样式 */
        .weather-stats {
            display: grid;
            gap: 16px;
        }
        
        .stat-item {
            display: flex;
            justify-content: space-between;
            align-items: center;
            padding: 16px;
            background: linear-gradient(135deg, #f8fafc, #f1f5f9);
            border-radius: 12px;
            border: 1px solid #e2e8f0;
            transition: all 0.3s ease;
        }
        
        .stat-item:hover {
            transform: translateX(4px);
            box-shadow: 0 4px 12px rgba(0,0,0,0.05);
        }
        
        .stat-item .label {
            font-weight: 500;
            color: #4b5563;
            display: flex;
            align-items: center;
            gap: 8px;
        }
        
        .stat-item .value {
            font-weight: 600;
            color: #1f2937;
            background: white;
            padding: 6px 12px;
            border-radius: 8px;
            box-shadow: 0 2px 4px rgba(0,0,0,0.05);
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
        
        /* 省市区选择器样式 */
        .location-selector {
            background: #f8fafc;
            border-radius: 16px;
            padding: 24px;
            margin-bottom: 20px;
            border: 1px solid #e2e8f0;
        }
        
        .form-row {
            display: grid;
            grid-template-columns: repeat(auto-fit, minmax(200px, 1fr));
            gap: 16px;
            margin-bottom: 20px;
        }
        
        .location-select {
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
        
        .location-select:focus {
            outline: none;
            border-color: #14b8a6;
            box-shadow: 0 0 0 3px rgba(20, 184, 166, 0.1);
        }
        
        .location-select:disabled {
            background-color: #f3f4f6;
            color: #9ca3af;
            cursor: not-allowed;
            border-color: #d1d5db;
        }
        
        .selected-location {
            background: white;
            border-radius: 12px;
            padding: 20px;
            border: 1px solid #e2e8f0;
            box-shadow: 0 2px 8px rgba(0, 0, 0, 0.04);
        }
        
        .location-info, .location-code {
            display: flex;
            align-items: center;
            gap: 12px;
            margin-bottom: 8px;
        }
        
        .location-code {
            margin-bottom: 0;
        }
        
        .location-label, .code-label {
            font-weight: 600;
            color: #374151;
            min-width: 80px;
        }
        
        .location-text {
            color: #14b8a6;
            font-weight: 600;
            font-size: 1.1rem;
        }
        
        .code-text {
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
    js += "            const value = Math.max(1, Math.min(1440, parseInt(this.value) || 1));\n";
    js += "            this.value = value;\n";
    js += "            timeoutSlider.value = Math.min(60, value);\n";
    js += "            timeoutValue.textContent = Math.min(60, value);\n";
    js += "        });\n";
    js += "    }\n\n";
    
    js += "    const startTimePicker = document.getElementById('startTime');\n";
    js += "    const endTimePicker = document.getElementById('endTime');\n";
    js += "    if (startTimePicker) {\n";
    js += "        startTimePicker.addEventListener('change', function() {\n";
    js += "            updateTimeFromPicker('start', this.value);\n";
    js += "        });\n";
    js += "    }\n";
    js += "    if (endTimePicker) {\n";
    js += "        endTimePicker.addEventListener('change', function() {\n";
    js += "            updateTimeFromPicker('end', this.value);\n";
    js += "        });\n";
    js += "    }\n";
    js += "}\n\n";
    // Raw string literal代码已转换为字符串拼接格式，删除重复部分
                
    // 添加关键的JavaScript函数
    js += "function updateTimeFromPicker(type, timeValue) {\n";
    js += "    const [hours, minutes] = timeValue.split(':').map(num => parseInt(num));\n";
    js += "    const hourElement = document.getElementById(type + 'Hour');\n";
    js += "    const minuteElement = document.getElementById(type + 'Minute');\n";
    js += "    if (hourElement) hourElement.value = hours;\n";
    js += "    if (minuteElement) minuteElement.value = minutes;\n";
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
    js += "                selectMode(data.mode || 0);\n";
    js += "                const startHour = document.getElementById('startHour');\n";
    js += "                const startMinute = document.getElementById('startMinute');\n";
    js += "                const endHour = document.getElementById('endHour');\n";
    js += "                const endMinute = document.getElementById('endMinute');\n";
    js += "                const startHourVal = data.startHour || 8;\n";
    js += "                const startMinuteVal = data.startMinute || 0;\n";
    js += "                const endHourVal = data.endHour || 22;\n";
    js += "                const endMinuteVal = data.endMinute || 0;\n";
    js += "                if (startHour) startHour.value = startHourVal;\n";
    js += "                if (startMinute) startMinute.value = startMinuteVal;\n";
    js += "                if (endHour) endHour.value = endHourVal;\n";
    js += "                if (endMinute) endMinute.value = endMinuteVal;\n";
    js += "                updatePickerFromTime('start', startHourVal, startMinuteVal);\n";
    js += "                updatePickerFromTime('end', endHourVal, endMinuteVal);\n";
    js += "                const timeoutMinutes = data.timeoutMinutes || 10;\n";
    js += "                const timeoutInput = document.getElementById('timeoutMinutes');\n";
    js += "                const timeoutSlider = document.getElementById('timeoutSlider');\n";
    js += "                const timeoutValue = document.getElementById('timeoutValue');\n";
    js += "                if (timeoutInput) timeoutInput.value = timeoutMinutes;\n";
    js += "                if (timeoutSlider) timeoutSlider.value = Math.min(60, timeoutMinutes);\n";
    js += "                if (timeoutValue) timeoutValue.textContent = Math.min(60, timeoutMinutes);\n";
    js += "            } else {\n";
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

// 服务器设置HTML已移至WebServerManager_ServerSettings.cpp

// 服务器设置CSS和JavaScript已移至WebServerManager_ServerSettings.cpp
