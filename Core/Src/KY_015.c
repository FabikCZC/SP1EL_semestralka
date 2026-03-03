#include "KY_015.h"
#include "lcd128_32.h"
#include "stm32wb55xx.h"
#include "stm32wbxx_hal_gpio.h"
#include "stm32wbxx_hal_i2c.h"
#include "stm32wbxx_nucleo.h"

/**
 * @brief Blokující čekání v mikrosekundách pomocí časovače LPTIM2.
 * @note  Využívá rozdíl hodnot čítače. Díky přetypování na (uint16_t)
 * správně zvládne i situaci, kdy čítač přeteče (přejde z 65535 na 0).
 */
void Wait_us(uint16_t us)
{
    uint16_t start = LPTIM2->CNT;
    while ((uint16_t)(LPTIM2->CNT - start) < us)
        ;
}

/**
 * @brief Inicializace senzoru (zapnutí napájení a nastavení pinu do výchozího stavu).
 */
void KY_015_Init(void)
{
    HAL_GPIO_WritePin(KY_015_VDD_GPIO_Port, KY_015_VDD_Pin, 1);   // Zapnutí fyzického napájení senzoru (VDD)
    HAL_GPIO_WritePin(KY_015_DATA_GPIO_Port, KY_015_DATA_Pin, 1); // Datový pin držíme v klidovém stavu HIGH
    HAL_Delay(1000); // Senzor potřebuje po zapnutí napájení cca 1 vteřinu na stabilizaci
}

/**
 * @brief Odpojení napájení senzoru (užitečné pro bateriové aplikace před uspáním).
 */
void KY_015_DeInit(void)
{
    HAL_GPIO_WritePin(KY_015_VDD_GPIO_Port, KY_015_VDD_Pin, 0);
    HAL_GPIO_WritePin(KY_015_DATA_GPIO_Port, KY_015_DATA_Pin, 0);
}

/**
 * @brief Vyslání "Start" signálu ze strany mikrokontroléru.
 * @note  Podle datasheetu DHT11/KY-015 musí MCU stáhnout linku do LOW
 * na minimálně 18 ms, aby senzor detekoval požadavek o data.
 */
void KY_015_RequestData(void)
{
    HAL_GPIO_WritePin(KY_015_DATA_GPIO_Port, KY_015_DATA_Pin, 0); // Stáhnout linku dolů (LOW)
    Wait_us(18000);                                               // Zadržet v LOW na 18 ms (Startovací pulz)
    HAL_GPIO_WritePin(KY_015_DATA_GPIO_Port, KY_015_DATA_Pin,
                      1); // Uvolnit linku zpět do HIGH (přechod do režimu čtení)
}

/**
 * @brief Vyčtení 40 bitů (5 bajtů) odpovědi přímo ze senzoru.
 * @param data Ukazatel na pole o velikosti 5 bajtů, kam se uloží výsledek.
 */
void KY_015_ReadData(uint8_t *data)
{
    uint8_t i, j;
    uint16_t start_tick;
    uint16_t pulse_length;

    // Bezpečnostní vymazání bufferu před novým čtením
    for (i = 0; i < 5; i++)
        data[i] = 0;

    // --- FÁZE POTVRZENÍ (ACK) OD SENZORU ---
    uint32_t timeout = 10000;

    // 1. Čekáme, až senzor stáhne linku do LOW (potvrzení příjmu požadavku)
    while (HAL_GPIO_ReadPin(KY_015_DATA_GPIO_Port, KY_015_DATA_Pin) == 1)
    {
        if (--timeout == 0)
            return; // Ochrana proti zaseknutí (zabrání zamrznutí programu, pokud senzor chybí)
    }

    // 2. Senzor drží linku LOW po dobu 80us. Čekáme, až ji pustí.
    timeout = 10000;
    while (HAL_GPIO_ReadPin(KY_015_DATA_GPIO_Port, KY_015_DATA_Pin) == 0)
    {
        if (--timeout == 0)
            return;
    }

    // 3. Senzor drží linku HIGH po dobu 80us (příprava na odeslání dat). Čekáme na konec.
    timeout = 10000;
    while (HAL_GPIO_ReadPin(KY_015_DATA_GPIO_Port, KY_015_DATA_Pin) == 1)
    {
        if (--timeout == 0)
            return;
    }

    // --- FÁZE ČTENÍ SAMOTNÝCH DAT (40 bitů) ---
    for (i = 0; i < 5; i++) // Smyčka pro 5 bajtů
    {
        for (j = 0; j < 8; j++) // Smyčka pro 8 bitů v každém bajtu
        {
            // a) Každý bit začíná 50us LOW signálem. Čekáme na jeho konec (přechod do HIGH).
            timeout = 10000;
            while (HAL_GPIO_ReadPin(KY_015_DATA_GPIO_Port, KY_015_DATA_Pin) == 0)
            {
                if (--timeout == 0)
                    return;
            }

            // b) Začíná datový HIGH pulz. Zaznamenáme si aktuální čas.
            start_tick = LPTIM2->CNT;

            // Čekáme, dokud HIGH pulz neskončí
            timeout = 10000;
            while (HAL_GPIO_ReadPin(KY_015_DATA_GPIO_Port, KY_015_DATA_Pin) == 1)
            {
                if (--timeout == 0)
                    return;
            }

            // Spočítáme reálnou délku trvání HIGH pulzu
            pulse_length = (uint16_t)(LPTIM2->CNT - start_tick);

            // c) Rozhodnutí o hodnotě bitu:
            // Krátký pulz (cca 26-28 us)  = logická 0
            // Dlouhý pulz (cca 70 us)     = logická 1
            // Pokud je pulz delší než 50 us (hraniční hodnota), zapsat logickou 1.
            if (pulse_length > 50)
            {
                // Bitový posun: (1 << (7 - j)) zapíše jedničku na správnou pozici v bajtu zleva doprava
                data[i] |= (1 << (7 - j));
            }
        }
    }
}

