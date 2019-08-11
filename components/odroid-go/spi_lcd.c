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

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_heap_caps.h"
#include "soc/gpio_struct.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "driver/ledc.h"

#include "sdkconfig.h"
#include "odroid.h"

#define SCREEN_WIDTH 320
#define SCREEN_HEIGHT 240

#define NO_SIM_TRANS 5        //Amount of SPI transfers to queue in parallel
#define PIXELS_PER_TRANS SCREEN_WIDTH * 2 //in 16-bit words

static uint16_t windowWidth = SCREEN_WIDTH;
static uint16_t windowHeight = SCREEN_HEIGHT;
static uint16_t windowXOffset = 0;
static uint16_t windowYOffset = 0;

static spi_device_handle_t spi;
static spi_transaction_t spiTrans[NO_SIM_TRANS];
static TaskHandle_t *task = NULL;

static uint8_t backlightLevel = 75;
static bool lcd_initialized = false;

static void *currFbPtr = NULL;
static bool currFbPtrIsExternal = false;
static int  bufferSize = SCREEN_WIDTH * SCREEN_HEIGHT * 2;
static bool useFrameBuffer = false;

static int16_t defaultPalette[256] = {LCD_COLOR_BLACK, LCD_COLOR_WHITE, LCD_COLOR_RED, LCD_COLOR_GREEN, LCD_COLOR_BLUE};
static int16_t colorPalette[256];
static bool useColorPalette = false;

static const uint8_t *displayFont;
static fontCharacter_t fontChar;
static uint8_t font_height, font_width;
static bool enablePrintWrap = true;
static uint16_t fontColor = LCD_COLOR_WHITE;

SemaphoreHandle_t dispSem = NULL;
SemaphoreHandle_t fbLock = NULL;

// LCD initialization commands
typedef struct
{
    uint8_t cmd;
    uint8_t data[16];
    uint8_t databytes; //No of data in data; bit 7 = delay after set; 0xFF = end of cmds.
} ili_init_cmd_t;

DRAM_ATTR static ili_init_cmd_t ili_init_cmds[] = {
    // VCI=2.8V
    //************* Start Initial Sequence **********//
    {0x01, {0}, 0x80}, // Software reset
    {0xCF, {0x00, 0xc3, 0x30}, 3},
    {0xED, {0x64, 0x03, 0x12, 0x81}, 4},
    {0xE8, {0x85, 0x00, 0x78}, 3},
    {0xCB, {0x39, 0x2c, 0x00, 0x34, 0x02}, 5},
    {0xF7, {0x20}, 1},
    {0xEA, {0x00, 0x00}, 2},
    {0xC0, {0x1B}, 1},       //Power control   //VRH[5:0]
    {0xC1, {0x12}, 1},       //Power control   //SAP[2:0];BT[3:0]
    {0xC5, {0x32, 0x3C}, 2}, //VCM control
    {0xC7, {0x91}, 1},       //VCM control2
    {0x36, {(MADCTL_MV | MADCTL_MY | TFT_RGB_BGR)}, 1}, // Memory Access Control
    {0x3A, {0x55}, 1},
    {0xB1, {0x00, 0x1B}, 2}, // Frame Rate Control (1B=70, 1F=61, 10=119)
    {0xB6, {0x0A, 0xA2}, 2}, // Display Function Control
    {0xF6, {0x01, 0x30}, 2},
    {0xF2, {0x00}, 1}, // 3Gamma Function Disable
    {0x26, {0x01}, 1}, //Gamma curve selected

    //Set Gamma
    {0xE0, {0x0F, 0x31, 0x2B, 0x0C, 0x0E, 0x08, 0x4E, 0xF1, 0x37, 0x07, 0x10, 0x03, 0x0E, 0x09, 0x00}, 15},
    {0XE1, {0x00, 0x0E, 0x14, 0x03, 0x11, 0x07, 0x31, 0xC1, 0x48, 0x08, 0x0F, 0x0C, 0x31, 0x36, 0x0F}, 15},

    {0x11, {0}, 0x80}, //Exit Sleep
    {0x29, {0}, 0x80}, //Display on

    {0, {0}, 0xff}
};


