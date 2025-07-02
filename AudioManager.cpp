/*
 * AudioManager.cpp - 音频管理器类实现文件
 * ESP32S3监控项目 - 音频播放管理模块
 */

#include "AudioManager.h"
#include "I2CBusManager.h"

static const char* TAG = "AudioManager";

// 构造函数
AudioManager::AudioManager()
    : m_initialized(false)
    , m_running(false)
    , m_debugMode(false)
    , m_psramManager(nullptr)
    , m_fileManager(nullptr)
    , m_i2sTxHandle(nullptr)
    , m_i2sRxHandle(nullptr)
    , m_es8311Handle(nullptr)
    , m_currentState(AUDIO_STATE_IDLE)
    , m_currentMode(AUDIO_MODE_ONCE)
    , m_currentVolume(50)
    , m_isMuted(false)
    , m_playPosition(0)
    , m_playTaskHandle(nullptr)
    , m_mutex(nullptr)
    , m_audioQueue(nullptr)
    , m_callback(nullptr)
{
    // 初始化音频文件信息
    m_currentAudioFile = AudioFileInfo();
    
    // 初始化统计信息
    m_statistics = AudioStatistics();
}

// 析构函数
AudioManager::~AudioManager()
{
    stop();
    
    // 清理音频数据
    unloadPCMData();
    
    // 删除信号量和队列
    if (m_mutex != nullptr) {
        vSemaphoreDelete(m_mutex);
        m_mutex = nullptr;
    }
    
    if (m_audioQueue != nullptr) {
        vQueueDelete(m_audioQueue);
        m_audioQueue = nullptr;
    }
    
    // 清理I2S
    if (m_i2sTxHandle != nullptr) {
        i2s_del_channel(m_i2sTxHandle);
        m_i2sTxHandle = nullptr;
    }
    
    if (m_i2sRxHandle != nullptr) {
        i2s_del_channel(m_i2sRxHandle);
        m_i2sRxHandle = nullptr;
    }
    
    // 清理ES8311
    if (m_es8311Handle != nullptr) {
        es8311_delete(m_es8311Handle);
        m_es8311Handle = nullptr;
    }
}

// 初始化音频管理器
bool AudioManager::init(PSRAMManager* psramManager, FileManager* fileManager)
{
    if (m_initialized) {
        logInfo("音频管理器已经初始化");
        return true;
    }
    
    if (psramManager == nullptr || fileManager == nullptr) {
        logError("PSRAM管理器或文件管理器为空");
        return false;
    }
    
    m_psramManager = psramManager;
    m_fileManager = fileManager;
    
    // 创建信号量
    m_mutex = xSemaphoreCreateMutex();
    if (m_mutex == nullptr) {
        logError("创建互斥锁失败");
        return false;
    }
    
    // 创建音频队列
    m_audioQueue = xQueueCreate(AUDIO_QUEUE_SIZE, sizeof(uint32_t));
    if (m_audioQueue == nullptr) {
        logError("创建音频队列失败");
        return false;
    }
    
    // 初始化GPIO
    if (!initGPIO()) {
        logError("GPIO初始化失败");
        return false;
    }
    
    // 初始化I2S
    if (!initI2S()) {
        logError("I2S初始化失败");
        return false;
    }
    
    // 初始化ES8311
    if (!initES8311()) {
        logError("ES8311初始化失败");
        return false;
    }
    
    m_initialized = true;
    logInfo("音频管理器初始化成功");
    
    return true;
}

// 初始化GPIO
bool AudioManager::initGPIO()
{
    // 配置功放使能引脚
    gpio_config_t io_conf = {};
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pin_bit_mask = (1ULL << PA);
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
    
    esp_err_t ret = gpio_config(&io_conf);
    if (ret != ESP_OK) {
        logError("功放引脚配置失败");
        return false;
    }
    
    // 默认启用功放
    gpio_set_level((gpio_num_t)PA, 1);
    
    return true;
}

