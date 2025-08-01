// button_driver.c
#include "button_driver.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/timers.h"
#include "esp_log.h"
#include <string.h>

#define TAG "button_driver"
#define DOUBLE_CLICK_TIMEOUT pdMS_TO_TICKS(100)
#define LONG_PRESS_TIMEOUT pdMS_TO_TICKS(500)
#define DEBOUNCE_TIME_MS pdMS_TO_TICKS(20)

typedef enum
{
    EVENT_PRESS,
    EVENT_RELEASE,
} button_raw_event_t;

typedef struct button_t
{
    gpio_num_t gpio_num;            // GPIO 编号
    button_callback_t callback;     // 用户注册的回调
    TimerHandle_t click_timer;      // 单/双击判断定时器
    TimerHandle_t long_press_timer; // 长按定时器
    uint8_t click_count;            // 连击次数统计
    TickType_t last_press_tick;     // 上一次按下时间
    TickType_t last_event_tick;     // 记录上次事件时间，用于去抖
    struct button_t *next;          // 链表指针
} button_t;

static button_t *button_list_head = NULL;
static QueueHandle_t button_evt_queue;

/**
 * @brief 遍历查找
 * @param  gpio_num         GPIO 编号
 * @return * button_t*
 */
static button_t *find_button(gpio_num_t gpio_num)
{
    button_t *p = button_list_head;
    while (p)
    {
        if (p->gpio_num == gpio_num)
            return p;
        p = p->next;
    }
    return NULL;
}


/**
 * @brief GPIO 中断服务程序
 * @param  arg              中断服务程序参数
 * @return * void
 */
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
        if (btn->callback)
        {
            btn->callback(BUTTON_SINGLE_CLICK, btn->gpio_num);
        }
        else
        {
            ESP_LOGW(TAG, "Button click callback not set");
        }
    }
    else if (btn->click_count == 2)
    {
        if (btn->callback)
        {
            btn->callback(BUTTON_DOUBLE_CLICK, btn->gpio_num);
        }

        else
        {
            ESP_LOGW(TAG, "Button double callback not set");
        }
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

            TickType_t now = xTaskGetTickCount();
            if (now - btn->last_event_tick < DEBOUNCE_TIME_MS)
            {
                // 小于去抖时间，忽略此次事件
                continue;
            }
            btn->last_event_tick = now;

            if (evt == EVENT_PRESS)
            {
                btn->click_count++;
                btn->last_press_tick = now;
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

static bool isr_service_installed = false; // 确保只调用一次

void button_driver_init(void)
{
    if (!button_evt_queue)
    {
        button_evt_queue = xQueueCreate(16, sizeof(uint32_t));
        xTaskCreate(button_task, "button_task", 2048, NULL, 10, NULL);

        if (!isr_service_installed)
        {
            gpio_install_isr_service(0);
            isr_service_installed = true;
        }
    }
}

void button_register(gpio_num_t gpio_num, button_callback_t callback)
{
    if (find_button(gpio_num))
    {
        ESP_LOGW(TAG, "Button GPIO %d already registered", gpio_num);
        return;
    }

    button_t *btn = calloc(1, sizeof(button_t));
    if (!btn)
    {
        ESP_LOGE(TAG, "Failed to allocate memory for button");
        return;
    }
    btn->click_count = 0;
    btn->last_press_tick = 0;
    btn->last_event_tick = 0;

    btn->gpio_num = gpio_num;
    btn->callback = callback;

    // 初始化计时器
    btn->click_timer = xTimerCreate("click_timer", DOUBLE_CLICK_TIMEOUT, pdFALSE, btn, click_timer_cb);
    btn->long_press_timer = xTimerCreate("long_timer", LONG_PRESS_TIMEOUT, pdFALSE, btn, long_press_timer_cb);

    // 添加到链表头
    btn->next = button_list_head;
    button_list_head = btn;

    // 硬件配置
    gpio_config_t io_conf = {
        .intr_type = GPIO_INTR_ANYEDGE,
        .mode = GPIO_MODE_INPUT,
        .pin_bit_mask = 1ULL << gpio_num,
        .pull_down_en = 0,
        .pull_up_en = 1,
    };
    gpio_config(&io_conf);

    gpio_isr_handler_add(gpio_num, gpio_isr_handler, (void *)gpio_num);

    ESP_LOGI(TAG, "Button GPIO %d registered", gpio_num);
}