static void backlight_init()
{
    //configure timer0
    ledc_timer_config_t ledc_timer;
    memset(&ledc_timer, 0, sizeof(ledc_timer));
    ledc_timer.duty_resolution = LEDC_TIMER_13_BIT; //set timer counter bit number
    ledc_timer.freq_hz = 5000;                      //set frequency of pwm
    ledc_timer.speed_mode = LEDC_LOW_SPEED_MODE;    //timer mode,
    ledc_timer.timer_num = LEDC_TIMER_0;            //timer index
    ledc_timer_config(&ledc_timer);

    //set the configuration
    ledc_channel_config_t ledc_channel;
    memset(&ledc_channel, 0, sizeof(ledc_channel));
    ledc_channel.channel = LEDC_CHANNEL_0;
    ledc_channel.duty = TFT_LED_DUTY_MAX;
    ledc_channel.gpio_num = PIN_NUM_LCD_BCKL;
    ledc_channel.intr_type = LEDC_INTR_FADE_END;
    ledc_channel.speed_mode = LEDC_LOW_SPEED_MODE;
    ledc_channel.timer_sel = LEDC_TIMER_0;
    ledc_channel_config(&ledc_channel);

    //initialize fade service.
    ledc_fade_func_install(0);

    backlight_percentage_set(backlightLevel);
}

void backlight_deinit()
{
    ledc_fade_func_uninstall();
    ledc_stop(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, 0);
}

short backlight_percentage_get(void)
{
    return backlightLevel;
}

void backlight_percentage_set(short level)
{
    if (level > 100) level = 100;
    if (level < 10) level = 10;

    if (odroid_nvs_handle && backlightLevel != level) {
        nvs_set_u8(odroid_nvs_handle, "screen_level", level);
        nvs_commit(odroid_nvs_handle);
    }

    backlightLevel = level;

    int duty = TFT_LED_DUTY_MAX * (backlightLevel * 0.01f);

    ledc_set_fade_with_time(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, duty, 1);
    ledc_fade_start(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, LEDC_FADE_WAIT_DONE /*LEDC_FADE_NO_WAIT*/);
}


void spi_lcd_write(void *data, int len, int dc)
{
    esp_err_t ret;
    spi_transaction_t t;
    if (len == 0) return;               //no need to send anything
    memset(&t, 0, sizeof(t));           //Zero out the transaction
    t.length = len * 8;                 //Command is 8 bits
    t.tx_buffer = data;                 //The data is the cmd itself
    t.user = (void *)dc;                //D/C needs to be set to 0
    ret = spi_device_transmit(spi, &t); //Transmit!
    assert(ret == ESP_OK);              //Should have had no issues.
}


static void spi_lcd_write8(uint8_t data)
{
    spi_lcd_write(&data, 1, 1);
}


static void spi_lcd_write16(uint16_t data)
{
    data = LCD_HTONS(data);
    spi_lcd_write(&data, 2, 1);
}


static void spi_lcd_cmd(uint8_t cmd)
{
    spi_lcd_write(&cmd, 1, 0);
}


static void spi_lcd_pre_transfer_callback(spi_transaction_t *t)
{
    gpio_set_level(PIN_NUM_DC, (int)t->user);
}


void spi_lcd_wait_finish()
{

}


void spi_lcd_setWindow(uint16_t x, uint16_t y, uint16_t width, uint16_t height)
{
    windowXOffset = x;
    windowYOffset = y;
    windowWidth = width;
    windowHeight = height;

    if (useFrameBuffer) {
        spi_lcd_fb_alloc();
    }
}


void spi_lcd_clip(int x0, int y0, int x1, int y1)
{
    spi_lcd_cmd(0x2A); //Column Address Set
    spi_lcd_write16(x0); //Start Col
    spi_lcd_write16(x1); //End Col

    spi_lcd_cmd(0x2B); // Page address
    spi_lcd_write16(y0); //Start page
    spi_lcd_write16(y1); //End page
}