// 初始化I2S
bool AudioManager::initI2S()
{
    esp_err_t ret;
    
    // 配置I2S通道
    i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_0, I2S_ROLE_MASTER);
    chan_cfg.auto_clear = true;
    
    ret = i2s_new_channel(&chan_cfg, &m_i2sTxHandle, &m_i2sRxHandle);
    if (ret != ESP_OK) {
        logError("创建I2S通道失败");
        return false;
    }
    
    // 配置I2S标准模式
    i2s_std_config_t std_cfg = {
        .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(AUDIO_SAMPLE_RATE),
        .slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_MONO),
        .gpio_cfg = {
            .mclk = (gpio_num_t)I2S_MCK_IO,
            .bclk = (gpio_num_t)I2S_BCK_IO,
            .ws = (gpio_num_t)I2S_WS_IO,
            .dout = (gpio_num_t)I2S_DO_IO,
            .din = (gpio_num_t)I2S_DI_IO,
            .invert_flags = {
                .mclk_inv = false,
                .bclk_inv = false,
                .ws_inv = false,
            },
        },
    };
    
    std_cfg.clk_cfg.mclk_multiple = (i2s_mclk_multiple_t)AUDIO_MCLK_MULTIPLE;
    
    // 初始化TX通道
    ret = i2s_channel_init_std_mode(m_i2sTxHandle, &std_cfg);
    if (ret != ESP_OK) {
        logError("初始化I2S TX通道失败");
        return false;
    }
    
    // 初始化RX通道
    ret = i2s_channel_init_std_mode(m_i2sRxHandle, &std_cfg);
    if (ret != ESP_OK) {
        logError("初始化I2S RX通道失败");
        return false;
    }
    
    // 启用I2S通道
    ret = i2s_channel_enable(m_i2sTxHandle);
    if (ret != ESP_OK) {
        logError("启用I2S TX通道失败");
        return false;
    }
    
    ret = i2s_channel_enable(m_i2sRxHandle);
    if (ret != ESP_OK) {
        logError("启用I2S RX通道失败");
        return false;
    }
    
    return true;
}

// 初始化ES8311
bool AudioManager::initES8311()
{
    // 创建ES8311设备句柄
    m_es8311Handle = es8311_create(ES8311_I2C_ADDR);
    if (m_es8311Handle == nullptr) {
        logError("创建ES8311句柄失败");
        return false;
    }
    
    // 配置ES8311时钟
    es8311_clock_config_t clk_cfg = {
        .mclk_inverted = false,
        .sclk_inverted = false,
        .mclk_from_mclk_pin = true,
        .mclk_frequency = AUDIO_MCLK_FREQ,
        .sample_frequency = AUDIO_SAMPLE_RATE
    };
    
    // 初始化ES8311
    esp_err_t ret = es8311_init(m_es8311Handle, &clk_cfg, ES8311_RESOLUTION_16, ES8311_RESOLUTION_16);
    if (ret != ESP_OK) {
        logError("ES8311初始化失败");
        return false;
    }
    
    // 设置音量
    ret = es8311_voice_volume_set(m_es8311Handle, m_currentVolume, nullptr);
    if (ret != ESP_OK) {
        logError("设置ES8311音量失败");
        return false;
    }
    
    // 配置麦克风
    ret = es8311_microphone_config(m_es8311Handle, false);
    if (ret != ESP_OK) {
        logError("配置ES8311麦克风失败");
        return false;
    }
    
    return true;
}

// 启动音频管理器
bool AudioManager::start()
{
    if (!m_initialized) {
        logError("音频管理器未初始化");
        return false;
    }
    
    if (m_running) {
        logInfo("音频管理器已经运行");
        return true;
    }
    
    // 创建音频播放任务，使用SRAM栈，运行在核心0
    BaseType_t result = xTaskCreatePinnedToCore(
        audioPlayTask,
        "AudioPlayTask",
        AUDIO_TASK_STACK_SIZE,
        this,
        AUDIO_TASK_PRIORITY,
        &m_playTaskHandle,
        0  // 运行在核心0
    );
    
    if (result != pdTRUE) {
        m_playTaskHandle = nullptr;
    }
    
    if (m_playTaskHandle == nullptr) {
        logError("创建音频播放任务失败");
        return false;
    }
    
    m_running = true;
    setState(AUDIO_STATE_IDLE);
    
    logInfo("音频管理器启动成功");
    return true;
}

