// button_driver.h
#pragma once

#include "driver/gpio.h"



#ifdef __cplusplus
extern "C"
{
#endif

    /**
     * @brief 按键事件类型
     */
    typedef enum
    {
        BUTTON_SINGLE_CLICK,
        BUTTON_DOUBLE_CLICK,
        BUTTON_LONG_PRESS,
    } button_event_t;

    /**
     * @brief 按键回调函数类型
     *
     * @param event     当前触发的事件类型（单击/双击/长按）
     * @param gpio_num  对应的 GPIO 引脚号
     */
    typedef void (*button_callback_t)(button_event_t event, gpio_num_t gpio_num);

    /**
     * @brief 初始化按键驱动
     *
     * 初始化队列、中断服务和任务调度，必须先调用一次
     */
    void button_driver_init(void);

    /**
     * @brief 注册一个 GPIO 按键并绑定回调
     *
     * @param gpio_num  要监听的 GPIO 引脚（必须支持中断）
     * @param callback  回调函数，当有按键事件时调用
     */
    void button_register(gpio_num_t gpio_num, button_callback_t callback);

#ifdef __cplusplus
}
#endif