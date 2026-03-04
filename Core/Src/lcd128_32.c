#include "lcd128_32.h"
#include "main.h"
#include "stm32wb55xx.h"
#include "stm32wbxx_hal_gpio.h"
#include <stdio.h>
#include <string.h>

/** * Globální pole pro uchování aktuální pozice kurzoru.
 * lcd_cursor[0] = Řádek (y), rozsah 0 až 3.
 * lcd_cursor[1] = Sloupec (x), rozsah 0 až 17.
 */
static uint8_t lcd_cursor[2] = {0, 0};

/**
 * @brief Bitová mapa fontu 7x8 (šířka 7 pixelů, výška 8 pixelů).
 * Každý znak je definován jako 7 sloupců (bajtů). Jednička v bajtu rozsvítí pixel.
 */
static const uint8_t font_7x8[95][7] = {
    {0x00, 0x3E, 0x51, 0x49, 0x45, 0x3E, 0x00}, // 0= '0'
    {0x00, 0x00, 0x42, 0x7F, 0x40, 0x00, 0x00}, // 1= '1'
    {0x00, 0x62, 0x51, 0x49, 0x49, 0x46, 0x00}, // 2= '2'
    {0x00, 0x21, 0x41, 0x49, 0x4D, 0x33, 0x00}, // 3= '3'
    {0x00, 0x18, 0x14, 0x12, 0x7F, 0x10, 0x00}, // 4= '4'
    {0x00, 0x27, 0x45, 0x45, 0x45, 0x39, 0x00}, // 5= '5'
    {0x00, 0x3C, 0x4A, 0x49, 0x49, 0x31, 0x00}, // 6= '6'
    {0x00, 0x01, 0x71, 0x09, 0x05, 0x03, 0x00}, // 7= '7'
    {0x00, 0x36, 0x49, 0x49, 0x49, 0x36, 0x00}, // 8= '8'
    {0x00, 0x46, 0x49, 0x49, 0x29, 0x1E, 0x00}, // 9= '9'
    {0x00, 0x24, 0x54, 0x54, 0x38, 0x40, 0x00}, // 10= 'a'
    {0x00, 0x7F, 0x28, 0x44, 0x44, 0x38, 0x00}, // 11= 'b'
    {0x00, 0x38, 0x44, 0x44, 0x44, 0x08, 0x00}, // 12= 'c'
    {0x00, 0x38, 0x44, 0x44, 0x28, 0x7F, 0x00}, // 13= 'd'
    {0x00, 0x38, 0x54, 0x54, 0x54, 0x08, 0x00}, // 14= 'e'
    {0x00, 0x08, 0x7E, 0x09, 0x09, 0x02, 0x00}, // 15= 'f'
    {0x00, 0x98, 0xA4, 0xA4, 0xA4, 0x78, 0x00}, // 16= 'g'
    {0x00, 0x7F, 0x08, 0x04, 0x04, 0x78, 0x00}, // 17= 'h'
    {0x00, 0x00, 0x00, 0x79, 0x00, 0x00, 0x00}, // 18= 'i'
    {0x00, 0x00, 0x80, 0x88, 0x79, 0x00, 0x00}, // 19= 'j'
    {0x00, 0x7F, 0x10, 0x28, 0x44, 0x40, 0x00}, // 20= 'k'
    {0x00, 0x00, 0x41, 0x7F, 0x40, 0x00, 0x00}, // 21= 'l'
    {0x00, 0x78, 0x04, 0x78, 0x04, 0x78, 0x00}, // 22= 'm'
    {0x00, 0x04, 0x78, 0x04, 0x04, 0x78, 0x00}, // 23= 'n'
    {0x00, 0x38, 0x44, 0x44, 0x44, 0x38, 0x00}, // 24= 'o'
    {0x00, 0xFC, 0x24, 0x24, 0x24, 0x18, 0x00}, // 25= 'p'
    {0x00, 0x18, 0x24, 0x24, 0x24, 0xFC, 0x00}, // 26= 'q'
    {0x00, 0x04, 0x78, 0x04, 0x04, 0x08, 0x00}, // 27= 'r'
    {0x00, 0x48, 0x54, 0x54, 0x54, 0x24, 0x00}, // 28= 's'
    {0x00, 0x04, 0x3F, 0x44, 0x44, 0x24, 0x00}, // 29= 't'
    {0x00, 0x3C, 0x40, 0x40, 0x3C, 0x40, 0x00}, // 30= 'u'
    {0x00, 0x1C, 0x20, 0x40, 0x20, 0x1C, 0x00}, // 31= 'v'
    {0x00, 0x3C, 0x40, 0x3C, 0x40, 0x3C, 0x00}, // 32= 'w'
    {0x00, 0x44, 0x28, 0x10, 0x28, 0x44, 0x00}, // 33= 'x'
    {0x00, 0x9C, 0xA0, 0xA0, 0x90, 0x7C, 0x00}, // 34= 'y'
    {0x00, 0x44, 0x64, 0x54, 0x4C, 0x44, 0x00}, // 35= 'z'
    {0x00, 0x7C, 0x12, 0x11, 0x12, 0x7C, 0x00}, // 36= 'A'
    {0x00, 0x7F, 0x49, 0x49, 0x49, 0x36, 0x00}, // 37= 'B'
    {0x00, 0x3E, 0x41, 0x41, 0x41, 0x22, 0x00}, // 38= 'C'
    {0x00, 0x7F, 0x41, 0x41, 0x41, 0x3E, 0x00}, // 39= 'D'
    {0x00, 0x7F, 0x49, 0x49, 0x49, 0x41, 0x00}, // 40= 'E'
    {0x00, 0x7F, 0x09, 0x09, 0x09, 0x01, 0x00}, // 41= 'F'
    {0x00, 0x3E, 0x41, 0x51, 0x51, 0x72, 0x00}, // 42= 'G'
    {0x00, 0x7F, 0x08, 0x08, 0x08, 0x7F, 0x00}, // 43= 'H'
    {0x00, 0x00, 0x41, 0x7F, 0x41, 0x00, 0x00}, // 44= 'I'
    {0x00, 0x20, 0x40, 0x41, 0x3F, 0x01, 0x00}, // 45= 'J'
    {0x00, 0x7F, 0x08, 0x14, 0x22, 0x41, 0x00}, // 46= 'K'
    {0x00, 0x7F, 0x40, 0x40, 0x40, 0x40, 0x00}, // 47= 'L'
    {0x00, 0x7F, 0x02, 0x0C, 0x02, 0x7F, 0x00}, // 48= 'M'
    {0x00, 0x7F, 0x04, 0x08, 0x10, 0x7F, 0x00}, // 49= 'N'
    {0x00, 0x3E, 0x41, 0x41, 0x41, 0x3E, 0x00}, // 50= 'O'
    {0x00, 0x7F, 0x09, 0x09, 0x09, 0x06, 0x00}, // 51= 'P'
    {0x00, 0x3E, 0x41, 0x51, 0x21, 0x5E, 0x00}, // 52= 'Q'
    {0x00, 0x7F, 0x09, 0x19, 0x29, 0x46, 0x00}, // 53= 'R'
    {0x00, 0x26, 0x49, 0x49, 0x49, 0x32, 0x00}, // 54= 'S'
    {0x00, 0x01, 0x01, 0x7F, 0x01, 0x01, 0x00}, // 55= 'T'
    {0x00, 0x3F, 0x40, 0x40, 0x40, 0x3F, 0x00}, // 56= 'U'
    {0x00, 0x1F, 0x20, 0x40, 0x20, 0x1F, 0x00}, // 57= 'V'
    {0x00, 0x7F, 0x20, 0x18, 0x20, 0x7F, 0x00}, // 58= 'W'
    {0x00, 0x63, 0x14, 0x08, 0x14, 0x63, 0x00}, // 59= 'X'
    {0x00, 0x03, 0x04, 0x78, 0x04, 0x03, 0x00}, // 60= 'Y'
    {0x00, 0x61, 0x51, 0x49, 0x45, 0x43, 0x00}, // 61= 'Z'
    {0x00, 0x00, 0x00, 0x5F, 0x00, 0x00, 0x00}, // 62= '!'
    {0x00, 0x00, 0x07, 0x00, 0x07, 0x00, 0x00}, // 63= '"'
    {0x00, 0x14, 0x7F, 0x14, 0x7F, 0x14, 0x00}, // 64= '#'
    {0x00, 0x24, 0x2E, 0x7B, 0x2A, 0x12, 0x00}, // 65= '$'
    {0x00, 0x23, 0x13, 0x08, 0x64, 0x62, 0x00}, // 66= '%'
    {0x00, 0x36, 0x49, 0x56, 0x20, 0x50, 0x00}, // 67= '&'
    {0x00, 0x00, 0x04, 0x03, 0x01, 0x00, 0x00}, // 68= '''
    {0x00, 0x00, 0x1C, 0x22, 0x41, 0x00, 0x00}, // 69= '('
    {0x00, 0x00, 0x41, 0x22, 0x1C, 0x00, 0x00}, // 70= ')'
    {0x00, 0x22, 0x14, 0x7F, 0x14, 0x22, 0x00}, // 71= '*'
    {0x00, 0x08, 0x08, 0x7F, 0x08, 0x08, 0x00}, // 72= '+'
    {0x00, 0x40, 0x30, 0x10, 0x00, 0x00, 0x00}, // 73= ','
    {0x00, 0x08, 0x08, 0x08, 0x08, 0x08, 0x00}, // 74= '-'
    {0x00, 0x20, 0x10, 0x08, 0x04, 0x02, 0x00}, // 75= '/'
    {0x00, 0x00, 0x36, 0x36, 0x00, 0x00, 0x00}, // 76= ':'
    {0x00, 0x40, 0x36, 0x36, 0x00, 0x00, 0x00}, // 77= ';'
    {0x00, 0x08, 0x14, 0x22, 0x41, 0x00, 0x00}, // 78= '<'
    {0x00, 0x14, 0x14, 0x14, 0x14, 0x14, 0x00}, // 79= '='
    {0x00, 0x00, 0x41, 0x22, 0x14, 0x08, 0x00}, // 80= '>'
    {0x00, 0x02, 0x01, 0x59, 0x05, 0x02, 0x00}, // 81= '?'
    {0x00, 0x3E, 0x41, 0x5D, 0x55, 0x5E, 0x00}, // 82= '@'
    {0x00, 0x08, 0x36, 0x41, 0x00, 0x00, 0x00}, // 83= '{'
    {0x00, 0x00, 0x00, 0x77, 0x00, 0x00, 0x00}, // 84= '|'
    {0x00, 0x00, 0x00, 0x41, 0x36, 0x08, 0x00}, // 85= '}'
    {0x00, 0x08, 0x04, 0x08, 0x10, 0x08, 0x00}, // 86= '~'
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // 87= ' '
    {0x00, 0x00, 0x60, 0x60, 0x00, 0x00, 0x00}, // 88= '.'
    {0x00, 0x04, 0x02, 0x7F, 0x02, 0x04, 0x00}, // 89= '^'
    {0x00, 0x08, 0x1C, 0x2A, 0x08, 0x08, 0x00}, // 90= '_'
    {0x00, 0x06, 0x09, 0x09, 0x06, 0x00, 0x00}, // 91= '`' -> '°'
    {0x00, 0x7F, 0x7F, 0x41, 0x41, 0x00, 0x00}, // 92= '['
    {0x00, 0x02, 0x04, 0x08, 0x10, 0x20, 0x00}, // 93= '\'
    {0x00, 0x00, 0x41, 0x41, 0x7F, 0x7F, 0x00}  // 94= ']'
};

