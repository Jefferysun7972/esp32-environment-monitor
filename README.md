# 🌡️ ESP32 Environment Monitor

[![ESP-IDF](https://img.shields.io/badge/ESP--IDF-v5.3.5-blue.svg)](https://docs.espressif.com/projects/esp-idf/en/latest/)
[![Platform](https://img.shields.io/badge/Platform-ESP32-green.svg)](https://www.espressif.com/en/products/socs/esp32)
[![License](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)
![Security](https://img.shields.io/badge/Security-✅%20Safe-brightgreen.svg)
![Version](https://img.shields.io/badge/Version-v1.4.0-blue.svg)
![Release Date](https://img.shields.io/badge/Release-2026--09--10-lightgrey.svg)
![Status](https://img.shields.io/badge/Status-Production%20Ready-success.svg)

**A multi-sensor environmental monitoring system based on ESP32**, integrating AM2020DY and SEN66/SEN68 sensors with ILI9341 TFT-LCD display, cloud data upload, and real-time visualization.

> ⚠️ **v1.4.0 重要更新**: 所有敏感信息已替换为占位符，代码可安全公开分享！详见 [安全说明](#-安全说明) 和 [配置指南](CONFIG.example.md)。

---

## 📑 目录

- [✨ Features](#-features)
  - [🆕 v1.4.0 新特性](#-v140-新特性)
  - [🌡️ Multi-Sensor Support](#-multi-sensor-support)
  - [🖥️ TFT-LCD Display](#-tft-lcd-display-ili9341-240×320)
  - [☁️ Cloud Integration](#-cloud-integration)
  - [📱 WeChat Mini Program](#-wechat-mini-program)
  - [🔔 Smart Alert System](#-smart-alert-system)
  - [💻 Architecture](#-architecture)
  - [📁 项目结构](#-项目结构)
- [🛠️ Hardware Requirements](#-hardware-requirements)
- [🔌 Wiring](#-wiring)
- [🚀 Quick Start](#-quick-start)
- [🖥️ Display Layouts](#-display-layouts)
- [⚙️ Configuration](#-configuration)
- [☁️ Cloud Setup Guide](#-cloud-setup-guide)
- [📱 WeChat Mini Program Setup](#-wechat-mini-program-setup)
- [🗺️ Roadmap](#-roadmap)
- [📋 更新日志 (Changelog)](#-更新日志-changelog)
- [🔒 安全说明](#-安全说明)
- [❓ 常见问题 (FAQ)](#-常见问题-faq)
- [🤝 贡献指南](#-贡献指南)
- [📄 License](#-license)
- [🙏 Acknowledgments](#-acknowledgments)

---

---

## ✨ Features

### 🆕 v1.4.0 新特性

- 🔒 **安全加固**: 代码已脱敏，所有凭证替换为占位符，可安全分享
- 📊 **统计摘要**: 历史曲线页面显示最小值、最大值、平均值
- 🎨 **UI优化**: 导航栏固定不随内容滚动，传感器名称智能截断
- 🌡️ **大气压修复**: 完整支持 UART 传感器的大气压数据
- 🛡️ **安全工具**: 自动化安全检查脚本，防止敏感信息泄露

### 🌡️ Multi-Sensor Support
- **AM2020DY**: Temperature, Humidity, PM1.0, PM2.5, PM10, TVOC, NO2, HCHO (I2C frame protocol, addr 0x28)
- **SEN66**: Temperature, Humidity, PM1.0, PM2.5, PM10, TVOC, NOx, CO2 (I2C, addr 0x6B)
- **SEN68**: Temperature, Humidity, PM1.0, PM2.5, PM10, TVOC, NOx, HCHO (I2C, addr 0x6B)
- **Auto-Detection**: SEN66/SEN68 identified via product name command; AM2020DY via command probe

### 🖥️ TFT-LCD Display (ILI9341, 240×320)
- **Dual Sensor Comparison**: Side-by-side AM2020DY vs SEN66/SEN68 readings
- **Single Sensor Views**: Optimized layouts for AM2020DY, SEN66, and SEN68 individually
- **Progress Bars**: Visual PM2.5 / CO2 / VOC level indicators
- **Real-time Update**: 20-second refresh interval
- **Title Bar**: App name + sensor names with "vs" comparison label

### ☁️ Cloud Integration
- **MQTT Upload**: Real-time data publishing to EMQX Cloud for mobile monitoring (MQTTX App)
- **InfluxDB Storage**: Direct HTTP write to InfluxDB Cloud for historical data storage (30-day retention)
- **Grafana Visualization**: Rich dashboards with multi-sensor comparison, correlation charts, and alerts
- **Data Architecture**: `ESP32 → MQTT → EMQX Cloud` + `ESP32 → HTTP → InfluxDB Cloud → Grafana Cloud`

### 📱 WeChat Mini Program
- **Real-time Dashboard**: Live sensor data from InfluxDB with auto-refresh
- **Historical Charts**: 1h / 6h / 24h time-range history curves with dual-sensor comparison
- **Multi-Metric Support**: Temperature, Humidity, PM1.0, PM2.5, PM10, TVOC, HCHO, NO₂, NOx
- **Data Table**: Time-grouped detail view showing both sensors side by side
- **Canvas 2D Rendering**: Hardware-accelerated charts with DPR-aware sharp rendering
- **Segment Control**: Toggle between metrics and time ranges instantly
- **Dark Mode**: Full dark theme support with smooth CSS transition animation
- **System Theme Follow**: Auto-detect system dark mode via `wx.onThemeChange`
- **Custom Accent Color**: 6 accent colors to personalize the UI
- **High Contrast Mode**: Accessibility mode for visually impaired users
- **Settings Page**: Theme, temperature unit (°C/°F), accent color, and system info

### 🔔 Smart Alert System
- **3-Color Level**: BLUE (Normal) → ORANGE (Warning) → RED (Danger)
- **Multi-Parameter Thresholds**: Temp, Humidity, PM1.0, PM2.5, CO2, NOx, TVOC, HCHO
- **LED Fast Flashing**: 200ms interval on GPIO2 when alert triggers
- **Status Bar**: Color-coded status display at screen bottom

### 💻 Architecture
- **Modular Design**: `sensor_config`, `sensor_detect`, `alert_manager`, `ui_display`, `i2c_manager`, `wifi_web`, `mqtt_cloud`, `influxdb_writer`
- **I2C Bus Manager**: Centralized I2C bus sharing for multi-device communication
- **FreeRTOS**: Task-based sensor reading, cloud upload, and display updates

### 📁 项目结构

```
esp32-environment-monitor/
├── 📂 main/                      # ESP32 主程序
│   ├── blink_example_main.c      # 入口文件，传感器初始化
│   ├── app_config.h              # 告警阈值配置
│   └── sensor_config.h           # 传感器读取间隔
├── 📂 components/                # 功能模块
│   ├── wifi_web/                 # WiFi 连接管理
│   ├── mqtt_cloud/               # MQTT 云端上传
│   ├── influxdb_writer/          # InfluxDB 数据写入
│   ├── sensor_detect/            # 传感器自动检测
│   ├── alert_manager/            # 智能告警系统
│   ├── ui_display/               # TFT-LCD 显示驱动
│   └── i2c_manager/              # I2C 总线管理
├── 📂 miniprogram/               # 微信小程序
│   ├── pages/
│   │   ├── index/                # 实时数据页面
│   │   ├── history/              # 历史曲线页面
│   │   └── settings/             # 设置页面
│   └── config/sensors.js         # 传感器配置
├── 📂 scripts/                   # 工具脚本
│   └── security-check.sh         # 安全检查脚本
├── 🔒 .gitignore                 # Git 忽略规则
├── 📖 CONFIG.example.md          # 配置指南
├── 🛡️ SECURITY.md                # 安全说明文档
└── 📄 README.md                  # 项目说明（本文件）
```

---

## 🛠️ Hardware Requirements

| Component | Specification | Qty | Description |
|-----------|--------------|-----|-------------|
| **Main Controller** | ESP32 DevKit V1 | 1 | ESP-WROOM-32 |
| **Sensor A** | AM2020DY | 1 | Multi-parameter sensor (I2C frame protocol) |
| **Sensor B** | Sensirion SEN66 or SEN68 | 1 | Multi-parameter sensor (I2C, auto-detected) |
| **TFT Display** | ILI9341 240×320 SPI | 1 | 2.4" TFT LCD |
| **LED** | Red LED + 220Ω Resistor | 1 | Alert indicator on GPIO2 |
| **USB Cable** | Micro USB | 1 | Power and programming |

---

## 🔌 Wiring

### AM2020DY Sensor (I2C, addr 0x28)

| ESP32 Pin | AM2020DY Pin | Description |
|-----------|-------------|-------------|
| **3.3V** | VCC | Power |
| **GND** | GND | Ground |
| **GPIO21** | SDA | I2C Data |
| **GPIO22** | SCL | I2C Clock |

### SEN66/SEN68 Sensor (I2C, addr 0x6B)

| ESP32 Pin | SEN Pin | Description |
|-----------|---------|-------------|
| **3.3V** | VCC | Power |
| **GND** | GND | Ground |
| **GPIO21** | SDA | I2C Data (shared) |
| **GPIO22** | SCL | I2C Clock (shared) |

### ILI9341 TFT LCD (SPI)

| ESP32 Pin | TFT Pin | Description |
|-----------|---------|-------------|
| **3.3V** | VCC | Power |
| **GND** | GND | Ground |
| **GPIO18** | SCK | SPI Clock |
| **GPIO23** | MOSI | SPI MOSI |
| **GPIO5** | CS | Chip Select |
| **GPIO4** | DC | Data/Command |
| **GPIO16** | RST | Reset |
| **GPIO17** | BLK | Backlight |

### LED Alert

| ESP32 Pin | Component |
|-----------|-----------|
| **GPIO2** | → 220Ω Resistor → LED(+) → LED(-) → GND |

### Complete Pin Summary

| ESP32 Pin | Connected To |
|-----------|-------------|
| GPIO2 | LED (alert) |
| GPIO4 | TFT DC |
| GPIO5 | TFT CS |
| GPIO16 | TFT RST |
| GPIO17 | TFT BLK |
| GPIO18 | TFT SCK |
| GPIO21 | AM2020DY SDA + SEN SDA |
| GPIO22 | AM2020DY SCL + SEN SCL |
| GPIO23 | TFT MOSI |

---

## 🚀 Quick Start

### ⚠️ 安全提醒 (必读)

**在使用此代码前，请务必：**
1. 🔐 阅读 [安全说明](#-安全说明) 和 [配置指南](CONFIG.example.md)
2. 📝 替换所有 `YOUR_XXX` 占位符为您的实际凭证
3. ✅ 运行 `bash scripts/security-check.sh` 验证安全性

### Prerequisites
- ESP-IDF v5.3.5+
- Python 3.8+
- USB Driver (CP210x / CH340)

### Build & Flash

```bash
# 1. 克隆项目
git clone --recursive https://github.com/Jefferysun7972/esp32-environment-monitor.git
cd esp32-environment-monitor

# 2. ⚠️ 重要：配置您的凭证（替换占位符）
# 编辑以下文件：
#   - components/wifi_web/wifi_web.c (WiFi)
#   - components/mqtt_cloud/mqtt_cloud.c (MQTT)
#   - components/influxdb_writer/influxdb_writer.c (InfluxDB)
#   - miniprogram/app.js (小程序 InfluxDB)

# 3. 运行安全检查（推荐）
bash scripts/security-check.sh

# 4. 编译和烧录
source ~/esp/esp-idf/export.sh
idf.py set-target esp32
idf.py build
idf.py -p /dev/cu.usbserial-* flash monitor
```

---

## 🖥️ Display Layouts

The system supports multiple display modes selected automatically based on detected sensors:

### Dual I2C Comparison (AM2020DY + SEN66/SEN68)

```
┌──────────────────────────────────────────┐
│ ESP32  AM2020DY vs SEN68    @FELLOWES   │ ← Title Bar (font 1)
│ Environment Monitor                      │
│──────────────────────────────────────────│
│          AM2020          SEN66           │
│ Temp     xx.x            xx.x            │
│ Hum      xx.x            xx.x            │
│ PM1.0    xxx.x           xxx.x           │
│ PM2.5    xxx.x           xxx.x           │
│ PM10     xxx.x           xxx.x           │
│ TVOC     xxx.x           xxx.x           │
│ NOx      xxx.x           xxx.x           │
│──────────────────────────────────────────│
│ HCHO     xxx.x                           │
│ CO2                     xxx              │
├──────────────────────────────────────────┤
│              * NORMAL *                  │ ← Status Bar
└──────────────────────────────────────────┘
```

### Single Sensor Views
- **AM2020DY**: Full-screen with progress bars for all parameters
- **SEN66**: Includes CO2
- **SEN68**: Includes HCHO

---

## ⚙️ Configuration

### Sensor Thresholds (`main/app_config.h`)

| Parameter | Normal | Warning | Danger |
|-----------|--------|---------|--------|
| **Temperature** | 18–26°C | 10–35°C | <10 or >35°C |
| **Humidity** | 40–70% | 20–90% | <20 or >90% |
| **PM1.0** | <25 µg/m³ | <50 µg/m³ | ≥50 µg/m³ |
| **PM2.5** | <35 µg/m³ | <75 µg/m³ | ≥75 µg/m³ |
| **CO2** | <800 ppm | <1200 ppm | ≥1200 ppm |
| **NOx** | <100 | <200 | ≥200 |

### Sensor Read Interval (`main/sensor_config.h`)

```c
#define SENSOR_READ_PERIOD_MS  20000   // 20 seconds
```

### WiFi (`components/wifi_web/wifi_web.c`)

```c
#define WIFI_SSID      "your_wifi_ssid"
#define WIFI_PASS      "your_wifi_password"
#define WIFI_MAX_RETRY 5
```

### MQTT Cloud (`components/mqtt_cloud/mqtt_cloud.c`)

```c
#define MQTT_BROKER_URI  "mqtts://your-broker.emqxsl.cn:8883"
#define MQTT_USERNAME    "your_username"
#define MQTT_PASSWORD    "your_password"
```

### InfluxDB Cloud (`components/influxdb_writer/influxdb_writer.c`)

```c
#define INFLUXDB_URL    "https://your-region.cloud2.influxdata.com"
#define INFLUXDB_ORG    "your_org"
#define INFLUXDB_BUCKET "sensor_data"
#define INFLUXDB_TOKEN  "your_api_token"
```

### Display Selection (`main/blink_example_main.c`)

```c
#define USE_TFT_LCD      1    // ILI9341 TFT-LCD (active)
#define USE_OLED_DISPLAY 0    // SSD1306 OLED (disabled)
```

---

## ☁️ Cloud Setup Guide

### Prerequisites
- [EMQX Cloud](https://www.emqx.com/cloud) account (Free tier: 100 sessions)
- [InfluxDB Cloud](https://cloud2.influxdata.com) account (Free tier: 30-day retention, 5GB/month)
- [Grafana Cloud](https://grafana.com) account (Free tier: unlimited dashboards, 3 users)

### Data Flow Architecture

```
┌─────────────────────────────────────────────────────┐
│                      ESP32                          │
│  ┌──────────────┐        ┌──────────────────┐       │
│  │  mqtt_cloud   │        │ influxdb_writer   │       │
│  │  (MQTT)       │        │ (HTTP POST)       │       │
│  └──────┬───────┘        └────────┬─────────┘       │
└─────────┼─────────────────────────┼─────────────────┘
          │                         │
          ▼                         ▼
   ┌──────────────┐        ┌──────────────────┐
   │  EMQX Cloud   │        │  InfluxDB Cloud   │
   │  (消息中转)    │        │  (时序数据存储)    │
   └──────┬───────┘        └────────┬─────────┘
          │                         │
          ▼                         ▼
   ┌──────────────┐        ┌──────────────────┐
   │  MQTTX App    │        │  Grafana Cloud    │
   │  (实时查看)    │        │  (可视化仪表盘)    │
   └──────────────┘        └──────────────────┘
```

### Setup Steps
1. **EMQX Cloud**: Create a free Serverless deployment, get broker URI and credentials
2. **InfluxDB Cloud**: Create a Bucket (`sensor_data`), generate an API Token
3. **Grafana Cloud**: Add InfluxDB as data source, create dashboards using Flux queries
4. **ESP32**: Update credentials in `wifi_web.c`, `mqtt_cloud.c`, `influxdb_writer.c`

### Grafana Dashboard Tips
- **XY Chart**: Sensor correlation analysis (e.g., AM2020DY PM2.5 vs SEN68 PM2.5)
- **Stat Panels**: Real-time latest values display
- **Thresholds**: Color-coded alert levels on charts
- **Transformations**: Calculate deviation between two sensors
- **Annotations**: Mark test events directly on time-series charts

---

## 📱 WeChat Mini Program Setup

### Prerequisites
- [WeChat Developer Tools](https://developers.weixin.qq.com/miniprogram/dev/devtools/download.html)
- WeChat Mini Program AppID (register at [mp.weixin.qq.com](https://mp.weixin.qq.com))
- InfluxDB Cloud account (same as ESP32's bucket)

### Project Structure
```
miniprogram/
├── app.js              # App entry, InfluxDB API client, theme & settings management
├── app.json            # Page routes & tab bar config
├── app.wxss            # Global styles, theme transitions, dark mode
├── config/
│   └── sensors.js      # Sensor field labels, units, thresholds, fallback config
├── pages/
│   ├── index/          # Real-time dashboard
│   │   ├── index.js
│   │   ├── index.wxml
│   │   └── index.wxss
│   ├── history/        # Historical charts
│   │   ├── history.js
│   │   ├── history.wxml
│   │   └── history.wxss
│   └── settings/       # App settings
│       ├── settings.js
│       ├── settings.wxml
│       └── settings.wxss
└── utils/
```

### Configuration
1. Open `miniprogram/app.js` and update the InfluxDB credentials:
```javascript
const INFLUXDB_URL = 'https://your-region.cloud2.influxdata.com';
const INFLUXDB_ORG = 'your_org';
const INFLUXDB_BUCKET = 'sensor_data';
const INFLUXDB_TOKEN = 'your_api_token';
```

2. Open the project in WeChat Developer Tools
3. Replace the AppID in `project.config.json` with your own
4. Click "Preview" to test on your phone

### Pages
| Page | Route | Description |
|------|-------|-------------|
| **实时数据** | `pages/index/index` | Latest readings from both sensors, auto-refresh |
| **历史曲线** | `pages/history/history` | Time-series charts with metric/time-range selectors |
| **设置** | `pages/settings/settings` | Theme, temperature unit, accent color, system info |

### Historical Chart Features
- **Time Ranges**: 1 hour / 6 hours / 24 hours (InfluxDB aggregateWindow)
- **Metrics**: Temperature, Humidity, PM1.0, PM2.5, PM10, TVOC, HCHO, NO₂, NOx
- **Dual Curves**: AM2020DY (solid) vs SEN68 (dashed), with legend showing data point counts
- **Y-axis Unit Labels**: Auto-appended units (°C, %, µg/m³, ppb)
- **Data Table**: Grouped by time, showing both sensors' values per row
- **Touch Tooltip**: Crosshair and tooltip on touch interaction
- **Pull-to-Refresh**: Swipe down to refresh chart data
- **Temperature Unit Toggle**: Switch between °C and °F

---

## 🗺️ Roadmap

### v1.0.0 ✅
- LED alert, SEN66/SEN68 integration, auto-detection, TFT-LCD display, multi-sensor comparison

### v1.1.0 ✅
- WiFi connectivity, MQTT cloud upload (EMQX Cloud), InfluxDB time-series storage, Grafana visualization dashboards

### v1.2.0 ✅
- WeChat Mini Program: real-time dashboard, historical charts, dual-sensor comparison, Canvas 2D rendering

### v1.3.0 ✅
- Dark mode with smooth transition animation, system theme auto-follow, custom accent colors, high contrast accessibility mode, settings page, touch tooltip, pull-to-refresh, temperature unit toggle, performance optimization (async non-blocking startup)

### v1.4.0 ✅ (2026-09-10)
- **安全增强**: 所有敏感信息替换为占位符，添加 .gitignore 和安全检查脚本
- **UI优化**: 历史曲线页面导航栏固定、传感器名称智能截断（5字符）、图例右对齐
- **数据可视化**: 统计摘要（最小值/最大值/平均值）、多传感器图例动态宽度
- **大气压支持**: 修复 UART 传感器大气压数据识别问题，统一字段归一化
- **代码质量**: 添加 dependencies.lock、完善配置文档和安全说明

---

## 📋 更新日志 (Changelog)

### [v1.4.0] - 2026-09-10

#### 🔒 安全
- ✅ 替换所有硬编码凭证为占位符（WiFi、MQTT、InfluxDB）
- ✅ 添加 `.gitignore` 防止敏感文件提交
- ✅ 创建 `scripts/security-check.sh` 自动化安全检查
- ✅ 添加 `CONFIG.example.md` 配置指南
- ✅ 添加 `SECURITY.md` 安全说明文档

#### 🎨 UI/UX 改进
- ✅ 修复历史曲线页面导航栏随内容滚动问题（使用 fixed 定位 + cover-view）
- ✅ 移除导航栏底部圆角，改为直角设计
- ✅ 调整导航栏与内容区域间隙至最佳视觉比例
- ✅ 传感器名称智能截断（最多5个字符）
- ✅ 图例右对齐，动态计算宽度
- ✅ 优化图例项间距

#### 📊 数据可视化
- ✅ 添加统计摘要显示（最小值、最大值、平均值）
- ✅ 将统计摘要移至图表上方，避免被图表覆盖
- ✅ 多传感器图例动态宽度适配

#### 🌡️ 数据处理
- ✅ 修复 UART 传感器大气压数据识别（`pres` → `pressure` 归一化）
- ✅ InfluxDB 查询支持多字段匹配（`pressure`/`pres`/`press`）
- ✅ 统一所有传感器的大气压字段名为 `pressure`

#### 📝 文档
- ✅ 更新 README.md 添加安全说明和 v1.4.0 特性
- ✅ 添加常见问题解答（FAQ）
- ✅ 添加贡献指南
- ✅ 添加项目结构说明
- ✅ 更新快速开始指南，强调安全配置步骤

#### 🔧 代码质量
- ✅ 添加 `dependencies.lock` 锁定依赖版本
- ✅ 完善代码注释
- ✅ 优化文件组织结构

---

### [v1.3.0] - 2026-09-04
- ✅ Dark mode with smooth transition animation
- ✅ System theme auto-follow via `wx.onThemeChange`
- ✅ Custom accent colors (6 colors)
- ✅ High contrast accessibility mode
- ✅ Settings page with theme and unit options
- ✅ Touch tooltip on charts
- ✅ Pull-to-refresh functionality
- ✅ Temperature unit toggle (°C/°F)
- ✅ Performance optimization (async non-blocking startup)

---

### [v1.2.0] - 2026-09-03
- ✅ WeChat Mini Program: real-time dashboard
- ✅ Historical charts with time-range selection
- ✅ Dual-sensor comparison view
- ✅ Canvas 2D rendering with DPR support

---

### [v1.1.0] - 2026-09-02
- ✅ WiFi connectivity
- ✅ MQTT cloud upload (EMQX Cloud)
- ✅ InfluxDB time-series storage
- ✅ Grafana visualization dashboards

---

### [v1.0.0] - 2026-09-01
- ✅ LED alert system
- ✅ SEN66/SEN68 sensor integration
- ✅ Auto-detection of sensor types
- ✅ TFT-LCD display (ILI9341)
- ✅ Multi-sensor comparison view

---

## 🔒 安全说明

### ✅ 安全特性 (v1.4.0)

- **占位符替换**: WiFi、MQTT、InfluxDB 等所有敏感信息已替换为 `YOUR_XXX` 格式占位符
- **Git 忽略规则**: `.gitignore` 已配置，防止凭证文件、编译产物等被意外提交
- **安全检查脚本**: `scripts/security-check.sh` 可自动检测代码中的硬编码凭证
- **配置文档**: 详细的 [CONFIG.example.md](CONFIG.example.md) 指导用户安全配置

### 📁 敏感文件保护

以下文件类型已被 .gitignore 保护：
- 凭证文件: `.env`, `credentials.json`, `secrets.yaml`, `*.key`, `*.pem`
- 编译产物: `build/`, `managed_components/`, `sdkconfig.old`
- 依赖目录: `node_modules/`, `miniprogram_npm/`

### 🔧 使用前必读

**⚠️ 重要**: 在使用此代码前，**必须**完成以下步骤：

1. **阅读配置指南**: 查看 [CONFIG.example.md](CONFIG.example.md) 了解所有需要配置的项
2. **替换占位符**: 将代码中的 `YOUR_XXX` 占位符替换为您的实际凭证
3. **运行安全检查**: 执行 `bash scripts/security-check.sh` 验证无遗漏的敏感信息
4. **本地测试**: 先在本地环境测试，确认功能正常后再部署

### 🛡️ 安全最佳实践

- **永远不要**将包含真实凭证的代码提交到公共仓库
- **定期更换** API Token 和密码
- **使用环境变量**管理敏感信息（生产环境推荐）
- **启用** GitHub 的 Secret Scanning 功能
- **审查** Pull Request 中的代码变更

详见完整安全文档：[SECURITY.md](SECURITY.md)

---

## 📄 License

MIT License. See [LICENSE](LICENSE) for details.

---

## ❓ 常见问题 (FAQ)

### 🔐 安全相关

**Q: 代码中的 `YOUR_XXX` 是什么？**
A: 这些是占位符，需要替换为您的真实凭证。详见 [CONFIG.example.md](CONFIG.example.md)。

**Q: 如何检查代码是否安全？**
A: 运行 `bash scripts/security-check.sh`，它会自动检测硬编码的凭证。

**Q: 为什么我的凭证不能提交到 GitHub？**
A: 公开仓库中的凭证可能被恶意使用，导致数据泄露或产生费用。始终使用占位符或环境变量。

### 🛠️ 开发相关

**Q: 编译时提示找不到传感器驱动？**
A: 确保使用 `--recursive` 克隆项目：`git clone --recursive <url>`

**Q: WiFi 连接失败怎么办？**
A: 检查 `components/wifi_web/wifi_web.c` 中的 WiFi SSID 和密码是否正确。

**Q: InfluxDB 数据写入失败？**
A: 验证：
   - INFLUXDB_URL、ORG、BUCKET、TOKEN 是否正确
   - 网络连接是否正常
   - Token 是否有写入权限

**Q: 微信小程序无法获取数据？**
A: 检查 `miniprogram/app.js` 中的 InfluxDB 配置是否与 ESP32 端一致。

### 🎨 UI 相关

**Q: 历史曲线页面导航栏随内容滚动？**
A: 这是 v1.4.0 已修复的问题。如仍存在，请更新到最新代码。

**Q: 传感器名称显示不全？**
A: v1.4.0 已优化为智能截断（最多5个字符）和动态宽度图例。

**Q: 大气压数据不显示？**
A: v1.4.0 已修复 UART 传感器大气压字段识别问题。确保使用最新代码。

### ☁️ 云服务相关

**Q: 免费套餐够用吗？**
A: 对于个人项目通常足够：
- EMQX Cloud: 100 个连接/月
- InfluxDB Cloud: 30天保留，5GB/月
- Grafana Cloud: 无限仪表盘，3个用户

**Q: 如何备份数据？**
A: InfluxDB Cloud 自动保留30天。如需长期存储，可导出数据或升级套餐。

---

## 🤝 贡献指南

欢迎贡献代码、报告问题或提出建议！

### 🐛 报告 Bug

1. 检查 [Issues](https://github.com/Jefferysun7972/esp32-environment-monitor/issues) 是否已存在相同问题
2. 创建新 Issue 时，请包含：
   - 📌 硬件型号（ESP32 开发板、传感器型号）
   - 💻 ESP-IDF 版本
   - 📝 复现步骤
   - 📸 错误日志或截图

### 💡 功能建议

1. 先讨论：在 Issues 中描述您的想法
2. 等待维护者反馈
3. 提交 Pull Request 前请先阅读安全要求

### 🔧 提交代码

**⚠️ 安全要求（必须遵守）：**

1. **永远不要提交真实凭证**
   - 使用 `YOUR_XXX` 格式占位符
   - 运行 `bash scripts/security-check.sh` 验证

2. **代码规范**
   - 保持与现有代码风格一致
   - 添加必要的注释
   - 测试通过后再提交

3. **Pull Request 流程**
   - Fork 项目
   - 创建功能分支 (`git checkout -b feature/your-feature`)
   - 提交更改 (`git commit -m 'Add some feature'`)
   - 推送到分支 (`git push origin feature/your-feature`)
   - 创建 Pull Request

### 📋 开发检查清单

- [ ] 代码无硬编码凭证（运行安全检查）
- [ ] 新功能有相应文档更新
- [ ] 代码风格与项目一致
- [ ] 已在本地测试通过
- [ ] Commit message 清晰描述变更内容

---

## 🙏 Acknowledgments

- [Espressif](https://www.espressif.com/) - ESP32 & ESP-IDF
- [Sensirion](https://www.sensirion.com/) - SEN sensor drivers
- [FreeRTOS](https://www.freertos.org/) - RTOS

---

**Last Updated**: 2026-09-10 | **Maintainer**: [Jefferysun7972](https://github.com/Jefferysun7972) | **Version**: v1.4.0