/*
 * WebServerManager_OTA.cpp - Web服务器OTA升级页面实现
 * ESP32S3监控项目 - OTA升级模块
 */

#include "WebServerManager.h"
#include "Version.h"

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
    html += "<p><strong>当前版本:</strong> <span id=\"currentVersion\">" + String(VERSION_STRING) + "</span></p>";
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
    js += "            currentVersion.textContent = data.currentVersion || '" + String(VERSION_STRING) + "';\n";
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
    js += "    }\n";
    js += "    finally {\n";
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