/**
 * @brief Odeslání konfiguračního příkazu do displeje.
 * @note Využívá HAL_I2C_Mem_Write. Parametr MemAddress = 0x00 říká řadiči displeje:
 * "Pozor, tento bajt je řídící příkaz (např. nastav kurzor, zapni displej), nekresli ho."
 */
static void WriteByte_command(I2C_HandleTypeDef *hi2c, uint8_t cmd)
{
    // MemAddress 0x00 = Co=0, A0=0 (příkaz)
    HAL_I2C_Mem_Write(hi2c, LCD_I2C_ADDR, 0x00, I2C_MEMADD_SIZE_8BIT, &cmd, 1, HAL_MAX_DELAY);
}

/**
 * @brief Odeslání grafických dat do displeje.
 * @note MemAddress = 0x40 říká řadiči: "Tento bajt jsou data k vykreslení (pixely)."
 * Displej je rovnou zapíše do své RAM paměti a automaticky posune interní kurzor doprava.
 */
static void WriteByte_dat(I2C_HandleTypeDef *hi2c, uint8_t dat)
{
    // MemAddress 0x40 = Co=0, A0=1 (data)
    HAL_I2C_Mem_Write(hi2c, LCD_I2C_ADDR, 0x40, I2C_MEMADD_SIZE_8BIT, &dat, 1, HAL_MAX_DELAY);
}

