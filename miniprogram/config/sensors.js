// 字段标签映射
const FIELD_LABELS = {
  temp: '温度',
  humi: '湿度',
  pm1: 'PM1.0',
  pm25: 'PM2.5',
  pm10: 'PM10',
  tvoc: 'TVOC',
  hcho: 'HCHO',
  no2: 'NO₂',
  nox: 'NOx',
  co2: 'CO₂',
  pressure: '大气压'
};

// 字段单位映射
const FIELD_UNITS = {
  temp: '°C',
  humi: '%',
  pm1: 'µg/m³',
  pm25: 'µg/m³',
  pm10: 'µg/m³',
  tvoc: 'ppb',
  hcho: 'µg/m³',
  no2: 'µg/m³',
  nox: 'µg/m³',
  co2: 'ppm',
  pressure: 'hPa'
};

// 字段 CSS 类名映射
const FIELD_CSS_CLASS = {
  temp: 'temp',
  humi: 'humi',
  pm1: 'pm',
  pm25: 'pm',
  pm10: 'pm',
  tvoc: 'tvoc',
  hcho: 'hcho',
  no2: 'gas',
  nox: 'gas',
  co2: 'gas',
  pressure: 'pressure'
};

// 字段阈值定义：{ key: [warn_value, danger_value] }
const FIELD_THRESHOLDS = {
  temp: [26, 35],
  humi: [70, 90],
  pm1: [25, 50],
  pm25: [35, 75],
  pm10: [50, 150],
  tvoc: [500, 1000],
  hcho: [100, 200],
  no2: [100, 200],
  nox: [100, 200],
  co2: [1000, 2000],
  pressure: [1000, 1030]
};

// 传感器颜色调色板（自动发现时循环分配）
const SENSOR_COLORS = [
  { color: '#ff6d00', colorLight: '#ffab40', iconBg: '#fff3e0', iconColor: '#ff6d00' },
  { color: '#1a73e8', colorLight: '#64b5f6', iconBg: '#e3f2fd', iconColor: '#1a73e8' },
  { color: '#2e7d32', colorLight: '#81c784', iconBg: '#e8f5e9', iconColor: '#2e7d32' },
  { color: '#6a1b9a', colorLight: '#ce93d8', iconBg: '#f3e5f5', iconColor: '#6a1b9a' },
  { color: '#c62828', colorLight: '#ef9a9a', iconBg: '#ffebee', iconColor: '#c62828' },
  { color: '#00695c', colorLight: '#4db6ac', iconBg: '#e0f2f1', iconColor: '#00695c' },
  { color: '#37474f', colorLight: '#90a4ae', iconBg: '#eceff1', iconColor: '#37474f' },
  { color: '#4a148c', colorLight: '#ab47bc', iconBg: '#f3e5f5', iconColor: '#4a148c' },
];

// 兜底传感器列表（仅在自动发现失败时使用）
const ENV_FALLBACK_SENSORS = [
  {
    id: 'am2020dy',
    measurement: 'am2020dy',
    label: 'AM2020DY',
    shortLabel: 'AM',
    description: 'Sensor Module',
    color: '#ff6d00', colorLight: '#ffab40', iconBg: '#fff3e0', iconColor: '#ff6d00'
  },
  {
    id: 'SEN68',
    measurement: 'SEN68',
    label: 'SEN68',
    shortLabel: 'S6',
    description: 'Sensor Module',
    color: '#1a73e8', colorLight: '#64b5f6', iconBg: '#e3f2fd', iconColor: '#1a73e8'
  },
  {
    id: 'uart',
    measurement: 'uart',
    label: 'UART',
    shortLabel: 'UA',
    description: 'UART Sensor',
    color: '#6a1b9a', colorLight: '#ce93d8', iconBg: '#f3e5f5', iconColor: '#6a1b9a'
  }
];

module.exports = {
  SENSOR_COLORS,
  ENV_FALLBACK_SENSORS,
  FIELD_LABELS,
  FIELD_UNITS,
  FIELD_CSS_CLASS,
  FIELD_THRESHOLDS
};