void spi_lcd_fill(int x0, int y0, int w, int h, uint16_t color)
{
    // maybe we should htons the color and make LCD_RGB not do it?
    if (useFrameBuffer) {
        xSemaphoreTake(fbLock, portMAX_DELAY);

        for (int x = x0; x < (x0 + w); x++) {
            for (int y = y0; y < (y0 + h); y++) {
                if (useColorPalette) {
                    ((uint8_t *)currFbPtr)[(y  * windowWidth) + x] = color;
                } else {
                    ((uint16_t *)currFbPtr)[(y  * windowWidth) + x] = color;
                }
            }
        }

        xSemaphoreGive(fbLock);
    }
    else {
        odroid_spi_bus_acquire();

        spi_lcd_clip(x0, y0, x0 + w, y0 + h);
        spi_lcd_cmd(0x2C); // Write

        if (useColorPalette) {
            color = colorPalette[color];
        }

        uint16_t buffer[4] = {color, color, color, color};

        for(int pixels = w * h; pixels > 0; pixels -= 4) {
            spi_lcd_write(buffer, (pixels > 4 ? 4 : pixels) * 2, 1);
        }
        odroid_spi_bus_release();
    }
}


void inline spi_lcd_drawPixel(int x, int y, uint16_t color)
{
    spi_lcd_fill(x, y, 1, 1, color);
}


void spi_lcd_usePalette(bool use)
{
    useColorPalette = use;

    if (useFrameBuffer) {
        spi_lcd_fb_alloc(); // Resize framebuffer
    }
}


void spi_lcd_setPalette(const int16_t *palette)
{
    if (palette == NULL) { // Reset to default palette which is black,white,red,green,blue
        palette = defaultPalette;
    }
    memcpy(colorPalette, palette, sizeof(colorPalette));
}


void spi_lcd_clear()
{
    if (useFrameBuffer) {
        xSemaphoreTake(fbLock, portMAX_DELAY);
        memset(currFbPtr, 0, bufferSize);
        xSemaphoreGive(fbLock);
    } else {
        spi_lcd_fill(windowXOffset, windowYOffset, windowWidth, windowHeight, 0x0000);
    }
}


static uint8_t getCharPtr(uint8_t c)
{
    fontChar.dataPtr = 4;

    do {
        fontChar.charCode = displayFont[fontChar.dataPtr++];
        fontChar.adjYOffset = displayFont[fontChar.dataPtr++];
        fontChar.width = displayFont[fontChar.dataPtr++];
        fontChar.height = displayFont[fontChar.dataPtr++];
        fontChar.xOffset = displayFont[fontChar.dataPtr++];
        fontChar.xOffset = fontChar.xOffset < 0x80 ? fontChar.xOffset : -(0xFF - fontChar.xOffset);
        fontChar.xDelta = displayFont[fontChar.dataPtr++];

        if (c != fontChar.charCode && fontChar.charCode != 0xFF) {
            if (fontChar.width != 0) {
                // packed bits
                fontChar.dataPtr += (((fontChar.width * fontChar.height)-1) / 8) + 1;
            }
        }
    } while ((c != fontChar.charCode) && (fontChar.charCode != 0xFF));

    return (c == fontChar.charCode) ? 1 : 0;
}


static int spi_lcd_drawChar(int x, int y, uint8_t c)
{
    if (!getCharPtr(c)) {
        return 0;
    }

	uint8_t ch = 0, char_width = 0;
	int cx, cy;

	char_width = ((fontChar.width > fontChar.xDelta) ? fontChar.width : fontChar.xDelta);

	// draw Glyph
	uint8_t mask = 0x80;
	for (int j = 0; j < fontChar.height; j++) {
		for (int i = 0; i < fontChar.width; i++) {
			if (((i + (j*fontChar.width)) % 8) == 0) {
				mask = 0x80;
				ch = displayFont[fontChar.dataPtr++];
			}

			if ((ch & mask) !=0) {
				cx = (uint16_t)(x+fontChar.xOffset+i);
				cy = (uint16_t)(y+j+fontChar.adjYOffset);
                // Should render to a ~16x16 buffer then clip and flush, it would be a lot faster
				spi_lcd_drawPixel(cx, cy, fontColor);
			}
			mask >>= 1;
		}
	}

	return char_width;
}


void spi_lcd_setFont(const uint8_t *font)
{
    displayFont = font;

    //font_width = font[0];
    font_height = font[1];
    getCharPtr('@');
    font_width = fontChar.width;
}


