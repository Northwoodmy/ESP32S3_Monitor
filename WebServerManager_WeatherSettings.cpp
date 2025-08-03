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
    },
    '湖南省': {
        code: '430000',
        cities: {
            '长沙市': { code: '430100', districts: {'芙蓉区': '430102', '天心区': '430103', '岳麓区': '430104', '开福区': '430105', '雨花区': '430111', '望城区': '430112', '长沙县': '430121', '浏阳市': '430181', '宁乡市': '430182'} },
            '株洲市': { code: '430200', districts: {'荷塘区': '430202', '芦淞区': '430203', '石峰区': '430204', '天元区': '430211', '渌口区': '430212', '攸县': '430223', '茶陵县': '430224', '炎陵县': '430225', '醴陵市': '430281'} },
            '湘潭市': { code: '430300', districts: {'雨湖区': '430302', '岳塘区': '430304', '湘潭县': '430321', '湘乡市': '430381', '韶山市': '430382'} },
            '衡阳市': { code: '430400', districts: {'珠晖区': '430405', '雁峰区': '430406', '石鼓区': '430407', '蒸湘区': '430408', '南岳区': '430412', '衡阳县': '430421', '衡南县': '430422', '衡山县': '430423', '衡东县': '430424', '祁东县': '430426', '耒阳市': '430481', '常宁市': '430482'} },
            '邵阳市': { code: '430500', districts: {'双清区': '430502', '大祥区': '430503', '北塔区': '430511', '邵东市': '430582', '新邵县': '430522', '邵阳县': '430523', '隆回县': '430524', '洞口县': '430525', '绥宁县': '430527', '新宁县': '430528', '城步苗族自治县': '430529', '武冈市': '430581'} },
            '岳阳市': { code: '430600', districts: {'岳阳楼区': '430602', '云溪区': '430603', '君山区': '430611', '岳阳县': '430621', '华容县': '430623', '湘阴县': '430624', '平江县': '430626', '汨罗市': '430681', '临湘市': '430682'} },
            '常德市': { code: '430700', districts: {'武陵区': '430702', '鼎城区': '430703', '安乡县': '430721', '汉寿县': '430722', '澧县': '430723', '临澧县': '430724', '桃源县': '430725', '石门县': '430726', '津市市': '430781'} },
            '张家界市': { code: '430800', districts: {'永定区': '430802', '武陵源区': '430811', '慈利县': '430821', '桑植县': '430822'} },
            '益阳市': { code: '430900', districts: {'资阳区': '430902', '赫山区': '430903', '南县': '430921', '桃江县': '430922', '安化县': '430923', '沅江市': '430981'} },
            '郴州市': { code: '431000', districts: {'北湖区': '431002', '苏仙区': '431003', '桂阳县': '431021', '宜章县': '431022', '永兴县': '431023', '嘉禾县': '431024', '临武县': '431025', '汝城县': '431026', '桂东县': '431027', '安仁县': '431028', '资兴市': '431081'} },
            '永州市': { code: '431100', districts: {'零陵区': '431102', '冷水滩区': '431103', '祁阳市': '431181', '东安县': '431122', '双牌县': '431123', '道县': '431124', '江永县': '431125', '宁远县': '431126', '蓝山县': '431127', '新田县': '431128', '江华瑶族自治县': '431129'} },
            '怀化市': { code: '431200', districts: {'鹤城区': '431202', '中方县': '431221', '沅陵县': '431222', '辰溪县': '431223', '溆浦县': '431224', '会同县': '431225', '麻阳苗族自治县': '431226', '新晃侗族自治县': '431227', '芷江侗族自治县': '431228', '靖州苗族侗族自治县': '431229', '通道侗族自治县': '431230', '洪江市': '431281'} },
            '娄底市': { code: '431300', districts: {'娄星区': '431302', '双峰县': '431321', '新化县': '431322', '冷水江市': '431381', '涟源市': '431382'} },
            '湘西土家族苗族自治州': { code: '433100', districts: {'吉首市': '433101', '泸溪县': '433122', '凤凰县': '433123', '花垣县': '433124', '保靖县': '433125', '古丈县': '433126', '永顺县': '433127', '龙山县': '433130'} }
        }
    },
    '湖北省': {
        code: '420000',
        cities: {
            '武汉市': { code: '420100', districts: {'江岸区': '420102', '江汉区': '420103', '硚口区': '420104', '汉阳区': '420105', '武昌区': '420106', '青山区': '420107', '洪山区': '420111', '东西湖区': '420112', '汉南区': '420113', '蔡甸区': '420114', '江夏区': '420115', '黄陂区': '420116', '新洲区': '420117'} },
            '黄石市': { code: '420200', districts: {'黄石港区': '420202', '西塞山区': '420203', '下陆区': '420204', '铁山区': '420205', '阳新县': '420222', '大冶市': '420281'} },
            '十堰市': { code: '420300', districts: {'茅箭区': '420302', '张湾区': '420303', '郧阳区': '420304', '郧西县': '420322', '竹山县': '420323', '竹溪县': '420324', '房县': '420325', '丹江口市': '420381'} },
            '宜昌市': { code: '420500', districts: {'西陵区': '420502', '伍家岗区': '420503', '点军区': '420504', '猇亭区': '420505', '夷陵区': '420506', '远安县': '420525', '兴山县': '420526', '秭归县': '420527', '长阳土家族自治县': '420528', '五峰土家族自治县': '420529', '宜都市': '420581', '当阳市': '420582', '枝江市': '420583'} },
            '襄阳市': { code: '420600', districts: {'襄城区': '420602', '樊城区': '420606', '襄州区': '420607', '南漳县': '420624', '谷城县': '420625', '保康县': '420626', '老河口市': '420682', '枣阳市': '420683', '宜城市': '420684'} },
            '鄂州市': { code: '420700', districts: {'梁子湖区': '420702', '华容区': '420703', '鄂城区': '420704'} },
            '荆门市': { code: '420800', districts: {'东宝区': '420802', '掇刀区': '420804', '沙洋县': '420822', '钟祥市': '420881', '京山市': '420882'} },
            '孝感市': { code: '420900', districts: {'孝南区': '420902', '孝昌县': '420921', '大悟县': '420922', '云梦县': '420923', '应城市': '420981', '安陆市': '420982', '汉川市': '420984'} },
            '荆州市': { code: '421000', districts: {'沙市区': '421002', '荆州区': '421003', '公安县': '421022', '监利市': '421083', '江陵县': '421024', '石首市': '421081', '洪湖市': '421083', '松滋市': '421087'} },
            '黄冈市': { code: '421100', districts: {'黄州区': '421102', '团风县': '421121', '红安县': '421122', '罗田县': '421123', '英山县': '421124', '浠水县': '421125', '蕲春县': '421126', '黄梅县': '421127', '麻城市': '421181', '武穴市': '421182'} },
            '咸宁市': { code: '421200', districts: {'咸安区': '421202', '嘉鱼县': '421221', '通城县': '421222', '崇阳县': '421223', '通山县': '421224', '赤壁市': '421281'} },
            '随州市': { code: '421300', districts: {'曾都区': '421303', '随县': '421321', '广水市': '421381'} },
            '恩施土家族苗族自治州': { code: '422800', districts: {'恩施市': '422801', '利川市': '422802', '建始县': '422822', '巴东县': '422823', '宣恩县': '422825', '咸丰县': '422826', '来凤县': '422827', '鹤峰县': '422828'} },
            '仙桃市': { code: '429004', districts: {'仙桃市': '429004'} },
            '潜江市': { code: '429005', districts: {'潜江市': '429005'} },
            '天门市': { code: '429006', districts: {'天门市': '429006'} },
            '神农架林区': { code: '429021', districts: {'神农架林区': '429021'} }
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
    // enableForecast 开关已从界面移除，但保留配置处理以兼容后端
    
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