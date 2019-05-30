#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>

#include "odroid.h"


void input_callback(odroid_input_state state)
{
	spi_lcd_clear();
	spi_lcd_setFontColor(LCD_RGB(255, 255, 255));
	spi_lcd_printf(0, 0, "Inputs:");
	spi_lcd_setFontColor(LCD_RGB(0, 255, 0));

	for (int i = 0; i < ODROID_INPUT_MAX; i++) {
		spi_lcd_print(i * 20, 30, state.values[i] ? "1" : "0");
	}
	spi_lcd_fb_update();
}

void app_main()
{
	printf("app_main(): Initializing Odroid GO stuff\n");
	odroid_system_init();
	spi_lcd_useFrameBuffer(true);
	spi_lcd_print(0, 0, "Hello World!");
	spi_lcd_fb_update();
	odroid_input_set_callback(&input_callback);
}