// 停止音频管理器
void AudioManager::stop()
{
    if (!m_running) {
        return;
    }
    
    m_running = false;
    
    // 停止播放
    stopPlayback();
    
    // 等待任务结束
    if (m_playTaskHandle != nullptr) {
        vTaskDelete(m_playTaskHandle);
        m_playTaskHandle = nullptr;
    }
    
    // 禁用I2S通道
    if (m_i2sTxHandle != nullptr) {
        i2s_channel_disable(m_i2sTxHandle);
    }
    
    if (m_i2sRxHandle != nullptr) {
        i2s_channel_disable(m_i2sRxHandle);
    }
    
    logInfo("音频管理器已停止");
}

// 播放PCM文件
bool AudioManager::playPCMFile(const String& filePath, AudioPlayMode mode)
{
    if (!m_initialized || !m_running) {
        logError("音频管理器未初始化或未运行");
        return false;
    }
    
    if (!takeMutex()) {
        logError("获取互斥锁失败");
        return false;
    }
    
    // 停止当前播放
    if (m_currentState == AUDIO_STATE_PLAYING) {
        setState(AUDIO_STATE_STOPPED);
    }
    
    // 加载新的音频文件
    bool loadResult = loadPCMData(filePath);
    if (!loadResult) {
        giveMutex();
        logError("加载PCM文件失败: " + filePath);
        return false;
    }
    
    // 设置播放参数
    m_currentFile = filePath;
    m_currentMode = mode;
    m_playPosition = 0;
    
    // 开始播放
    setState(AUDIO_STATE_PLAYING);
    
    giveMutex();
    
    logInfo("开始播放PCM文件: " + filePath);
    return true;
}

// 音频播放任务
void AudioManager::audioPlayTask(void* parameter)
{
    AudioManager* audioManager = static_cast<AudioManager*>(parameter);
    audioManager->audioPlayTaskImpl();
}

// 音频播放任务实现
void AudioManager::audioPlayTaskImpl()
{
    TickType_t lastPlayTime = 0;
    bool playCompleted = false;  // 标记当前音频是否播放完成
    
    while (m_running) {
        if (!takeMutex(pdMS_TO_TICKS(100))) {
            continue;
        }
        
        if (m_currentState == AUDIO_STATE_PLAYING && m_currentAudioFile.isLoaded) {
            TickType_t currentTime = xTaskGetTickCount();
            
            // 处理间隔循环播放模式
            if (m_currentMode == AUDIO_MODE_INTERVAL_LOOP) {
                if (playCompleted) {
                    // 播放完成，检查是否到达下次播放时间
                    if (pdTICKS_TO_MS(currentTime - lastPlayTime) >= AUDIO_PLAY_INTERVAL) {
                        m_playPosition = 0;  // 重置播放位置，开始新的播放周期
                        playCompleted = false;
                        lastPlayTime = currentTime;
                        logInfo("开始新的播放周期");
                    } else {
                        // 还未到播放时间，继续等待
                        giveMutex();
                        vTaskDelay(pdMS_TO_TICKS(100));
                        continue;
                    }
                }
            }
            
            // 播放音频数据
            if (m_playPosition < m_currentAudioFile.dataSize) {
                size_t remainingBytes = m_currentAudioFile.dataSize - m_playPosition;
                size_t chunkSize = (remainingBytes > AUDIO_BUFFER_SIZE) ? AUDIO_BUFFER_SIZE : remainingBytes;
                
                uint8_t* dataPtr = m_currentAudioFile.audioData + m_playPosition;
                
                giveMutex();  // 释放锁以避免阻塞
                
                if (writeAudioData(dataPtr, chunkSize)) {
                    if (takeMutex(pdMS_TO_TICKS(100))) {
                        m_playPosition += chunkSize;
                        updateStatistics(true, chunkSize);
                        giveMutex();
                    }
                } else {
                    if (takeMutex(pdMS_TO_TICKS(100))) {
                        updateStatistics(false, 0);
                        giveMutex();
                    }
                }
            } else {
                // 播放完成，处理循环逻辑
                if (m_currentMode == AUDIO_MODE_LOOP) {
                    m_playPosition = 0;  // 立即重新开始
                    logInfo("循环播放：重新开始");
                } else if (m_currentMode == AUDIO_MODE_INTERVAL_LOOP) {
                    // 标记播放完成，设置完成时间
                    playCompleted = true;
                    lastPlayTime = xTaskGetTickCount();
                    logInfo("音频播放完成，等待下一个播放周期");
                } else {
                    // 单次播放完成
                    setState(AUDIO_STATE_IDLE);
                    notifyCallback(AUDIO_STATE_IDLE, m_currentFile);
                    logInfo("单次播放完成");
                }
            }
        } else {
            giveMutex();
            vTaskDelay(pdMS_TO_TICKS(100));
            continue;
        }
        
        giveMutex();
        vTaskDelay(pdMS_TO_TICKS(10));  // 短暂延时避免CPU占用过高
    }
}

