/* 
 * This file is part of odroid-go-std-lib.
 * Copyright (c) 2019 ducalex.
 * 
 * This program is free software: you can redistribute it and/or modify  
 * it under the terms of the GNU General Public License as published by  
 * the Free Software Foundation, version 3.
 *
 * This program is distributed in the hope that it will be useful, but 
 * WITHOUT ANY WARRANTY; without even the implied warranty of 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU 
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License 
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef _SPI_LCD_H_
#define _SPI_LCD_H_

typedef struct {
      uint8_t charCode;
      int adjYOffset;
      int width;
      int height;
      int xOffset;
      int xDelta;
      uint16_t dataPtr;
} propFont;

#define MADCTL_MY 0x80
#define MADCTL_MX 0x40
#define MADCTL_MV 0x20
#define MADCTL_ML 0x10
#define MADCTL_MH 0x04
#define TFT_RGB_BGR 0x08
#define TFT_LED_DUTY_MAX 0x1fff
#define LCD_CMD 0
#define LCD_DATA 1

#define LCD_HTONS(a)      ((((a) << 8) | ((a) >> 8)) & 0xFFFF)
#define LCD_HEX(x)        (LCD_HTONS(x))
#define LCD_RGB(r, g, b)  (LCD_HTONS((((r / 255 * 31) & 0x1F) << 11) | (((g / 255 * 62) & 0x3F) << 5) | (((b / 255 * 31) & 0x1F))))

#define LCD_COLOR_BLACK                    LCD_HEX(0x0000)
#define LCD_COLOR_WHITE                    LCD_HEX(0xFFFF)
#define LCD_COLOR_RED                      LCD_HEX(0xF800)
#define LCD_COLOR_GREEN                    LCD_HEX(0x07E0)
#define LCD_COLOR_BLUE                     LCD_HEX(0x001F)
#define LCD_COLOR_ORANGE                   LCD_HEX(0xFD20)
#define LCD_COLOR_YELLOW                   LCD_HEX(0xFFE0)
#define LCD_COLOR_TEAL                     LCD_HEX(0x0410)
#define LCD_COLOR_CYAN                     LCD_HEX(0x07FF)
#define LCD_COLOR_CADET_BLUE               LCD_HEX(0x64F3)
#define LCD_COLOR_MIDNIGHT_BLUE            LCD_HEX(0x18CE)
#define LCD_COLOR_INDIGO                   LCD_HEX(0x4810)
#define LCD_COLOR_PURPLE                   LCD_HEX(0x8010)
#define LCD_COLOR_MAGENTA                  LCD_HEX(0xF81F)
#define LCD_COLOR_SNOW                     LCD_HEX(0xFFDE)
#define LCD_COLOR_DIM_GRAY                 LCD_HEX(0x6B4D)
#define LCD_COLOR_GRAY                     LCD_HEX(0x8410)
#define LCD_COLOR_DARK_GRAY                LCD_HEX(0xAD55)
#define LCD_COLOR_SILVER                   LCD_HEX(0xBDF7)
#define LCD_COLOR_LIGHT_GRAY               LCD_HEX(0xD69A)

#define LCD_DEFAULT_PALETTE NULL

short backlight_percentage_get(void);
void backlight_percentage_set(short level);

void spi_lcd_init();
void spi_lcd_deinit();
void spi_lcd_cmd(uint8_t cmd);
void spi_lcd_write8(uint8_t data);
void spi_lcd_write16(uint16_t data);
void spi_lcd_write(void *data, int len, int dc);
void spi_lcd_setWindow(uint16_t x, uint16_t y, uint16_t width, uint16_t height);
void spi_lcd_wait_finish();
void spi_lcd_usePalette(bool use);
void spi_lcd_setPalette(const int16_t *palette);
void spi_lcd_clear();
void spi_lcd_drawPixel(int x, int y, uint16_t color);
void spi_lcd_fill(int x, int y, int w, int h, uint16_t color);
void spi_lcd_setFont(const uint8_t *font);
void spi_lcd_setFontColor(uint16_t color);
void spi_lcd_print(int x, int y, char *string);
void spi_lcd_printf(int x, int y, char *string, ...);
void spi_lcd_update();
void spi_lcd_useFrameBuffer(bool use);
void spi_lcd_fb_setPtr(void *buffer);
void*spi_lcd_fb_getPtr();
void spi_lcd_fb_alloc();
void spi_lcd_fb_clear();
void spi_lcd_fb_flush(); // fb_flush sends the buffer to the display now, it's synchronous.
void spi_lcd_fb_update(); // fb_update tells the display task it's time to redraw, it's async. Could be named notify?
void spi_lcd_fb_write(void *buffer);

extern const unsigned char tft_Dejavu12[];
extern const unsigned char tft_Dejavu18[];
extern const unsigned char tft_Dejavu24[];

#endif