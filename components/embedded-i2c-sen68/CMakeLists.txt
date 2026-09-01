idf_component_register(
    SRCS
        "sen68_i2c.c"
        "sensirion_common.c"
        "sensirion_i2c.c"
        "sensirion_i2c_hal_custom.c"  # ← 自定义HAL层
    INCLUDE_DIRS
        "."
    REQUIRES 
        driver
        i2c_manager
)