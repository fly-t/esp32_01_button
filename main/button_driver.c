// button_driver.c
#include "button_driver.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/timers.h"
#include "esp_log.h"
#include <string.h>

#define TAG "button_driver"
#define MAX_BUTTONS 10
#define DOUBLE_CLICK_TIMEOUT pdMS_TO_TICKS(100)
#define LONG_PRESS_TIMEOUT pdMS_TO_TICKS(500)

typedef enum
{
    EVENT_PRESS,
    EVENT_RELEASE,
} button_raw_event_t;

typedef struct
{
    gpio_num_t gpio_num;
    button_callback_t callback;
    TimerHandle_t click_timer;
    TimerHandle_t long_press_timer;
    uint8_t click_count;
    TickType_t last_press_tick;
} button_t;

static button_t buttons[MAX_BUTTONS];
static int button_count = 0;
static QueueHandle_t button_evt_queue;

static button_t *find_button(gpio_num_t gpio_num)
{
    for (int i = 0; i < button_count; i++)
    {
        if (buttons[i].gpio_num == gpio_num)
        {
            return &buttons[i];
        }
    }
    return NULL;
}

static void IRAM_ATTR gpio_isr_handler(void *arg)
{
    gpio_num_t gpio_num = (gpio_num_t)(uint32_t)arg;
    int level = gpio_get_level(gpio_num);
    button_raw_event_t evt = (level == 0) ? EVENT_PRESS : EVENT_RELEASE;
    uint32_t data = (gpio_num << 8) | evt;
    xQueueSendFromISR(button_evt_queue, &data, NULL);
}

static void click_timer_cb(TimerHandle_t xTimer)
{
    button_t *btn = (button_t *)pvTimerGetTimerID(xTimer);
    if (btn->click_count == 1)
    {
        btn->callback(BUTTON_SINGLE_CLICK, btn->gpio_num);
    }
    else if (btn->click_count == 2)
    {
        btn->callback(BUTTON_DOUBLE_CLICK, btn->gpio_num);
    }
    btn->click_count = 0;
}

static void long_press_timer_cb(TimerHandle_t xTimer)
{
    button_t *btn = (button_t *)pvTimerGetTimerID(xTimer);
    btn->callback(BUTTON_LONG_PRESS, btn->gpio_num);
    btn->click_count = 0;
    xTimerStop(btn->click_timer, 0);
}

static void button_task(void *arg)
{
    uint32_t data;
    for (;;)
    {
        if (xQueueReceive(button_evt_queue, &data, portMAX_DELAY))
        {
            gpio_num_t gpio = (data >> 8) & 0xFF;
            button_raw_event_t evt = data & 0xFF;
            button_t *btn = find_button(gpio);
            if (!btn)
                continue;

            if (evt == EVENT_PRESS)
            {
                btn->click_count++;
                btn->last_press_tick = xTaskGetTickCount();
                xTimerStart(btn->long_press_timer, 0);
            }
            else if (evt == EVENT_RELEASE)
            {
                xTimerStop(btn->long_press_timer, 0);
                xTimerStart(btn->click_timer, 0);
            }
        }
    }
}

void button_driver_init(void)
{
    if (!button_evt_queue)
    {
        button_evt_queue = xQueueCreate(16, sizeof(uint32_t));
        xTaskCreate(button_task, "button_task", 2048, NULL, 10, NULL);
        gpio_install_isr_service(0);
    }
}

void button_register(gpio_num_t gpio_num, button_callback_t callback)
{
    if (button_count >= MAX_BUTTONS)
    {
        ESP_LOGW(TAG, "Max button count reached");
        return;
    }

    gpio_config_t io_conf = {
        .intr_type = GPIO_INTR_ANYEDGE,
        .mode = GPIO_MODE_INPUT,
        .pin_bit_mask = 1ULL << gpio_num,
        .pull_down_en = 0,
        .pull_up_en = 1,
    };
    gpio_config(&io_conf);

    gpio_isr_handler_add(gpio_num, gpio_isr_handler, (void *)(uint32_t)gpio_num);

    button_t *btn = &buttons[button_count++];
    btn->gpio_num = gpio_num;
    btn->callback = callback;
    btn->click_count = 0;
    btn->last_press_tick = 0;
    btn->click_timer = xTimerCreate("click_timer", DOUBLE_CLICK_TIMEOUT, pdFALSE, btn, click_timer_cb);
    btn->long_press_timer = xTimerCreate("long_press_timer", LONG_PRESS_TIMEOUT, pdFALSE, btn, long_press_timer_cb);
}
