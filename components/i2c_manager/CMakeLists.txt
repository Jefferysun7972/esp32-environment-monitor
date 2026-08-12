# I2C Bus Manager Component
# Centralized I2C bus management for multi-device sharing
#
# Purpose:
# - Provides single point of control for I2C bus lifecycle
# - Decouples device drivers from bus initialization details
# - Ensures consistent I2C configuration across all components
#
# Dependencies:
# - driver: For i2c_master_* APIs

idf_component_register(
    SRCS "i2c_manager.c"
    INCLUDE_DIRS "."
    REQUIRES
        driver
)