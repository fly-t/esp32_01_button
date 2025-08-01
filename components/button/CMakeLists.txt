file(GLOB_RECURSE SOURCES "./*.c" )

idf_component_register(SRCS "${SOURCES}"
                       INCLUDE_DIRS "."
                       PRIV_INCLUDE_DIRS ""
                       PRIV_REQUIRES "driver"
                       REQUIRES "")

# SRCS: 源文件
# INCLUDE_DIRS: 对外暴露头文件路径
# REQUIRES driver: 本组件依赖 ESP-IDF 的 GPIO 驱动、定时器等（来自 driver 组件）