// 加载PCM数据
bool AudioManager::loadPCMData(const String& filePath)
{
    // 清理旧数据
    unloadPCMData();
    
    if (!m_fileManager->exists(filePath)) {
        logError("文件不存在: " + filePath);
        return false;
    }
    
    size_t fileSize = m_fileManager->getFileSize(filePath);
    if (fileSize == 0) {
        logError("文件大小为0: " + filePath);
        return false;
    }
    
    // 使用PSRAM分配内存
    uint8_t* audioBuffer = nullptr;
    if (m_psramManager != nullptr) {
        audioBuffer = static_cast<uint8_t*>(
            m_psramManager->allocateDataBuffer(fileSize, "AudioPCMBuffer_" + filePath)
        );
    } else {
        audioBuffer = static_cast<uint8_t*>(malloc(fileSize));
    }
    
    if (audioBuffer == nullptr) {
        logError("分配音频缓冲区失败，大小: " + String(fileSize));
        return false;
    }
    
    // 读取文件数据
    uint8_t* tempBuffer = nullptr;
    size_t actualSize = 0;
    bool readResult = m_fileManager->downloadFile(filePath, tempBuffer, actualSize);
    
    if (!readResult || tempBuffer == nullptr || actualSize != fileSize) {
        if (m_psramManager != nullptr) {
            m_psramManager->deallocate(audioBuffer);
        } else {
            free(audioBuffer);
        }
        if (tempBuffer != nullptr) {
            free(tempBuffer);
        }
        logError("读取文件失败: " + filePath);
        return false;
    }
    
    // 复制数据到PSRAM缓冲区
    memcpy(audioBuffer, tempBuffer, actualSize);
    free(tempBuffer);  // 释放临时缓冲区
    
    // 设置音频文件信息
    m_currentAudioFile.filePath = filePath;
    m_currentAudioFile.fileSize = fileSize;
    m_currentAudioFile.audioData = audioBuffer;
    m_currentAudioFile.dataSize = actualSize;
    m_currentAudioFile.isLoaded = true;
    
    logInfo("PCM文件加载成功: " + filePath + ", 大小: " + String(actualSize));
    return true;
}

// 卸载PCM数据
void AudioManager::unloadPCMData()
{
    if (m_currentAudioFile.isLoaded && m_currentAudioFile.audioData != nullptr) {
        if (m_psramManager != nullptr) {
            m_psramManager->deallocate(m_currentAudioFile.audioData);
        } else {
            free(m_currentAudioFile.audioData);
        }
        
        m_currentAudioFile.audioData = nullptr;
        m_currentAudioFile.isLoaded = false;
        m_currentAudioFile.dataSize = 0;
        m_currentAudioFile.filePath = "";
        
        logInfo("PCM数据已卸载");
    }
}

