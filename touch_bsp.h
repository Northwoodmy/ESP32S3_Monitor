#ifndef TOUCH_BSP_H
#define TOUCH_BSP_H

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

void Touch_Init(void);
uint8_t getTouch(uint16_t *x,uint16_t *y);

// I2C总线互斥锁相关函数
bool I2C_Lock(uint32_t timeout_ms);
void I2C_Unlock(void);
SemaphoreHandle_t I2C_GetMutex(void);

#ifdef __cplusplus
}
#endif

#endif
