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

#ifndef _ODROID_H_
#define _ODROID_H_

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>
#include <driver/gpio.h>
#include <nvs_flash.h>
#include <nvs.h>
#include <esp_heap_caps.h>
#include <esp_log.h>
#include "spi_lcd.h"
#include "sdcard.h"
#include "sound.h"
#include "input.h"

#define PIN_NUM_MISO 19
#define PIN_NUM_MOSI 23
#define PIN_NUM_CLK  18
#define PIN_NUM_DC 21
#define PIN_NUM_LCD_CS 5
#define PIN_NUM_SD_CS 22
#define PIN_NUM_LCD_BCKL 14
#define SPI_DMA_CHANNEL 2

#define ODROID_TASKS_USE_CORE 1

typedef struct
{
    bool init_sdcard;
    bool init_sound;
    bool init_nvs;
    bool use_input_task;
    bool use_lcd_task;
    int  cpu_core;
} odroid_config_t;

#define ODROID_DEFAULT_CONFIG() {\
    .init_sdcard = true, .init_sound = false, .init_nvs = false, \
    .use_input_task = true, .use_lcd_task = true, .cpu_core = 1}

void odroid_system_init(odroid_config_t *config);
void odroid_system_led_set(int value);
void odroid_spi_bus_acquire();
void odroid_spi_bus_release();
void odroid_fatal_error(char *error);
void odroid_delay(uint32_t ms);
size_t odroid_mem_free(uint32_t caps);
uint32_t odroid_millis();

extern SemaphoreHandle_t odroid_spi_lock;
extern nvs_handle odroid_nvs_handle;

// Deprecated
#define free_bytes_total() odroid_mem_free(MALLOC_CAP_DEFAULT)
#define free_bytes_internal() odroid_mem_free(MALLOC_CAP_INTERNAL)
#define free_bytes_spiram() odroid_mem_free(MALLOC_CAP_SPIRAM)

#endif