void spi_lcd_setFontColor(uint16_t color)
{
    fontColor = color;
}


void spi_lcd_print(int x, int y, char *string)
{
    int orig_x = x;
    //int orig_y = y;

    for (int i = 0; i < strlen(string); i++) {
        if ((enablePrintWrap && x >= (windowWidth - font_width)) || string[i] == '\n') {
            y += font_height + 5;
            x = orig_x;
        }
        x += spi_lcd_drawChar(x, y, (uint8_t) string[i]);
    }

    //spi_lcd_fb_update();
}


void spi_lcd_printf(int x, int y, char *format, ...)
{
    char buffer[1024];
    va_list argptr;
    va_start(argptr, format);
    vsprintf(buffer, format, argptr);
    va_end(argptr);

    spi_lcd_print(x, y, buffer);
}


void spi_lcd_update()
{
    if (useFrameBuffer) {
        spi_lcd_fb_update();
    }
    else {
        // Do nothing
    }
}


void spi_lcd_flush()
{
    if (useFrameBuffer) {
        spi_lcd_fb_flush();
    }
    else {
        // Do nothing
    }
}


void spi_lcd_fb_enable()
{
    spi_lcd_fb_alloc();
    spi_lcd_fb_update();
    useFrameBuffer = true;
}


void spi_lcd_fb_disable()
{
    xSemaphoreTake(fbLock, portMAX_DELAY);
    if (!currFbPtrIsExternal)
        free(currFbPtr);
    currFbPtr = NULL;
    xSemaphoreGive(fbLock);
    useFrameBuffer = false;
}


void spi_lcd_fb_flush()
{
    if (!useFrameBuffer || !currFbPtr) return;

    odroid_spi_bus_acquire();

    int idx = 0;
    int inProgress = 0;

    spi_transaction_t *rtrans;
    esp_err_t ret;

    spi_lcd_clip(windowXOffset, windowYOffset, windowWidth + windowXOffset - 1, windowHeight + windowYOffset - 1);
    spi_lcd_cmd(0x2C); // Write

    xSemaphoreTake(fbLock, portMAX_DELAY);

    for (int x = 0; x < windowWidth * windowHeight; x += PIXELS_PER_TRANS)
    {
        if (useColorPalette) {
            for (int i = 0; i < PIXELS_PER_TRANS; i++)
            {
                ((uint8_t *)spiTrans[idx].tx_buffer)[i] = colorPalette[((uint8_t *)currFbPtr)[x + i]];
            }
        } else {
            memcpy(spiTrans[idx].tx_buffer, currFbPtr + (x * 2), PIXELS_PER_TRANS * sizeof(uint16_t));
        }

        spiTrans[idx].length = PIXELS_PER_TRANS * 16;
        ret = spi_device_queue_trans(spi, &spiTrans[idx], portMAX_DELAY);
        assert(ret == ESP_OK);

        idx++;
        if (idx >= NO_SIM_TRANS)
            idx = 0;

        if (inProgress == NO_SIM_TRANS - 1)
        {
            ret = spi_device_get_trans_result(spi, &rtrans, portMAX_DELAY);
            assert(ret == ESP_OK);
        }
        else
        {
            inProgress++;
        }
    }

    xSemaphoreGive(fbLock);

    while (inProgress)
    {
        ret = spi_device_get_trans_result(spi, &rtrans, portMAX_DELAY);
        assert(ret == ESP_OK);
        inProgress--;
    }

    odroid_spi_bus_release();
}


void spi_lcd_fb_setPtr(void *buffer)
{
    if (currFbPtr != NULL && !currFbPtrIsExternal) {
        free(currFbPtr);
    }
    currFbPtr = buffer;
    currFbPtrIsExternal = true;

    spi_lcd_fb_update();
}


void * spi_lcd_fb_getPtr()
{
    return currFbPtr;
}


void spi_lcd_fb_write(void *buffer)
{
    xSemaphoreTake(fbLock, portMAX_DELAY); // wait until previous frame is done sending
    memcpy(currFbPtr, buffer, bufferSize);
    xSemaphoreGive(fbLock);

    spi_lcd_fb_update();
}


