# The following five lines of boilerplate have to be in this order for cmake to work correctly
cmake_minimum_required(VERSION 3.16)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(blink)

# ===========================================
# 🔐 添加项目根目录到 include 路径
# 用途：允许所有组件访问 credentials.local.h
# ===========================================
# 这样 components/ 下的文件可以直接 #include "credentials.local.h"
# 而不需要使用相对路径如 "../../credentials.local.h"
include_directories(${CMAKE_SOURCE_DIR})