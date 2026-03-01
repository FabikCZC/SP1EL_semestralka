#include "KY_015.h"
#include "lcd128_32.h"
#include "stm32wb55xx.h"

void Wait_us(uint16_t us) {
  uint16_t start = LPTIM2->CNT;
  // uint16_t duration = us;
  while ((uint16_t)(LPTIM2->CNT - start) < us)
    ;
}

void KY_015_Init(void) {
  HAL_GPIO_WritePin(KY_015_VDD_GPIO_Port, KY_015_VDD_Pin,
                    1); // Power on the sensor
  HAL_GPIO_WritePin(KY_015_DATA_GPIO_Port, KY_015_DATA_Pin,
                    1); // Set DATA pin high
  HAL_Delay(1000);      // Wait for sensor to stabilize
  printf("KY_015 inicialized\n");
}

void KY_015_DeInit(void) {
  HAL_GPIO_WritePin(KY_015_VDD_GPIO_Port, KY_015_VDD_Pin,
                    0); // Power off the sensor
}

void KY_015_RequestData(void) {
  printf("KY_015 want data\n");
  HAL_GPIO_WritePin(KY_015_DATA_GPIO_Port, KY_015_DATA_Pin,
                    0); // Pull DATA pin low
  Wait_us(18000);       // Hold for at least 18ms
  // HAL_Delay(18);
  HAL_GPIO_WritePin(KY_015_DATA_GPIO_Port, KY_015_DATA_Pin, 1); // DATA pin high
  // Wait_us(40); // Wait for 20-40us
}

void KY_015_ReadData(uint8_t *data) {
  uint8_t i, j;
  uint16_t start_tick;
  uint16_t pulse_length;

  // Vymazat buffer
  for (i = 0; i < 5; i++)
    data[i] = 0;

  // 1. Čekáme na odezvu (LOW)
  uint32_t timeout = 10000;
  while (HAL_GPIO_ReadPin(KY_015_DATA_GPIO_Port, KY_015_DATA_Pin) == 1) {
    if (--timeout == 0)
      return;
  }

  // 2. Čekáme, dokud je LOW (80us presence)
  timeout = 10000;
  while (HAL_GPIO_ReadPin(KY_015_DATA_GPIO_Port, KY_015_DATA_Pin) == 0) {
    if (--timeout == 0)
      return;
  }

  // 3. Čekáme, dokud je HIGH (80us presence)
  timeout = 10000;
  while (HAL_GPIO_ReadPin(KY_015_DATA_GPIO_Port, KY_015_DATA_Pin) == 1) {
    if (--timeout == 0)
      return;
  }

  // 4. Čteme 40 bitů
  for (i = 0; i < 5; i++) {
    for (j = 0; j < 8; j++) {
      // a) Čekáme na konec úvodního LOW (start bitu)
      timeout = 10000;
      while (HAL_GPIO_ReadPin(KY_015_DATA_GPIO_Port, KY_015_DATA_Pin) == 0) {
        if (--timeout == 0)
          return;
      }

      // b) Měříme délku HIGH pulzu pomocí LPTIM2
      start_tick = LPTIM2->CNT;

      // Čekáme, dokud neskončí HIGH
      timeout = 10000;
      while (HAL_GPIO_ReadPin(KY_015_DATA_GPIO_Port, KY_015_DATA_Pin) == 1) {
        if (--timeout == 0)
          return;
      }

      // Spočítáme délku pulzu
      pulse_length = (uint16_t)(LPTIM2->CNT - start_tick);

      // c) Rozhodnutí:
      // 1 tick timeru při 1MHz = 1 us.
      // Logická 0: 26-28us -> cca 26 - 28 ticků
      // Logická 1: 70us    -> cca 70 ticků
      // Práh dáme doprostřed: 50us -> cca 50 ticků

      if (pulse_length > 50) {
        data[i] |= (1 << (7 - j));
      }
    }
  }
}

void KY_015_GetData(void) {
  uint8_t data[5] = {0};
  float humidity = 0.0;
  float temperature = 0.0;
  float dew_point = 0.0;
  char lcd_buffer[19];

  // --- KRITICKÁ SEKCE ZAČÁTEK ---
  __disable_irq(); // Vypneme všechna přerušení

  KY_015_RequestData();
  KY_015_ReadData(data);

  __enable_irq(); // Zapneme přerušení zpět
  // --- KRITICKÁ SEKCE KONEC ---

  // Verify checksum
  // DHT11 checksum je součet prvních 4 bajtů (posledních 8 bitů součtu)
  if (data[4] == ((data[0] + data[1] + data[2] + data[3]) & 0xFF)) {
    // Pro DHT11 je formát obvykle:
    // data[0] = Vlhkost celá část
    // data[1] = Vlhkost desetinná (u DHT11 často 0)
    // data[2] = Teplota celá část
    // data[3] = Teplota desetinná (u DHT11 často 0)
    humidity = data[0] + data[1] * 0.1;
    temperature = data[2] + data[3] * 0.1;
    dew_point =
        temperature - ((100 - humidity) / 5.0); // Jednoduchý odhad rosného bodu
  } else {
    printf("Chyba checksumu! Data: %02X %02X %02X %02X | CRC: %02X\n", data[0],
           data[1], data[2], data[3], data[4]);
    return; // Nepokračujeme dál
  }

  LCD_Cursor(0, 0);
  LCD_Display(&hi2c3, "SP1EL semestralka");

  printf("Teplota: %.2f °C\n", temperature);
  snprintf(lcd_buffer, sizeof(lcd_buffer), "Teplota: %.2f `C", temperature);
  LCD_Cursor(1, 0);
  LCD_Display(&hi2c3, lcd_buffer);

  printf("Vlhkost: %.2f %%\n", humidity);
  snprintf(lcd_buffer, sizeof(lcd_buffer), "Vlhkost: %.2f %%", humidity);
  LCD_Cursor(2, 0);
  LCD_Display(&hi2c3, lcd_buffer);

  printf("Rosný bod: %.2f °C\n", dew_point);
  snprintf(lcd_buffer, sizeof(lcd_buffer), "Rosny bod:%.2f `C", dew_point);
  LCD_Cursor(3, 0);
  LCD_Display(&hi2c3, lcd_buffer);
  printf("--------------------------------------------\n");
}