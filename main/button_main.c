#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/timers.h"
#include "driver/gpio.h"
#include "esp_log.h"

#include "button_driver.h"

void button1_callback(button_event_t event, gpio_num_t gpio_num)
{
    switch (event)
    {
    case BUTTON_SINGLE_CLICK:
        ESP_LOGI("APP", "GPIO %d 单击", gpio_num);
        break;
    case BUTTON_DOUBLE_CLICK:
        ESP_LOGI("APP", "GPIO %d 双击", gpio_num);
        break;
    case BUTTON_LONG_PRESS:
        ESP_LOGI("APP", "GPIO %d 长按", gpio_num);
        break;
    }
}

/* 这个按钮不存在,只是示例 */
void button2_callback(button_event_t event, gpio_num_t gpio_num)
{
    switch (event)
    {
    case BUTTON_SINGLE_CLICK:
        ESP_LOGI("APP", "GPIO %d 单击", gpio_num);
        break;
    case BUTTON_DOUBLE_CLICK:
        ESP_LOGI("APP", "GPIO %d 双击", gpio_num);
        break;
    case BUTTON_LONG_PRESS:
        ESP_LOGI("APP", "GPIO %d 长按", gpio_num);
        break;
    }
}

void app_main(void)
{
    button_driver_init(); // 初始化驱动

    // 注册 GPIO0 按键，绑定回调
    button_register(GPIO_NUM_0, button1_callback);
    button_register(GPIO_NUM_10, button2_callback);
}
