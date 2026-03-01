#ifndef _LCD128_32_H__
#define _LCD128_32_H__

#include "main.h"

// Definice adresy (Původní 0x3F posunuto o 1 bit doleva pro STM32 HAL)
#define LCD_I2C_ADDR (0x3F << 1)

// Makra příkazů
#define LCD_CMD_DISPLAY_ON 0xaf
#define LCD_CMD_DISPLAY_OFF 0xae
#define LCD_CMD_START_LINE 0x40
#define LCD_CMD_RESTART 0xe2
#define LCD_CMD_SEG 0xa0
#define LCD_CMD_INV_NORMAL 0xa6
#define LCD_CMD_AP_NORMAL 0xa4
#define LCD_CMD_BS 0xa3
#define LCD_CMD_COM 0xc8
#define LCD_CMD_POWER_CON1 0x2c
#define LCD_CMD_POWER_CON2 0x2e
#define LCD_CMD_POWER_CON3 0x2f
#define LCD_CMD_REG_RATIO 0x22
#define LCD_CMD_EV1 0x81
#define LCD_CMD_EV2 0x30
#define LCD_CMD_EXIT_EC 0xfe
#define LCD_CMD_ENTER_EC 0xff
#define LCD_CMD_DSM_ON 0x72
#define LCD_CMD_DSM_OFF 0x70
#define LCD_CMD_DT 0xd6
#define LCD_CMD_BA 0x90
#define LCD_CMD_FR 0x9d

// Prototypy funkcí
void LCD_Init(I2C_HandleTypeDef *hi2c);
void LCD_Clear(I2C_HandleTypeDef *hi2c);
void LCD_Cursor(uint8_t y, uint8_t x);
void LCD_Display(I2C_HandleTypeDef *hi2c, const char *str);
void LCD_DisplayNum(I2C_HandleTypeDef *hi2c, int num);

#endif /* _LCD128_32_H__ */