// 写入音频数据
bool AudioManager::writeAudioData(const uint8_t* data, size_t size)
{
    if (m_i2sTxHandle == nullptr || data == nullptr || size == 0) {
        return false;
    }
    
    size_t bytesWritten = 0;
    esp_err_t ret = i2s_channel_write(m_i2sTxHandle, data, size, &bytesWritten, pdMS_TO_TICKS(1000));
    
    if (ret != ESP_OK) {
        logError("I2S写入失败，错误码: " + String(ret));
        return false;
    }
    
    if (bytesWritten != size) {
        logError("I2S写入不完整，期望: " + String(size) + ", 实际: " + String(bytesWritten));
        return false;
    }
    
    return true;
}

// 设置音量
bool AudioManager::setVolume(uint8_t volume)
{
    if (volume > 100) {
        volume = 100;
    }
    
    if (m_es8311Handle == nullptr) {
        return false;
    }
    
    esp_err_t ret = es8311_voice_volume_set(m_es8311Handle, volume, nullptr);
    if (ret == ESP_OK) {
        m_currentVolume = volume;
        logInfo("音量设置为: " + String(volume));
        return true;
    }
    
    logError("设置音量失败");
    return false;
}

// 获取音量
uint8_t AudioManager::getVolume() const
{
    return m_currentVolume;
}

// 静音控制
bool AudioManager::mute(bool isMuted)
{
    if (m_es8311Handle == nullptr) {
        return false;
    }
    
    esp_err_t ret = es8311_voice_mute(m_es8311Handle, isMuted);
    if (ret == ESP_OK) {
        m_isMuted = isMuted;
        logInfo(isMuted ? "已静音" : "取消静音");
        return true;
    }
    
    logError("静音控制失败");
    return false;
}

// 是否静音
bool AudioManager::isMuted() const
{
    return m_isMuted;
}

// 停止播放
bool AudioManager::stopPlayback()
{
    if (!takeMutex()) {
        return false;
    }
    
    setState(AUDIO_STATE_STOPPED);
    m_playPosition = 0;
    
    giveMutex();
    
    logInfo("播放已停止");
    return true;
}

// 获取状态
AudioState AudioManager::getState() const
{
    return m_currentState;
}

// 是否正在播放
bool AudioManager::isPlaying() const
{
    return m_currentState == AUDIO_STATE_PLAYING;
}

// 获取当前文件
String AudioManager::getCurrentFile() const
{
    return m_currentFile;
}

// 设置状态
void AudioManager::setState(AudioState newState)
{
    if (m_currentState != newState) {
        AudioState oldState = m_currentState;
        m_currentState = newState;
        
        if (m_debugMode) {
            logInfo("状态变更: " + stateToString(oldState) + " -> " + stateToString(newState));
        }
        
        notifyCallback(newState, m_currentFile);
    }
}

// 更新统计信息
void AudioManager::updateStatistics(bool success, size_t bytesProcessed)
{
    m_statistics.totalPlayCount++;
    if (success) {
        m_statistics.successPlayCount++;
        m_statistics.bytesPlayed += bytesProcessed;
    } else {
        m_statistics.errorCount++;
    }
    m_statistics.lastPlayTime = millis();
}

// 通知回调
void AudioManager::notifyCallback(AudioState state, const String& filePath)
{
    if (m_callback != nullptr) {
        m_callback(state, filePath);
    }
}

// 获取统计信息
AudioStatistics AudioManager::getStatistics() const
{
    return m_statistics;
}

// 设置回调
void AudioManager::setCallback(AudioCallback callback)
{
    m_callback = callback;
}

// 打印状态
void AudioManager::printStatus()
{
    printf("=== 音频管理器状态 ===\n");
    printf("初始化: %s\n", m_initialized ? "是" : "否");
    printf("运行中: %s\n", m_running ? "是" : "否");
    printf("当前状态: %s\n", stateToString(m_currentState).c_str());
    printf("当前文件: %s\n", m_currentFile.c_str());
    printf("音量: %d%%\n", m_currentVolume);
    printf("静音: %s\n", m_isMuted ? "是" : "否");
    printf("播放次数: %lu\n", m_statistics.totalPlayCount);
    printf("成功次数: %lu\n", m_statistics.successPlayCount);
    printf("错误次数: %lu\n", m_statistics.errorCount);
    printf("播放字节: %zu\n", m_statistics.bytesPlayed);
    printf("====================\n");
}

