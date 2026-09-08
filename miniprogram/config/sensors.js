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

// 传感器定义 —— 更换传感器只需修改这里
const SENSORS = [
  {
    id: 'am2020dy',
    measurement: 'am2020dy',
    label: 'AM2020DY',
    shortLabel: 'AM',
    description: 'Sensor Module',
    color: '#ff6d00',
    colorLight: '#ffab40',
    iconBg: '#fff3e0',
    iconColor: '#ff6d00'
  },
  {
    id: 'sen68',
    measurement: 'SEN68',
    label: 'SEN68',
    shortLabel: 'S6',
    description: 'Sensor Module',
    color: '#1a73e8',
    colorLight: '#64b5f6',
    iconBg: '#e3f2fd',
    iconColor: '#1a73e8'
  }
];

// 所有已知字段（从 FIELD_LABELS 自动生成，添加新字段只需改 FIELD_LABELS）
const ALL_FIELDS = Object.keys(FIELD_LABELS);

module.exports = {
  SENSORS,
  FIELD_LABELS,
  FIELD_UNITS,
  FIELD_CSS_CLASS,
  ALL_FIELDS
};