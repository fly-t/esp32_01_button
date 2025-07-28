#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/timers.h"
#include "driver/gpio.h"
#include "esp_log.h"

static const char *TAG = "key_example";

#define CASE_KEY_IO GPIO_NUM_0
#define DOUBLE_CLICK_TIMEOUT pdMS_TO_TICKS(80)
#define LONG_PRESS_TIMEOUT pdMS_TO_TICKS(500)

typedef enum
{
    EVENT_PRESS,
    EVENT_RELEASE,
} key_event_t;

static QueueHandle_t gpio_evt_queue = NULL;
static TimerHandle_t click_timer = NULL;
static TimerHandle_t long_press_timer = NULL;

static uint8_t click_count = 0;

static void IRAM_ATTR gpio_isr_handler(void *arg)
{
    uint32_t gpio_num = (uint32_t)arg;
    int level = gpio_get_level(gpio_num);
    key_event_t evt = (level == 0) ? EVENT_PRESS : EVENT_RELEASE;  // 判断按下释放
    xQueueSendFromISR(gpio_evt_queue, &evt, NULL);
}

static void long_press_timer_cb(TimerHandle_t xTimer)
{
    ESP_LOGI(TAG, "⏱️ 长按检测");
    click_count = 0;
}

static void click_timer_cb(TimerHandle_t xTimer)
{
    if (click_count == 1)
    {
        ESP_LOGI(TAG, "✔️ 单击");
    }
    else if (click_count == 2)
    {
        ESP_LOGI(TAG, "🔁 双击");
    }
    click_count = 0;
}

static void gpio_task_example(void *arg)
{
    key_event_t evt;
    for (;;)
    {
        if (xQueueReceive(gpio_evt_queue, &evt, portMAX_DELAY))
        {
            switch (evt)
            {
            case EVENT_PRESS:
                click_count++;
                // 启动长按定时器
                xTimerStart(long_press_timer, 0);
                break;
            case EVENT_RELEASE:
                // 停止长按定时器
                xTimerStop(long_press_timer, 0);
                // 启动单击/双击检测定时器
                xTimerStart(click_timer, 0);
                break;
            }
        }
    }
}

void app_main(void)
{
    gpio_config_t io_conf = {
        .intr_type = GPIO_INTR_ANYEDGE,
        .mode = GPIO_MODE_INPUT,
        .pin_bit_mask = 1ULL << CASE_KEY_IO,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .pull_up_en = GPIO_PULLUP_ENABLE,
    };
    gpio_config(&io_conf);

    gpio_evt_queue = xQueueCreate(10, sizeof(key_event_t));

    click_timer = xTimerCreate("click_timer", DOUBLE_CLICK_TIMEOUT, pdFALSE, NULL, click_timer_cb);
    long_press_timer = xTimerCreate("long_press_timer", LONG_PRESS_TIMEOUT, pdFALSE, NULL, long_press_timer_cb);

    xTaskCreate(gpio_task_example, "gpio_task_example", 2048, NULL, 10, NULL);

    gpio_install_isr_service(0);
    gpio_isr_handler_add(CASE_KEY_IO, gpio_isr_handler, (void *)CASE_KEY_IO);
}