// 获取状态JSON
String AudioManager::getStatusJSON()
{
    String json = "{";
    json += "\"initialized\":" + String(m_initialized ? "true" : "false") + ",";
    json += "\"running\":" + String(m_running ? "true" : "false") + ",";
    json += "\"state\":\"" + stateToString(m_currentState) + "\",";
    json += "\"currentFile\":\"" + m_currentFile + "\",";
    json += "\"volume\":" + String(m_currentVolume) + ",";
    json += "\"muted\":" + String(m_isMuted ? "true" : "false") + ",";
    json += "\"statistics\":{";
    json += "\"totalPlayCount\":" + String(m_statistics.totalPlayCount) + ",";
    json += "\"successPlayCount\":" + String(m_statistics.successPlayCount) + ",";
    json += "\"errorCount\":" + String(m_statistics.errorCount) + ",";
    json += "\"bytesPlayed\":" + String(m_statistics.bytesPlayed);
    json += "}}";
    return json;
}

// 设置调试模式
void AudioManager::setDebugMode(bool enabled)
{
    m_debugMode = enabled;
    logInfo("调试模式: " + String(enabled ? "开启" : "关闭"));
}

// 线程安全辅助函数
bool AudioManager::takeMutex(TickType_t timeout)
{
    if (m_mutex == nullptr) {
        return false;
    }
    return xSemaphoreTake(m_mutex, timeout) == pdTRUE;
}

void AudioManager::giveMutex()
{
    if (m_mutex != nullptr) {
        xSemaphoreGive(m_mutex);
    }
}

// 工具方法
String AudioManager::stateToString(AudioState state) const
{
    switch (state) {
        case AUDIO_STATE_IDLE: return "空闲";
        case AUDIO_STATE_PLAYING: return "播放中";
        case AUDIO_STATE_PAUSED: return "暂停";
        case AUDIO_STATE_STOPPED: return "停止";
        case AUDIO_STATE_ERROR: return "错误";
        default: return "未知";
    }
}

void AudioManager::logError(const String& message)
{
    printf("[错误] AudioManager: %s\n", message.c_str());
}

void AudioManager::logInfo(const String& message)
{
    if (m_debugMode) {
        printf("[信息] AudioManager: %s\n", message.c_str());
    }
}

