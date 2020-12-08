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

#include "unistd.h"
#include "odroid.h"

SemaphoreHandle_t odroid_spi_lock = NULL;
nvs_handle odroid_nvs_handle;

void odroid_system_init(odroid_config_t *config)
{
    ESP_LOGI(__func__, "Initializing ODROID-GO hardware.");

    if (config == NULL) {
        odroid_config_t cfg = ODROID_DEFAULT_CONFIG();
        config = &cfg;
    }

    // NVS Flash
    if (config->init_nvs) {
        esp_err_t err = nvs_flash_init();
        if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
            nvs_flash_erase();
            nvs_flash_init();
        }
        nvs_open("odroid", NVS_READWRITE, &odroid_nvs_handle);
    }

    // SPI
    odroid_spi_lock = xSemaphoreCreateMutex();

    // LED
	gpio_set_direction(GPIO_NUM_2, GPIO_MODE_OUTPUT);
	gpio_set_level(GPIO_NUM_2, 0);

    // SD Card (needs to be before LCD) (optional)
    if (config->init_sdcard) {
        odroid_sdcard_init();
    }

    // LCD (always desirable)
    spi_lcd_init(config->use_lcd_task);

    // Input (always desirable)
    odroid_input_init(config->use_input_task);

    // Sound  (optional)
    if (config->init_sound) {
        odroid_sound_init();
    }
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
    ESP_LOGE(__func__, "Error: %s", error);
    spi_lcd_init(false); // This is really only called if the error occurs in the SD card init
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
    xSemaphoreTake(odroid_spi_lock, portMAX_DELAY);
}


void inline odroid_spi_bus_release()
{
    xSemaphoreGive(odroid_spi_lock);
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
    usleep(ms * 1000);
}
