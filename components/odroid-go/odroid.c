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

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "odroid.h"

static SemaphoreHandle_t spiLock = NULL;


void odroid_system_init()
{
    // SPI
    spiLock = xSemaphoreCreateMutex();

    // LED
	gpio_set_direction(GPIO_NUM_2, GPIO_MODE_OUTPUT);
	gpio_set_level(GPIO_NUM_2, 0);
    
    // SD Card (needs to be before LCD)
    odroid_sdcard_init();

    // LCD
    spi_lcd_init();

    // NVS Flash
    // nvs_flash_init();

    // Input
    odroid_input_init();
    
    // Sound
    odroid_sound_init();
}


void odroid_system_led_set(int value)
{
    gpio_set_level(GPIO_NUM_2, value);
}


void odroid_system_sleep()
{

}


int odroid_system_battery_level()
{
    return 0;
}


void odroid_fatal_error(char *error)
{
    printf("Error: %s\n", error);
    spi_lcd_init(); // This is really only called if the error occurs in the SD card init
    spi_lcd_fb_disable(); // Send the error directly to the LCD
    spi_lcd_usePalette(false);
    spi_lcd_setFontColor(LCD_COLOR_RED);
    spi_lcd_clear();
    spi_lcd_print(0, 0, "A fatal error occurred :(");
    spi_lcd_print(0, 50, error);
    odroid_sound_deinit();
    exit(-1);
}


void inline odroid_spi_bus_acquire()
{
    xSemaphoreTake(spiLock, portMAX_DELAY);
}


void inline odroid_spi_bus_release()
{
    xSemaphoreGive(spiLock);
}


size_t odroid_mem_free(uint32_t caps)
{
  multi_heap_info_t info;
  heap_caps_get_info(&info, caps);
  return info.total_free_bytes;
}


uint32_t odroid_millis()
{
    return (uint32_t)(esp_timer_get_time() / 1000);
}


void odroid_delay(uint32_t ms)
{
    vTaskDelay(ms / portTICK_PERIOD_MS);
}


void odroid_usleep(uint32_t us)
{
    uint64_t timeout = esp_timer_get_time() + us;

    while (timeout > esp_timer_get_time())
    {
        asm volatile ("nop");
    }
}
