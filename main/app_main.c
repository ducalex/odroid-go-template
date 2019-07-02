#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include "esp_system.h"
#include "odroid.h"


void input_callback(odroid_input_state state)
{
	spi_lcd_clear();
	spi_lcd_setFontColor(LCD_COLOR_WHITE);
	spi_lcd_printf(0, 0, "Inputs:");
	spi_lcd_setFontColor(LCD_COLOR_GREEN);

	for (int i = 0; i < ODROID_INPUT_MAX; i++) {
		spi_lcd_print(i * 20, 30, state.values[i] ? "1" : "0");
	}
	spi_lcd_update();
}


void app_main()
{
	printf("###### Demo app for odroid-go-std-lib ######\n");
	odroid_system_init(false, false);
	odroid_input_set_callback(&input_callback);

	int time = 0;

	while(1) {
		spi_lcd_fb_disable();
		spi_lcd_clear();
		time = odroid_millis();
		for (int i = 0; i < 20; i++) {
			spi_lcd_setFont(font_Ubuntu16);
			spi_lcd_setFontColor(LCD_COLOR_WHITE);
			spi_lcd_print(0, 0, "Drawing with no frame buffer!");
			spi_lcd_setFontColor(LCD_COLOR_PINK);
			spi_lcd_setFont(font_DejaVuMono24);
			spi_lcd_printf(50, 50, "%d %d", odroid_mem_free(MALLOC_CAP_INTERNAL), odroid_mem_free(MALLOC_CAP_SPIRAM));
			spi_lcd_setFontColor(LCD_COLOR_BLACK);
			spi_lcd_fill(100, 100, 100, 100, LCD_COLOR_BLUE);
			spi_lcd_printf(100, 100, "%d ms\n%d", (odroid_millis() - time), i);
			time = odroid_millis();
		}

		spi_lcd_fb_enable();
		spi_lcd_clear();
		time = odroid_millis();
		for (int i = 0; i < 60; i++) {
			spi_lcd_setFont(font_DejaVu18);
			spi_lcd_setFontColor(LCD_COLOR_WHITE);
			spi_lcd_print(0, 0, "Drawing with a frame buffer!");
			spi_lcd_setFontColor(LCD_COLOR_PINK);
			spi_lcd_setFont(font_DejaVuMono24);
			spi_lcd_printf(50, 50, "%d %d", odroid_mem_free(MALLOC_CAP_INTERNAL), odroid_mem_free(MALLOC_CAP_SPIRAM));
			spi_lcd_setFontColor(LCD_COLOR_BLACK);
			spi_lcd_fill(100, 100, 100, 100, LCD_COLOR_PURPLE);
			spi_lcd_printf(100, 100, "%d ms\n%d", (odroid_millis() - time), i);
			time = odroid_millis();
			spi_lcd_fb_flush();
		}

		spi_lcd_clear();
		time = odroid_millis();
		for (int i = 0; i < 60; i++) {
			spi_lcd_setFont(font_DejaVu24);
			spi_lcd_setFontColor(LCD_COLOR_WHITE);
			spi_lcd_print(0, 0, "Hello! (NB)");
			spi_lcd_setFontColor(LCD_COLOR_PINK);
			spi_lcd_printf(50, 50, "%d %d", odroid_mem_free(MALLOC_CAP_INTERNAL), odroid_mem_free(MALLOC_CAP_SPIRAM));
			spi_lcd_setFontColor(LCD_COLOR_BLACK);
			spi_lcd_fill(100, 100, 100, 100, LCD_COLOR_ORANGE);
			spi_lcd_printf(100, 100, "%d ms\n%d", (odroid_millis() - time), i);
			time = odroid_millis();
			spi_lcd_update();
		}

		odroid_delay(1000);
	}
}
