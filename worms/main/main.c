#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <string.h>
#include "esp_log.h"
#include "driver/i2c.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "graphics.h"
#include "nvs_flash.h"

#define I2C_INTERFACE       I2C_NUM_0
#define I2C_MASTER_BITRATE  400000

#define WORM1_TASK_PRIORITY 9
#define WORM2_TASK_PRIORITY 9
#define WORM3_TASK_PRIORITY 9

#define WORM_LENGTH 7

static int worm1_x = 0;
static int worm2_x = 0;
static int worm3_x = 0;

#define WORM1_Y 0   
#define WORM2_Y 16  
#define WORM3_Y 32  

static const char* TAG = "worms";

static esp_err_t initI2C(i2c_port_t i2c_num) {
    ESP_LOGI(TAG, "init I2C");
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = GPIO_NUM_5,
        .scl_io_num = GPIO_NUM_6,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = I2C_MASTER_BITRATE
    };
    i2c_param_config(i2c_num, &conf);
    esp_err_t res = i2c_driver_install(i2c_num, conf.mode, 0, 0, 0);
    if (res != ESP_OK) {
        ESP_LOGE(TAG, "i2c_driver_install() FAILED: %d!", res);
    } else {
        ESP_LOGI(TAG, "i2c_driver_install() OK");
    }
    return res;
}

static void drawWorm(int x, int y, int length) {
    ESP_LOGI(TAG, "Drawing worm at x=%d, y=%d", x, y);
    graphics_startUpdate();
    for (int i = 0; i < length; i++) {
        if (x + i < graphics_getDisplayWidth()) {
            graphics_setPixel(x + i, y);
        }
    }
    graphics_finishUpdate();
    vTaskDelay(50 / portTICK_PERIOD_MS);
}

static void clearWorm(int x, int y, int length) {
    ESP_LOGI(TAG, "Clearing worm at x=%d, y=%d", x, y);
    graphics_startUpdate();
    for (int i = 0; i < length; i++) {
        if (x + i < graphics_getDisplayWidth()) {
            graphics_clearPixel(x + i, y);
        }
    }
    graphics_finishUpdate();
    vTaskDelay(50 / portTICK_PERIOD_MS);
}

void worm1_task(void *pvParameters) {
    while (1) {
        clearWorm(worm1_x, WORM1_Y, WORM_LENGTH);
        worm1_x++;
        if (worm1_x + WORM_LENGTH >= graphics_getDisplayWidth()) {
            worm1_x = 0;
        }
        drawWorm(worm1_x, WORM1_Y, WORM_LENGTH);
        ESP_LOGI(TAG, "Worm1 at x=%d", worm1_x);
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
}

void worm2_task(void *pvParameters) {
    while (1) {
        clearWorm(worm2_x, WORM2_Y, WORM_LENGTH);
        worm2_x++;
        if (worm2_x + WORM_LENGTH >= graphics_getDisplayWidth()) {
            worm2_x = 0;
        }
        drawWorm(worm2_x, WORM2_Y, WORM_LENGTH);
        ESP_LOGI(TAG, "Worm2 at x=%d", worm2_x);
        vTaskDelay(50 / portTICK_PERIOD_MS);
    }
}

void worm3_task(void *pvParameters) {
    while (1) {
        clearWorm(worm3_x, WORM3_Y, WORM_LENGTH);
        worm3_x++;
        if (worm3_x + WORM_LENGTH >= graphics_getDisplayWidth()) {
            worm3_x = 0;
        }
        drawWorm(worm3_x, WORM3_Y, WORM_LENGTH);
        ESP_LOGI(TAG, "Worm3 at x=%d", worm3_x);
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
}

void test_task(void *pvParameters) {
    while (1) {
        ESP_LOGI(TAG, "Test task running");
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}

void force_display_update(void *pvParameters) {
    while (1) {
        graphics_startUpdate();
        graphics_finishUpdate();
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
}

void app_main(void) {
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ESP_ERROR_CHECK(nvs_flash_init());
    }
    ESP_ERROR_CHECK(ret);

    ESP_ERROR_CHECK(initI2C(I2C_INTERFACE));

    esp_err_t display_ret = graphics_init(I2C_INTERFACE, CONFIG_GRAPHICS_PIXELWIDTH, CONFIG_GRAPHICS_PIXELHEIGHT, 0, true, false);
    if (display_ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize display: %d", display_ret);
    } else {
        ESP_LOGI(TAG, "Display initialized successfully");
    }

    graphics_startUpdate();
    graphics_clearScreen();
    graphics_println("Worms Test");
    graphics_setPixel(10, WORM1_Y);
    graphics_setPixel(10, WORM2_Y);
    graphics_setPixel(10, WORM3_Y);
    graphics_finishUpdate();

    vTaskDelay(2000 / portTICK_PERIOD_MS);

    graphics_startUpdate();
    graphics_clearScreen();
    graphics_finishUpdate();

    xTaskCreate(worm1_task, "worm1", 4096, NULL, WORM1_TASK_PRIORITY, NULL);
    xTaskCreate(worm2_task, "worm2", 4096, NULL, WORM2_TASK_PRIORITY, NULL);
    xTaskCreate(worm3_task, "worm3", 4096, NULL, WORM3_TASK_PRIORITY, NULL);
    xTaskCreate(force_display_update, "force_update", 2048, NULL, 5, NULL);

    ESP_LOGI(TAG, "Tasks created");
}