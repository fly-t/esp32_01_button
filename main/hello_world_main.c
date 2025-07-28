#include <stdio.h>
#include <inttypes.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"

#include "esp_log.h"
static const char *TAG = "example";

static QueueHandle_t gpio_evt_queue = NULL;

static void IRAM_ATTR gpio_isr_handler(void *arg)
{
    uint32_t gpio_num = (uint32_t)arg;
    xQueueSendFromISR(gpio_evt_queue, &gpio_num, NULL);
}

static void gpio_task_example(void *arg)
{
    uint32_t io_num;
    for (;;)
    {
        if (xQueueReceive(gpio_evt_queue, &io_num, portMAX_DELAY))
        {
            switch (io_num)
            {
            case GPIO_NUM_0:

                // info 打印
                ESP_LOGI(TAG, "GPIO[%ld] intr, val: %d", io_num, gpio_get_level(io_num));
                break;

            default:
                break;
            }
        }
    }
}

#define CASE_KEY_IO GPIO_NUM_0

void app_main(void)
{
    // zero-initialize the config structure.
    gpio_config_t io_conf = {
        .intr_type = GPIO_INTR_NEGEDGE,
        .mode = GPIO_MODE_INPUT,
        .pin_bit_mask = 1ULL << CASE_KEY_IO,
        .pull_down_en = 0,
        .pull_up_en = 1};
    // configure GPIO with the given settings
    gpio_config(&io_conf);

    // create a queue to handle gpio event from isr
    gpio_evt_queue = xQueueCreate(10, sizeof(uint32_t));
    // start gpio task
    xTaskCreate(gpio_task_example, "gpio_task_example", 2048, NULL, 10, NULL);

    // install gpio isr service
    gpio_install_isr_service(0);
    // hook isr handler for specific gpio pin
    gpio_isr_handler_add(CASE_KEY_IO, gpio_isr_handler, (void *)CASE_KEY_IO);
}