/**
 * @brief Vykreslí jeden znak z naší tabulky fontů.
 * @param num Index znaku v poli font_7x8 (odpovídá zhruba ASCII kódu minus nějaký offset).
 */
static void WriteFont(I2C_HandleTypeDef *hi2c, int num)
{
    for (int i = 0; i < 7; i++)
    {
        WriteByte_dat(hi2c, font_7x8[num][i]); // Pošle 7 vertikálních sloupců tvořících znak
    }
}

// ==========================================================
// Veřejné API funkce
// ==========================================================

/**
 * @brief Vymaže celý displej.
 * @note Paměť displeje je rozdělena na 4 horizontální pruhy ("Pages" / stránky).
 * Každá stránka je vysoká 8 pixelů a široká 128 pixelů (sloupců).
 */
void LCD_Clear(I2C_HandleTypeDef *hi2c)
{
    for (int x = 0; x < 4; x++) // Projdi všechny 4 řádky (stránky 0 až 3)
    {
        WriteByte_command(hi2c, 0xb0 + x); // Nastav adresu stránky (0xB0, 0xB1, 0xB2, 0xB3)
        WriteByte_command(hi2c, 0x10);     // Nastav horní část adresy sloupce na 0
        WriteByte_command(hi2c, 0x00);     // Nastav dolní část adresy sloupce na 0

        for (int i = 0; i < 128; i++) // Vymaž všech 128 sloupců v daném řádku zapsáním nul
        {
            WriteByte_dat(hi2c, 0x00);
        }
    }
}