/**
 * @brief Hlavní aplikační funkce. Získá data, ověří je, vypočítá hodnoty a vypíše na LCD.
 */
void KY_015_GetData(void)
{
    BSP_LED_On(LED2); // Vizuální indikace probíhajícího měření

    uint8_t data[5] = {0};
    float humidity = 0.0;
    float temperature = 0.0;
    float dew_point = 0.0;
    char lcd_buffer[19];

    // --- KRITICKÁ SEKCE ZAČÁTEK ---
    // Časování sběrnice u DHT11 je extrémně citlivé na mikrosekundy.
    // Pokud by v tuto chvíli přišlo přerušení (např. od SysTicku nebo UARTu),
    // čtení by se zpozdilo a načetli bychom nesmyslná data. Proto přerušení vypínáme.
    __disable_irq();

    KY_015_RequestData();
    KY_015_ReadData(data);

    __enable_irq(); // Okamžitě zapneme přerušení zpět, aby fungoval zbytek systému
    // --- KRITICKÁ SEKCE KONEC ---

    // Ověření platnosti dat (Checksum)
    // Pátý bajt (data[4]) musí být přesným součtem prvních čtyř bajtů.
    // '& 0xFF' ořízne výsledek pouze na spodních 8 bitů (pro případ přetečení).
    if (data[4] == ((data[0] + data[1] + data[2] + data[3]) & 0xFF))
    {
        // Převod surových dat na fyzikální veličiny podle specifikace DHT11
        humidity = data[0] + data[1] * 0.1;
        temperature = data[2] + data[3] * 0.1;

        // Zjednodušený empirický výpočet rosného bodu
        dew_point = temperature - ((100 - humidity) / 5.0);
    }
    else
    {
        printf("Chyba checksumu! Data: %02X %02X %02X %02X | CRC: %02X\n", data[0], data[1], data[2], data[3], data[4]);
        return; // Zastavení funkce - nebudeme na displej vypisovat nesmysly
    }

    // --- VÝPIS NA DISPLEJ A DO KONZOLE ---
    LCD_Cursor(0, 0);
    LCD_Display(&hi2c3, "SP1EL semestralka");

    // Výpis teploty
    printf("Teplota: %.2f °C\n", temperature);
    // Využíváme speciální znak '`' (ASCII 96), který máme ve fontu displeje překreslený jako znak stupně
    snprintf(lcd_buffer, sizeof(lcd_buffer), "Teplota: %.2f `C", temperature);
    LCD_Cursor(1, 0);
    LCD_Display(&hi2c3, lcd_buffer);

    // Výpis vlhkosti
    printf("Vlhkost: %.2f %%\n", humidity);
    snprintf(lcd_buffer, sizeof(lcd_buffer), "Vlhkost: %.2f %%", humidity);
    LCD_Cursor(2, 0);
    LCD_Display(&hi2c3, lcd_buffer);

    // Výpis rosného bodu
    printf("Rosný bod: %.2f °C\n", dew_point);
    snprintf(lcd_buffer, sizeof(lcd_buffer), "Rosny bod:%.2f `C", dew_point);
    LCD_Cursor(3, 0);
    LCD_Display(&hi2c3, lcd_buffer);

    printf("--------------------------------------------\n");
    BSP_LED_Off(LED2); // Konec měření, zhasnout LED
}

/**
 * @brief Uspí procesor do hlubokého režimu STOP2 pro extrémní úsporu baterie.
 */
void enter_STOP2_mode(void)
{
    KY_015_DeInit(); // Odpojení napájení senzoru, aby nespotřebovával energii během spánku

    /*LCD_DeInit(&hi2c3);     // Vypnutí displeje a jeho napájení, aby nespotřebovával energii během spánku
    HAL_I2C_DeInit(&hi2c3); // De-inicializace I2C, aby nespotřebovávalo energii během spánku
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = GPIO_PIN_0 | GPIO_PIN_1;
    GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);*/

    HAL_SuspendTick(); // Vypne hlavní časovač (SysTick), jinak by nás probudil za 1 milisekundu

    // Samotný příkaz k uspání. Jádro se zde zastaví a čeká na přerušení (např. od LPTIM1).
    HAL_PWREx_EnterSTOP2Mode(PWR_STOPENTRY_WFI);

    // --- KÓD POKRAČUJE ZDE AŽ PO PROBUZENÍ ---

    SystemClock_Config(); // Ve STOP2 se vypínají hlavní hodiny (PLL), musíme je znovu nahodit
    HAL_ResumeTick();     // Znovu zapne SysTick, aby fungovaly funkce jako HAL_Delay()
    KY_015_Init();        // Znovu inicializujeme senzor, protože jsme odpojili jeho napájení

    // LCD_Init(&hi2c3); // Znovu inicializujeme displej, protože jsme odpojili jeho napájení
}