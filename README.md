# 🌡️ ESP32 Environment Monitor

[![ESP-IDF](https://img.shields.io/badge/ESP--IDF-v5.3.5-blue.svg)](https://docs.espressif.com/projects/esp-idf/en/latest/)
[![Platform](https://img.shields.io/badge/Platform-ESP32-green.svg)](https://www.espressif.com/en/products/socs/esp32)
[![License](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)

**A multi-sensor environmental monitoring system based on ESP32**, integrating AM2020DY and SEN66/SEN68 sensors with ILI9341 TFT-LCD display and real-time comparison.

---

## ✨ Features

### 🌡️ Multi-Sensor Support
- **AM2020DY**: Temperature, Humidity, PM1.0, PM2.5, PM10, TVOC, NO2, HCHO (I2C frame protocol, addr 0x28)
- **SEN66**: Temperature, Humidity, PM1.0, PM2.5, PM10, TVOC, NOx, CO2 (I2C, addr 0x6B)
- **SEN68**: Temperature, Humidity, PM1.0, PM2.5, PM10, TVOC, NOx, HCHO (I2C, addr 0x6B)
- **Auto-Detection**: SEN66/SEN68 identified via product name command; AM2020DY via command probe

### 🖥️ TFT-LCD Display (ILI9341, 240×320)
- **Dual Sensor Comparison**: Side-by-side AM2020DY vs SEN66/SEN68 readings
- **Single Sensor Views**: Optimized layouts for AM2020DY, SEN66, and SEN68 individually
- **Progress Bars**: Visual PM2.5 / CO2 / VOC level indicators
- **Real-time Update**: 5-second refresh interval
- **Title Bar**: App name + sensor names with "vs" comparison label

### 🔔 Smart Alert System
- **3-Color Level**: BLUE (Normal) → ORANGE (Warning) → RED (Danger)
- **Multi-Parameter Thresholds**: Temp, Humidity, PM1.0, PM2.5, CO2, NOx, TVOC, HCHO
- **LED Fast Flashing**: 200ms interval on GPIO2 when alert triggers
- **Status Bar**: Color-coded status display at screen bottom

### 💻 Architecture
- **Modular Design**: `sensor_config`, `sensor_detect`, `alert_manager`, `ui_display`, `i2c_manager`
- **I2C Bus Manager**: Centralized I2C bus sharing for multi-device communication
- **FreeRTOS**: Task-based sensor reading and display updates

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

### Prerequisites
- ESP-IDF v5.3.5+
- Python 3.8+
- USB Driver (CP210x / CH340)

### Build & Flash

```bash
git clone --recursive https://github.com/Jefferysun7972/esp32-environment-monitor.git
cd esp32-environment-monitor
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

### Display Selection (`main/blink_example_main.c`)

```c
#define USE_TFT_LCD      1    // ILI9341 TFT-LCD (active)
#define USE_OLED_DISPLAY 0    // SSD1306 OLED (disabled)
```

---

## 🗺️ Roadmap

### v1.0.0 ✅
- LED alert, SEN66/SEN68 integration, auto-detection, TFT-LCD display, multi-sensor comparison

### v1.1.0 (Planned)
- WiFi data upload, Web configuration, OTA updates

---

## 📄 License

MIT License. See [LICENSE](LICENSE) for details.

---

## 🙏 Acknowledgments

- [Espressif](https://www.espressif.com/) - ESP32 & ESP-IDF
- [Sensirion](https://www.sensirion.com/) - SEN sensor drivers
- [FreeRTOS](https://www.freertos.org/) - RTOS

---

**Last Updated**: 2026-09-02 | **Maintainer**: [Jefferysun7972](https://github.com/Jefferysun7972)