void spi_lcd_fb_alloc()
{
    //if (currFbPtrIsExternal) {
    //    currFbPtrIsExternal = false;
    //    currFbPtr = NULL;
    //}
    xSemaphoreTake(fbLock, portMAX_DELAY);

    bufferSize = windowWidth * windowHeight * (useColorPalette ? 1 : 2);
    currFbPtr = heap_caps_realloc(currFbPtr, bufferSize, MALLOC_CAP_8BIT);
    memset(currFbPtr, 0, bufferSize);

    xSemaphoreGive(fbLock);
}


void spi_lcd_fb_update()
{
    xSemaphoreGive(dispSem);
}


void displayTask(void *arg)
{
    ESP_LOGI(__func__, "Display task started.");

    while (1)
    {
        xSemaphoreTake(dispSem, portMAX_DELAY);
        spi_lcd_fb_flush();
    }
}


void spi_lcd_deinit()
{
    spi_bus_remove_device(spi);
    backlight_deinit();
    gpio_reset_pin(PIN_NUM_LCD_CS);
    gpio_reset_pin(PIN_NUM_LCD_BCKL);
}


void spi_lcd_init(bool use_update_task)
{
    if (lcd_initialized) return;

    ESP_LOGI(__func__, "Initializing SPI LCD.");

    dispSem = xSemaphoreCreateBinary();
    fbLock = xSemaphoreCreateMutex();

    esp_err_t ret;

    spi_bus_config_t buscfg = {
        .miso_io_num = PIN_NUM_MISO,
        .mosi_io_num = PIN_NUM_MOSI,
        .sclk_io_num = PIN_NUM_CLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = (PIXELS_PER_TRANS * 2) + 16
    };

    spi_device_interface_config_t devcfg = {
        .clock_speed_hz = 40000000,              //Clock out at 40 MHz. Yes, that's heavily overclocked.
        .mode = 0,                               //SPI mode 0
        .spics_io_num = PIN_NUM_LCD_CS,              //CS pin
        .queue_size = NO_SIM_TRANS,              //We want to be able to queue this many transfers
        .pre_cb = spi_lcd_pre_transfer_callback, //Specify pre-transfer callback to handle D/C line
        .flags = SPI_DEVICE_NO_DUMMY
    };

    odroid_spi_bus_acquire();

    //Initialize the SPI bus
    ret = spi_bus_initialize(HSPI_HOST, &buscfg, SPI_DMA_CHANNEL); // DMA Channel
    assert(ret==ESP_OK || ret==ESP_ERR_INVALID_STATE);

    //Attach the LCD to the SPI bus
    ret = spi_bus_add_device(HSPI_HOST, &devcfg, &spi);
    assert(ret == ESP_OK);

    gpio_set_direction(PIN_NUM_DC, GPIO_MODE_OUTPUT);

    //Send the init commands
    int cmd = 0;
    while (ili_init_cmds[cmd].databytes != 0xff)
    {
        spi_lcd_cmd(ili_init_cmds[cmd].cmd);
        spi_lcd_write(ili_init_cmds[cmd].data, ili_init_cmds[cmd].databytes & 0x1F, 1);
        cmd++;
    }

    // Initialize SPI transactions with DMA buffers
    for (int x = 0; x < NO_SIM_TRANS; x++)
    {
        memset(&spiTrans[x], 0, sizeof(spi_transaction_t));
        spiTrans[x].length = PIXELS_PER_TRANS * 16;
        spiTrans[x].user = (void *)1;
        spiTrans[x].tx_buffer = heap_caps_malloc(PIXELS_PER_TRANS * 2, MALLOC_CAP_DMA);
        assert(spiTrans[x].tx_buffer);
    }

    odroid_spi_bus_release();

    spi_lcd_setFont(font_DejaVu18);
    spi_lcd_usePalette(false);
    spi_lcd_fb_enable();

    if (odroid_nvs_handle) {
        nvs_get_u8(odroid_nvs_handle, "screen_level", &backlightLevel);
    }

    //Enable backlight
    backlight_init();

    lcd_initialized = true;
    if (use_update_task) {
        xTaskCreatePinnedToCore(&displayTask, "display", 6000, NULL, 6, task, ODROID_TASKS_USE_CORE);
    }
}