// 设置音频格式
bool AudioManager::setAudioFormat(uint32_t sampleRate, uint8_t channels, uint8_t bitsPerSample)
{
    if (!m_initialized) {
        logError("音频管理器未初始化");
        return false;
    }
    
    // 检查参数有效性
    if (sampleRate < 8000 || sampleRate > 96000) {
        logError("采样率必须在8000-96000Hz范围内");
        return false;
    }
    
    if (channels < 1 || channels > 2) {
        logError("声道数必须是1或2");
        return false;
    }
    
    if (bitsPerSample != 16 && bitsPerSample != 24 && bitsPerSample != 32) {
        logError("位深度必须是16、24或32位");
        return false;
    }
    
    // 停止当前播放
    if (m_currentState == AUDIO_STATE_PLAYING) {
        stopPlayback();
        vTaskDelay(pdMS_TO_TICKS(100)); // 等待停止完成
    }
    
    // 禁用I2S通道
    if (m_i2sTxHandle != nullptr) {
        i2s_channel_disable(m_i2sTxHandle);
        i2s_del_channel(m_i2sTxHandle);
        m_i2sTxHandle = nullptr;
    }
    
    if (m_i2sRxHandle != nullptr) {
        i2s_channel_disable(m_i2sRxHandle);
        i2s_del_channel(m_i2sRxHandle);
        m_i2sRxHandle = nullptr;
    }
    
    // 重新配置I2S
    esp_err_t ret;
    
    // 配置I2S通道
    i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_0, I2S_ROLE_MASTER);
    chan_cfg.auto_clear = true;
    
    ret = i2s_new_channel(&chan_cfg, &m_i2sTxHandle, &m_i2sRxHandle);
    if (ret != ESP_OK) {
        logError("重新创建I2S通道失败");
        return false;
    }
    
    // 配置I2S标准模式
    i2s_data_bit_width_t bitWidth;
    switch (bitsPerSample) {
        case 16: bitWidth = I2S_DATA_BIT_WIDTH_16BIT; break;
        case 24: bitWidth = I2S_DATA_BIT_WIDTH_24BIT; break;
        case 32: bitWidth = I2S_DATA_BIT_WIDTH_32BIT; break;
        default: bitWidth = I2S_DATA_BIT_WIDTH_16BIT; break;
    }
    
    i2s_slot_mode_t slotMode = (channels == 1) ? I2S_SLOT_MODE_MONO : I2S_SLOT_MODE_STEREO;
    
    i2s_std_config_t std_cfg = {
        .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(sampleRate),
        .slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(bitWidth, slotMode),
        .gpio_cfg = {
            .mclk = (gpio_num_t)I2S_MCK_IO,
            .bclk = (gpio_num_t)I2S_BCK_IO,
            .ws = (gpio_num_t)I2S_WS_IO,
            .dout = (gpio_num_t)I2S_DO_IO,
            .din = (gpio_num_t)I2S_DI_IO,
            .invert_flags = {
                .mclk_inv = false,
                .bclk_inv = false,
                .ws_inv = false,
            },
        },
    };
    
    // 计算MCLK倍数
    uint32_t mclkMultiple = (sampleRate <= 48000) ? 256 : 128;
    std_cfg.clk_cfg.mclk_multiple = (i2s_mclk_multiple_t)mclkMultiple;
    
    // 初始化TX通道
    ret = i2s_channel_init_std_mode(m_i2sTxHandle, &std_cfg);
    if (ret != ESP_OK) {
        logError("重新初始化I2S TX通道失败");
        return false;
    }
    
    // 初始化RX通道
    ret = i2s_channel_init_std_mode(m_i2sRxHandle, &std_cfg);
    if (ret != ESP_OK) {
        logError("重新初始化I2S RX通道失败");
        return false;
    }
    
    // 启用I2S通道
    ret = i2s_channel_enable(m_i2sTxHandle);
    if (ret != ESP_OK) {
        logError("重新启用I2S TX通道失败");
        return false;
    }
    
    ret = i2s_channel_enable(m_i2sRxHandle);
    if (ret != ESP_OK) {
        logError("重新启用I2S RX通道失败");
        return false;
    }
    
    // 重新配置ES8311
    if (m_es8311Handle != nullptr) {
        // 配置ES8311采样率
        esp_err_t es_ret = es8311_sample_frequency_config(m_es8311Handle, sampleRate * mclkMultiple, sampleRate);
        if (es_ret != ESP_OK) {
            logError("ES8311采样率配置失败");
            return false;
        }
    }
    
    logInfo("音频格式配置成功: " + String(sampleRate) + "Hz, " + 
            String(channels) + "声道, " + String(bitsPerSample) + "位");
    
    return true;
}

// 打印支持的音频格式
void AudioManager::printSupportedFormats()
{
    printf("\n=== 支持的音频格式 ===\n");
    printf("常用PCM格式配置:\n");
    printf("1. 44100Hz, 单声道, 16位 (标准CD音质单声道)\n");
    printf("2. 44100Hz, 立体声, 16位 (标准CD音质立体声)\n");
    printf("3. 22050Hz, 单声道, 16位 (低质量单声道)\n");
    printf("4. 16000Hz, 单声道, 16位 (语音质量)\n");
    printf("5. 8000Hz, 单声道, 16位 (电话质量)\n");
    printf("6. 48000Hz, 立体声, 16位 (专业音频)\n");
    printf("\n使用方法:\n");
    printf("audioManager.setAudioFormat(采样率, 声道数, 位深度);\n");
    printf("例如: audioManager.setAudioFormat(44100, 1, 16);\n");
    printf("========================\n\n");
} 