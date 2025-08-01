idf_component_register(SRCS "button_main.c "
                    PRIV_REQUIRES spi_flash esp_driver_gpio
                    INCLUDE_DIRS "."
                    REQUIRES button
                    )

# REQUIRES 填写组件命令, 组件名称就是components 目录下的 文件夹的名字

