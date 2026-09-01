idf_component_register(
    SRCS "am2020dy.c"
    INCLUDE_DIRS "."
    REQUIRES i2c_manager driver
)