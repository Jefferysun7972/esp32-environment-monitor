# 🌡️ ESP32 Environment Monitor

[![ESP-IDF](https://img.shields.io/badge/ESP--IDF-v5.3.5-blue.svg)](https://docs.espressif.com/projects/esp-idf/en/latest/)
[![Platform](https://img.shields.io/badge/Platform-ESP32-green.svg)](https://www.espressif.com/en/products/socs/esp32)
[![License](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)
[![Release](https://img.shields.io/badge/Release-v1.0.0-orange.svg)](https://github.com/Jefferysun7972/esp32-environment-monitor/releases)

**A multi-functional environmental monitoring system based on ESP32**, integrated with SEN66 environmental sensor and SSD1306 OLED display.

---

## ✨ Features

### 🌡️ Environmental Monitoring
- ✅ **Temperature**: Range -40°C ~ +70°C, Accuracy ±0.5°C
- ✅ **Humidity**: Range 0 ~ 100% RH, Accuracy ±3% RH
- ✅ **PM2.5 Concentration**: Real-time air quality monitoring
- ✅ **CO₂ Concentration**: Indoor air quality monitoring
- ✅ **VOC Index**: Volatile organic compounds monitoring
- ✅ **NOx Index**: Nitrogen oxides monitoring

### 🖥️ Visual Display
- ✅ **SSD1306 OLED**: 128×64 pixels yellow-blue dual-color display
- ✅ **Professional UI Layout**: Optimized dual-color partition display
- ✅ **Real-time Data Update**: Refresh sensor data every 5 seconds
- ✅ **Progress Bar Indicator**: Intuitive display of PM2.5 and CO₂ concentration
- ✅ **Warm-up Progress Interface**: Sensor startup countdown

### 🔔 Smart Alert System
- ✅ **PM2.5 Threshold**: Alert when > 75 µg/m³
- ✅ **CO₂ Threshold**: Alert when > 1000 ppm
- ✅ **LED Fast Flashing**: 200ms interval warning light
- ✅ **Screen Status Prompt**: "ALERT!" / "OK" status display

### 💻 Technical Highlights
- ✅ **Modular Design**: Component-based architecture, easy to extend
- ✅ **Custom Driver**: Bypass esp_lcd compatibility issues
- ✅ **I2C Bus Sharing**: Support multi-device simultaneous communication
- ✅ **FreeRTOS Task Management**: Real-time operating system support
- ✅ **Git Version Control**: Comprehensive branch management strategy

---

## 🛠️ Hardware Requirements

### Core Components

| Component | Model Specification | Quantity | Description |
|-----------|---------------------|----------|-------------|
| **Main Controller** | ESP32 DevKit V1 (ESP-WROOM-32) | 1 | Main controller |
| **Environmental Sensor** | Sensirion SEN66 | 1 | Multi-parameter environmental sensor |
| **OLED Display** | SSD1306 128×64 Yellow-Blue Dual-Color I2C | 1 | Data visualization |
| **LED Indicator** | Red LED + 220Ω Resistor | 1 | Alert indicator |
| **USB Cable** | Micro USB / USB-C | 1 | Power supply and programming |

---

## 🔌 Wiring Instructions

### Connection 1: ESP32 to SEN66 Sensor

| ESP32 Pin | SEN66 Pin | Description |
|-----------|-----------|-------------|
| **3.3V** | VCC | Power supply (3.3V) |
| **GND** | GND | Ground |
| **GPIO21** | SDA | I2C Data Line |
| **GPIO22** | SCL | I2C Clock Line |

**SEN66 I2C Address**: `0x69` (default)

---

### Connection 2: ESP32 to OLED Display (SSD1306)

| ESP32 Pin | OLED Pin | Description |
|-----------|----------|-------------|
| **3.3V** | VCC | Power supply (3.3V) |
| **GND** | GND | Ground |
| **GPIO21** | SDA | I2C Data Line (**shared with SEN66**) |
| **GPIO22** | SCL | I2C Clock Line (**shared with SEN66**) |

> ⚠️ **Important**: OLED and SEN66 share the same I2C bus!
>
> Both devices connect to the same GPIO21 (SDA) and GPIO22 (SCL) pins.

**OLED I2C Address**: 
- Default: `0x3C`
- Alternative: `0x3D` (change via onboard resistor)

---

### Connection 3: LED Alert Light

**Components needed:**
- 1× Red LED (or any color)
- 1× 220Ω Resistor
- 2× Jumper wires

**Wiring steps:**
1. Connect **GPIO2** → Resistor (one end)
2. Connect Resistor (other end) → **LED Anode (+)** (longer leg)
3. Connect **LED Cathode (-)** (shorter leg) → **GND**

**Summary table:**

| Component | Connection |
|-----------|------------|
| ESP32 GPIO2 | → 220Ω Resistor → LED(+) |
| LED(-) | → GND |

**Note**: LED is configured on **GPIO2 (D2 pin)**

---

### Complete Connection Summary

**All connections at a glance:**

| ESP32 Pin | Connected To | Function |
|-----------|--------------|----------|
| **3.3V** | SEN66(VCC), OLED(VCC) | Power for sensors & display |
| **GND** | SEN66(GND), OLED(GND), LED(-) | Common ground |
| **GPIO21** | SEN66(SDA), OLED(SDA) | I2C Data (shared bus) |
| **GPIO22** | SEN66(SCL), OLED(SCL) | I2C Clock (shared bus) |
| **GPIO2** | LED(via 220Ω resistor) | Alert indicator output |

**Key points:**
- ✅ Only **5 pins** used on ESP32
- ✅ I2C bus supports **multiple devices** (SEN66 + OLED)
- ✅ LED uses simple **digital output** mode

---

## 🚀 Quick Start

### Prerequisites

- ✅ **ESP-IDF v5.3.5** or higher
- ✅ **Python 3.8+** (for IDF toolchain)
- ✅ **Git** (for cloning repository and submodules)
- ✅ **USB Driver** (CP210x or CH340)
- ✅ **Serial Terminal Tool** (like screen, minicom, PuTTY)

### Installation Steps

#### 1️⃣ Clone the Project

```bash
git clone --recursive https://github.com/Jefferysun7972/esp32-environment-monitor.git cd esp32-environment-monitor git submodule update --init --recursive


Plain Text


#### 2️⃣ Configure ESP-IDF Environment

```bash
source ~/esp/esp-idf/export.sh idf.py --version

Expected output: v5.3.5

Plain Text


#### 3️⃣ Configure and Build

```bash
idf.py set-target esp32 idf.py fullclean idf.py build


Plain Text


#### 4️⃣ Flash Firmware

```bash
macOS Example:
idf.py -p /dev/cu.usbserial-110 flash monitor

Linux Example:
idf.py -p /dev/ttyUSB0 flash monitor

Windows Example:
idf.py -p COM3 flash monitor


Plain Text


---

## 🖥️ OLED Display Interface

### Display Layout Design

Optimized layout for **Yellow-Blue Dual-Color SSD1306 OLED**:

**Screen dimensions**: 128 pixels (wide) × 64 pixels (high)

**Layout structure (from top to bottom):**

| Row | Y-Position | Content | Color Zone |
|-----|-----------|---------|------------|
| **Border** | 0, 63 | Frame border (inset 1px) | - |
| **Margin** | 0-5 | Empty space | - |
| **Title** | **6-13** | Temperature & Humidity + Status | 🟨 **Yellow** |
| **Buffer** | 14-15 | Transition space | - |
| **PM2.5 Data** | **16-23** | PM2.5 value + unit | 🔵 **Blue** |
| **PM2.5 Bar** | **24-29** | Progress bar (6px height) | 🔵 **Blue** |
| **CO₂ Data** | **32-39** | CO₂ value + unit | 🔵 **Blue** |
n| **CO₂ Bar** | **40-45** | Progress bar (6px height) | 🔵 **Blue** |
| **VOC/NOx** | **50-57** | VOC and NOx values | 🔵 **Blue** |
| **Border** | 63 | Bottom frame line | - |

### Color Partition Strategy

| Area | Row Range | Usage | Color |
|------|-----------|-------|-------|
| **Margin Zone** | y = 0-5 | Top margin | - |
| **Title Zone** | y = 6-13 | Temperature + Humidity + Status | 🟨 **Yellow** |
| **Buffer Zone** | y = 14-15 | Yellow-blue transition | - |
| **Data Zone** | y = 16-63 | All sensor data | 🔵 **Blue** |

### Display Modes

#### 🟢 Normal Mode
T:25.3c H:55% OK 
PM2.5: 12.5 ug/m3 
[████░░░░░░░░░░░░░░░░░░░] 
CO2: 650 ppm 
[░░░░░░░░░░░░░░░░░░░░░░░░░] 
VOC:45 NOX:2


Plain Text


#### 🔴 Alert Mode
T:28.1c H:70% ALERT! 
PM2.5: 89.2 ug/m3 
[███████████████████████████] 
CO2: 1250 ppm 
[████████████████████████████] 
VOC:120 NOX:15


Plain Text

*(LED flashes rapidly simultaneously)*

#### ⏳ Warm-up Mode

Plain Text

 Warming up: 10s
┌──────────────────┐ 
│████████░░░░░░░░░░│ ← Thin progress bar (14px) 
└──────────────────┘ 67% 5/15 sec


Plain Text


---

## ⚙️ Configuration Parameters

### Sensor Thresholds (`main/blink_example_main.c`):

```c
#define PM25_ALERT_THRESHOLD 75 // µg/m³ #define CO2_ALERT_THRESHOLD 1000 // ppm


Plain Text


### I2C Configuration:

```c
#define I2C_MASTER_SCL_IO 22 // Clock pin #define I2C_MASTER_SDA_IO 21 // Data pin #define I2C_MASTER_FREQ_HZ 100000 // 100kHz


Plain Text


### OLED Special Configuration

Perfect parameters for your OLED module (built-in):

```c
// SSD1306 initialization commands oled_custom_send_command(dev, 0xA1); // Segment Re-map: fix horizontal mirror oled_custom_send_command(dev, 0xC8); // Scan Direction: fix vertical flip

// Font rendering bit operation if (col_data & (0x01 << row)) { // LSB=top: fix character flip


Plain Text


---

## 🐛 Troubleshooting

### ❌ OLED Garbled Display
1. Check I2C address (0x3C or 0x3D)
2. Verify wiring connections
3. Try reducing I2C frequency to 50kHz

### ❌ Character Flip
```c
// Fix horizontal mirror oled_custom_send_command(dev, 0xA1);

// Fix vertical flip oled_custom_send_command(dev, 0xC8);

// Fix character inversion if (col_data & (0x01 << row)) {


Plain Text


### ❌ Sensor Reads Zero
- Wait 15 seconds for sensor warm-up
- Check I2C wiring
- Verify sensor is not damaged

---

## 🗺️ Roadmap

### v1.0.0 (Current) ✅
- [x] LED control
- [x] SEN66 sensor integration
- [x] Alert system
- [x] SSD1306 OLED display
- [x] Dual-color layout optimization

### v1.1.0 (Planned)
- [ ] WiFi data upload
- [ ] Web configuration
- [ ] OTA updates

### v2.0.0 (Future)
- [ ] TFT-LCD integration
- [ ] RTOS optimization
- [ ] Touchscreen support

---

## 📄 License

This project is licensed under the **MIT License**.

See [LICENSE](LICENSE) for details.

---

## 🙏 Acknowledgments

- [Espressif Systems](https://www.espressif.com/) - ESP32 and ESP-IDF
- [Sensirion](https://www.sensirion.com/) - SEN66 sensor driver
- [Adafruit](https://www.adafruit.com/) - GFX library and fonts
- [FreeRTOS](https://www.freertos.org/) - RTOS

---

## 📞 Contact

- **GitHub**: [Jefferysun7972](https://github.com/Jefferysun7972)
- **Issues**: [Submit Issue](https://github.com/Jefferysun7972/esp32-environment-monitor/issues)

---

**Last Updated**: 2026-08-12 | **Version**: v1.0.0 | **Maintainer**: Jefferysun7972