/**
 * @brief Inicializační sekvence pro řadič displeje (typicky ST7565/ST7567).
 */
void LCD_Init(I2C_HandleTypeDef *hi2c)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = LCD_VDD_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    // 1. Hardwarové zapnutí napájení displeje
    HAL_GPIO_WritePin(LCD_VDD_GPIO_Port, LCD_VDD_Pin, GPIO_PIN_SET);
    HAL_Delay(10);

    // 2. Softwarový reset řadiče
    WriteByte_command(hi2c, 0xe2);
    HAL_Delay(10);

    // 3. Konfigurace hardwarových parametrů skla
    WriteByte_command(hi2c, 0xa3); // Nastavení Biasu (poměr napětí)
    WriteByte_command(hi2c, 0xa0); // Směr skenování segmentů (zrcadlení X)
    WriteByte_command(hi2c, 0xc8); // Směr skenování COM vývodů (zrcadlení Y)
    WriteByte_command(hi2c, 0x22); // Nastavení interního regulátoru napětí
    WriteByte_command(hi2c, 0x81); // Začátek příkazu pro nastavení kontrastu
    WriteByte_command(hi2c, 0x30); // Samotná hodnota kontrastu

    // 4. Zapnutí napěťových pump
    WriteByte_command(hi2c, 0x2c); // Booster obvod
    WriteByte_command(hi2c, 0x2e); // Regulátor napětí
    WriteByte_command(hi2c, 0x2f); // Sledovač napětí

    LCD_Clear(hi2c); // Vymazat případný rozsypaný čaj z RAM

    // 5. Pokročilé nastavení a zapnutí
    WriteByte_command(hi2c, 0xff); // Vstup do rozšířené sady příkazů
    WriteByte_command(hi2c, 0x72);
    WriteByte_command(hi2c, 0xfe); // Návrat do standardní sady
    WriteByte_command(hi2c, 0xd6);
    WriteByte_command(hi2c, 0x90);
    WriteByte_command(hi2c, 0x9d);
    WriteByte_command(hi2c, 0xaf); // Hlavní příkaz: ZAPNOUT DISPLEJ (Display ON)
    WriteByte_command(hi2c, 0x40); // Nastavení startovní řádky RAM na 0
}

void LCD_DeInit(I2C_HandleTypeDef *hi2c)
{
    // 1. Vypnutí displeje
    WriteByte_command(hi2c, 0xae); // Příkaz pro vypnutí displeje (Display OFF)

    // 2. Hardwarové vypnutí napájení
    HAL_GPIO_WritePin(LCD_VDD_GPIO_Port, LCD_VDD_Pin, GPIO_PIN_RESET);

    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = GPIO_PIN_0 | GPIO_PIN_1;
    GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);
    GPIO_InitStruct.Pin = GPIO_PIN_1;
    GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
}

