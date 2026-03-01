#include "main.h"
#include <stdint.h>
#include <stdio.h>

// extern TIM_HandleTypeDef htim16;
extern I2C_HandleTypeDef hi2c3;
extern _Bool DHT11_read;

// uint32_t LPTIM1_period = 30000; // 30ms period for LPTIM1

void Wait_us(uint16_t us);
void KY_015_Init(void);
void KY_015_DeInit(void);
void KY_015_RequestData(void);
void KY_015_ReadData(uint8_t *data);
void KY_015_GetData(void);