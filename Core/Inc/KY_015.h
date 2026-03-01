#include "main.h"
#include <stdint.h>
#include <stdio.h>

extern I2C_HandleTypeDef hi2c3;
extern void SystemClock_Config(void);

void Wait_us(uint16_t us);
void KY_015_Init(void);
void KY_015_DeInit(void);
void KY_015_RequestData(void);
void KY_015_ReadData(uint8_t *data);
void KY_015_GetData(void);
void enter_STOP2_mode(void);