/**
 * @brief Uložení virtuální pozice kurzoru pro funkci LCD_Display.
 * @param y Řádek (0 až 3). Na displeji o výšce 32 pixelů se vejdou 4 řádky textu.
 * @param x Sloupec textu (0 až 17). Znak má na šířku 7 pixelů. 128 / 7 = 18 celých znaků.
 */
void LCD_Cursor(uint8_t y, uint8_t x)
{
    // Bezpečnostní oříznutí, aby kód nezapisoval mimo paměť displeje
    if (x > 17)
        x = 17;
    if (y > 3)
        y = 3;

    lcd_cursor[0] = y;
    lcd_cursor[1] = x;
}

/**
 * @brief Vykreslí textový řetězec na displej na aktuální pozici kurzoru.
 */
void LCD_Display(I2C_HandleTypeDef *hi2c, const char *str)
{
    int len = strlen(str);

    // 1. Nastavení kurzoru přímo v hardwaru displeje
    WriteByte_command(hi2c, 0xb0 + lcd_cursor[0]); // Určení stránky/řádku (0xB0 až 0xB3)

    // Výpočet pixelového sloupce (virtuální znaková pozice X vynásobená 7 pixely)
    // Protože adresa sloupce se posílá nadvakrát (horní a dolní 4 bity), musíme to rozdělit
    WriteByte_command(hi2c, 0x10 + (lcd_cursor[1] * 7 / 16)); // Horní polovina adresy sloupce
    WriteByte_command(hi2c, 0x00 + (lcd_cursor[1] * 7 % 16)); // Dolní polovina adresy sloupce

    // 2. Samotné vykreslování znaků
    for (int num = 0; num < len; num++)
    {
        switch (str[num])
        {
        case '0':
            WriteFont(hi2c, 0);
            break;
        case '1':
            WriteFont(hi2c, 1);
            break;
        case '2':
            WriteFont(hi2c, 2);
            break;
        case '3':
            WriteFont(hi2c, 3);
            break;
        case '4':
            WriteFont(hi2c, 4);
            break;
        case '5':
            WriteFont(hi2c, 5);
            break;
        case '6':
            WriteFont(hi2c, 6);
            break;
        case '7':
            WriteFont(hi2c, 7);
            break;
        case '8':
            WriteFont(hi2c, 8);
            break;
        case '9':
            WriteFont(hi2c, 9);
            break;
        case 'a':
            WriteFont(hi2c, 10);
            break;
        case 'b':
            WriteFont(hi2c, 11);
            break;
        case 'c':
            WriteFont(hi2c, 12);
            break;
        case 'd':
            WriteFont(hi2c, 13);
            break;
        case 'e':
            WriteFont(hi2c, 14);
            break;
        case 'f':
            WriteFont(hi2c, 15);
            break;
        case 'g':
            WriteFont(hi2c, 16);
            break;
        case 'h':
            WriteFont(hi2c, 17);
            break;
        case 'i':
            WriteFont(hi2c, 18);
            break;
        case 'j':
            WriteFont(hi2c, 19);
            break;
        case 'k':
            WriteFont(hi2c, 20);
            break;
        case 'l':
            WriteFont(hi2c, 21);
            break;
        case 'm':
            WriteFont(hi2c, 22);
            break;
        case 'n':
            WriteFont(hi2c, 23);
            break;
        case 'o':
            WriteFont(hi2c, 24);
            break;
        case 'p':
            WriteFont(hi2c, 25);
            break;
        case 'q':
            WriteFont(hi2c, 26);
            break;
        case 'r':
            WriteFont(hi2c, 27);
            break;
        case 's':
            WriteFont(hi2c, 28);
            break;
        case 't':
            WriteFont(hi2c, 29);
            break;
        case 'u':
            WriteFont(hi2c, 30);
            break;
        case 'v':
            WriteFont(hi2c, 31);
            break;
        case 'w':
            WriteFont(hi2c, 32);
            break;
        case 'x':
            WriteFont(hi2c, 33);
            break;
        case 'y':
            WriteFont(hi2c, 34);
            break;
        case 'z':
            WriteFont(hi2c, 35);
            break;
        case 'A':
            WriteFont(hi2c, 36);
            break;
        case 'B':
            WriteFont(hi2c, 37);
            break;
        case 'C':
            WriteFont(hi2c, 38);
            break;
        case 'D':
            WriteFont(hi2c, 39);
            break;
        case 'E':
            WriteFont(hi2c, 40);
            break;
        case 'F':
            WriteFont(hi2c, 41);
            break;
        case 'G':
            WriteFont(hi2c, 42);
            break;
        case 'H':
            WriteFont(hi2c, 43);
            break;
        case 'I':
            WriteFont(hi2c, 44);
            break;
        case 'J':
            WriteFont(hi2c, 45);
            break;
        case 'K':
            WriteFont(hi2c, 46);
            break;
        case 'L':
            WriteFont(hi2c, 47);
            break;
        case 'M':
            WriteFont(hi2c, 48);
            break;
        case 'N':
            WriteFont(hi2c, 49);
            break;
        case 'O':
            WriteFont(hi2c, 50);
            break;
        case 'P':
            WriteFont(hi2c, 51);
            break;
        case 'Q':
            WriteFont(hi2c, 52);
            break;
        case 'R':
            WriteFont(hi2c, 53);
            break;
        case 'S':
            WriteFont(hi2c, 54);
            break;
        case 'T':
            WriteFont(hi2c, 55);
            break;
        case 'U':
            WriteFont(hi2c, 56);
            break;
        case 'V':
            WriteFont(hi2c, 57);
            break;
        case 'W':
            WriteFont(hi2c, 58);
            break;
        case 'X':
            WriteFont(hi2c, 59);
            break;
        case 'Y':
            WriteFont(hi2c, 60);
            break;
        case 'Z':
            WriteFont(hi2c, 61);
            break;
        case '!':
            WriteFont(hi2c, 62);
            break;
        case '"':
            WriteFont(hi2c, 63);
            break;
        case '#':
            WriteFont(hi2c, 64);
            break;
        case '$':
            WriteFont(hi2c, 65);
            break;
        case '%':
            WriteFont(hi2c, 66);
            break;
        case '&':
            WriteFont(hi2c, 67);
            break;
        case '\'':
            WriteFont(hi2c, 68);
            break;
        case '(':
            WriteFont(hi2c, 69);
            break;
        case ')':
            WriteFont(hi2c, 70);
            break;
        case '*':
            WriteFont(hi2c, 71);
            break;
        case '+':
            WriteFont(hi2c, 72);
            break;
        case ',':
            WriteFont(hi2c, 73);
            break;
        case '-':
            WriteFont(hi2c, 74);
            break;
        case '/':
            WriteFont(hi2c, 75);
            break;
        case ':':
            WriteFont(hi2c, 76);
            break;
        case ';':
            WriteFont(hi2c, 77);
            break;
        case '<':
            WriteFont(hi2c, 78);
            break;
        case '=':
            WriteFont(hi2c, 79);
            break;
        case '>':
            WriteFont(hi2c, 80);
            break;
        case '?':
            WriteFont(hi2c, 81);
            break;
        case '@':
            WriteFont(hi2c, 82);
            break;
        case '{':
            WriteFont(hi2c, 83);
            break;
        case '|':
            WriteFont(hi2c, 84);
            break;
        case '}':
            WriteFont(hi2c, 85);
            break;
        case '~':
            WriteFont(hi2c, 86);
            break;
        case ' ':
            WriteFont(hi2c, 87);
            break;
        case '.':
            WriteFont(hi2c, 88);
            break;
        case '^':
            WriteFont(hi2c, 89);
            break;
        case '_':
            WriteFont(hi2c, 90);
            break;
        case '`':
            WriteFont(hi2c, 91);
            break;
        case '[':
            WriteFont(hi2c, 92);
            break;
        case '\\':
            WriteFont(hi2c, 93);
            break;
        case ']':
            WriteFont(hi2c, 94);
            break;
        default:
            break;
        }
    }
}

/**
 * @brief Pomocná funkce pro vypsání celého čísla na displej.
 * @note Využívá knihovní funkci snprintf k převodu intu na text a poté volá LCD_Display.
 */
void LCD_DisplayNum(I2C_HandleTypeDef *hi2c, int num)
{
    char str[18]; // Textový buffer (18 znaků je maximum, co se vejde na jeden řádek displeje)
    snprintf(str, sizeof(str), "%d", num);
    LCD_Display(hi2c, str);
}