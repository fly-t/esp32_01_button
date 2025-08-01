# 按钮单/双击/长按

## 使用链表结构实现

| 优势                   | 说明 |
| ---------------------- | ---- |
| ✅ 不再限制按键数量     |      |
| ✅ 动态添加按键，更灵活 |      |
| ✅ 结构清晰，易于维护   |      |



## 终端响应函数

IRAM_ATTR 是 ESP-IDF（ESP32/ESP8266）中用于 指定函数放入 IRAM（Instruction RAM，指令内存）区域 的一个属性宏。主要用于中断服务程序（ISR）等 对响应速度有极高要求的代码。

✅ 2. 用法示例

``` c
static void IRAM_ATTR gpio_isr_handler(void* arg)
{
    // 中断里只能做极少的事情：快速收集数据、发信号
}
```

这个中断函数就是 GPIO 电平变化时触发的函数，你必须用 IRAM_ATTR 标记它。



## 封装成组件调用

组件名就是 文件夹名 在应用层的cmake中需要添加 依赖的 组件名(文件夹名)

![](https://raw.githubusercontent.com/fly-t/images/main/blog/README-2025-08-01-22-21-17.png)

![](https://raw.githubusercontent.com/fly-t/images/main/blog/README-2025-08-01-22-21-48.png)


