/*
 * AudioManager.h - 音频管理器类头文件
 * ESP32S3监控项目 - 音频播放管理模块
 * 
 * 功能描述：
 * - 管理ES8311音频编解码器
 * - 管理I2S音频接口
 * - 实现PCM音频文件播放
 * - 支持循环播放模式
 * - 使用PSRAM进行音频缓冲
 * - 集成文件系统读取音频文件
 */

#ifndef AUDIOMANAGER_H
#define AUDIOMANAGER_H

#include <Arduino.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"
#include "driver/i2s_std.h"
#include "driver/gpio.h"
#include "esp_err.h"
#include "esp_log.h"
#include "es8311_bsp.h"
#include "PSRAMManager.h"
#include "FileManager.h"
#include "pin_config.h"

// 音频配置参数
#define AUDIO_SAMPLE_RATE     44100        // 采样率 (Hz) - 标准CD音质
#define AUDIO_BITS_PER_SAMPLE 16           // 位深度
#define AUDIO_CHANNELS        1            // 声道数 (单声道)
#define AUDIO_MCLK_MULTIPLE   256          // MCLK倍数
#define AUDIO_MCLK_FREQ       (AUDIO_SAMPLE_RATE * AUDIO_MCLK_MULTIPLE)
#define AUDIO_BUFFER_SIZE     4096         // 音频缓冲区大小
#define AUDIO_QUEUE_SIZE      8            // 音频队列深度
#define AUDIO_TASK_STACK_SIZE 8192         // 音频任务栈大小
#define AUDIO_TASK_PRIORITY   5            // 音频任务优先级
#define AUDIO_PLAY_INTERVAL   2000         // 播放间隔 (毫秒)

// ES8311 I2C地址
#define ES8311_I2C_ADDR       0x18

// 音频播放状态枚举
enum AudioState {
    AUDIO_STATE_IDLE,           // 空闲状态
    AUDIO_STATE_PLAYING,        // 播放中
    AUDIO_STATE_PAUSED,         // 暂停
    AUDIO_STATE_STOPPED,        // 停止
    AUDIO_STATE_ERROR           // 错误状态
};

// 音频播放模式枚举
enum AudioPlayMode {
    AUDIO_MODE_ONCE,            // 单次播放
    AUDIO_MODE_LOOP,            // 循环播放
    AUDIO_MODE_INTERVAL_LOOP    // 间隔循环播放
};

// 音频文件信息结构体
struct AudioFileInfo {
    String filePath;            // 文件路径
    size_t fileSize;           // 文件大小
    uint8_t* audioData;        // 音频数据指针 (在PSRAM中)
    size_t dataSize;           // 数据大小
    bool isLoaded;             // 是否已加载
    
    AudioFileInfo() : fileSize(0), audioData(nullptr), dataSize(0), isLoaded(false) {}
};

// 音频统计信息结构体
struct AudioStatistics {
    uint32_t totalPlayCount;    // 总播放次数
    uint32_t successPlayCount;  // 成功播放次数
    uint32_t errorCount;        // 错误次数
    size_t bytesPlayed;         // 播放字节数
    unsigned long lastPlayTime; // 最后播放时间
    unsigned long totalPlayTime; // 总播放时间
    
    AudioStatistics() : totalPlayCount(0), successPlayCount(0), errorCount(0),
                       bytesPlayed(0), lastPlayTime(0), totalPlayTime(0) {}
};

class AudioManager {
public:
    AudioManager();
    ~AudioManager();
    
    // 初始化音频管理器
    bool init(PSRAMManager* psramManager, FileManager* fileManager);
    
    // 启动音频管理器
    bool start();
    
    // 停止音频管理器
    void stop();
    
    // 音频播放控制
    bool playFile(const String& filePath, AudioPlayMode mode = AUDIO_MODE_ONCE);
    bool playPCMFile(const String& filePath, AudioPlayMode mode = AUDIO_MODE_INTERVAL_LOOP);
    bool pause();
    bool resume();
    bool stopPlayback();
    
    // 音量控制
    bool setVolume(uint8_t volume);  // 0-100
    uint8_t getVolume() const;
    bool mute(bool isMuted);
    bool isMuted() const;
    
    // 状态查询
    AudioState getState() const;
    bool isPlaying() const;
    String getCurrentFile() const;
    AudioPlayMode getCurrentMode() const;
    
    // 音频文件管理
    bool loadAudioFile(const String& filePath);
    bool unloadAudioFile(const String& filePath);
    void unloadAllFiles();
    
    // 统计信息
    AudioStatistics getStatistics() const;
    void resetStatistics();
    
    // 音频设备控制
    bool enableAmplifier(bool enable);
    bool setMicrophoneGain(uint8_t gain);
    
    // 调试和监控
    void printStatus();
    String getStatusJSON();
    void setDebugMode(bool enabled);
    
    // 音频格式配置（用于匹配不同的PCM文件格式）
    bool setAudioFormat(uint32_t sampleRate, uint8_t channels, uint8_t bitsPerSample = 16);
    void printSupportedFormats();
    
    // 回调函数类型定义
    typedef void (*AudioCallback)(AudioState state, const String& filePath);
    void setCallback(AudioCallback callback);

private:
    // 初始化状态
    bool m_initialized;
    bool m_running;
    bool m_debugMode;
    
    // 外部依赖
    PSRAMManager* m_psramManager;
    FileManager* m_fileManager;
    
    // I2S和ES8311句柄
    i2s_chan_handle_t m_i2sTxHandle;
    i2s_chan_handle_t m_i2sRxHandle;
    es8311_handle_t m_es8311Handle;
    
    // 音频状态
    AudioState m_currentState;
    AudioPlayMode m_currentMode;
    String m_currentFile;
    uint8_t m_currentVolume;
    bool m_isMuted;
    
    // 音频数据管理
    AudioFileInfo m_currentAudioFile;
    size_t m_playPosition;
    
    // FreeRTOS任务和同步
    TaskHandle_t m_playTaskHandle;
    SemaphoreHandle_t m_mutex;
    QueueHandle_t m_audioQueue;
    
    // 统计信息
    AudioStatistics m_statistics;
    AudioCallback m_callback;
    
    // 内部初始化方法
    bool initGPIO();
    bool initI2S();
    bool initES8311();
    
    // 音频播放任务
    static void audioPlayTask(void* parameter);
    void audioPlayTaskImpl();
    
    // 音频数据处理
    bool loadPCMData(const String& filePath);
    void unloadPCMData();
    bool writeAudioData(const uint8_t* data, size_t size);
    
    // 内部控制方法
    void setState(AudioState newState);
    void updateStatistics(bool success, size_t bytesProcessed);
    void notifyCallback(AudioState state, const String& filePath);
    
    // 线程安全辅助
    bool takeMutex(TickType_t timeout = pdMS_TO_TICKS(1000));
    void giveMutex();
    
    // 工具方法
    String stateToString(AudioState state) const;
    String modeToString(AudioPlayMode mode) const;
    void logError(const String& message);
    void logInfo(const String& message);
};

#endif // AUDIOMANAGER_H 