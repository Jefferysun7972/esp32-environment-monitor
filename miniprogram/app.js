const INFLUXDB_URL = 'https://us-east-1-1.aws.cloud2.influxdata.com';
const INFLUXDB_ORG = 'Fellowes';
const INFLUXDB_TOKEN = 'doR-H4EoxcxidC5AYN0NjzYQB7kJ5cusQvXe16b7j1W_tO4ouL35MlFayhPfTlnxR0djAgCwCFfgOVZSCXyzog==';
const REFRESH_INTERVAL = 20000;

const { SENSOR_COLORS, ENV_FALLBACK_SENSORS } = require('./config/sensors');

App({
  globalData: {
    sensors: [],
    sensorData: {},
    connected: false,
    lastUpdate: '',
    dataCached: false,
    theme: 'light',
    isFahrenheit: false
  },

  _sensors() {
    return this.globalData.sensors.length > 0 ? this.globalData.sensors : ENV_FALLBACK_SENSORS;
  },

  _splitCSV(csv) {
    const allLines = csv.trim().split('\n');
    if (allLines.length < 2) return [];
    let headerIdx = 0;
    for (let i = 0; i < allLines.length; i++) {
      if (!allLines[i].startsWith('#')) {
        headerIdx = i;
        break;
      }
    }
    return allLines.slice(headerIdx);
  },

  onLaunch() {
    this._loadCache();
    this._loadSettings();
    this.discoverSensors(() => {
      this.fetchData();
      setInterval(() => this.fetchData(), REFRESH_INTERVAL);
    });
  },

  _loadCache() {
    try {
      const cache = wx.getStorageSync('sensor_cache');
      if (cache && cache.sensorData && cache.lastUpdate) {
        this.globalData.sensors = cache.sensors || ENV_FALLBACK_SENSORS;
        this.globalData.sensorData = cache.sensorData;
        this.globalData.lastUpdate = cache.lastUpdate + ' (缓存)';
        this.globalData.dataCached = true;
      }
    } catch (e) {}
  },

  _saveCache() {
    try {
      wx.setStorageSync('sensor_cache', {
        sensors: this.globalData.sensors,
        sensorData: this.globalData.sensorData,
        lastUpdate: this.globalData.lastUpdate,
        timestamp: Date.now()
      });
      this.globalData.dataCached = false;
    } catch (e) {}
  },

  _loadSettings() {
    try {
      const settings = wx.getStorageSync('app_settings');
      if (settings) {
        if (settings.theme) this.globalData.theme = settings.theme;
        if (settings.isFahrenheit !== undefined) this.globalData.isFahrenheit = settings.isFahrenheit;
      }
    } catch (e) {}
  },

  _saveSettings() {
    try {
      wx.setStorageSync('app_settings', {
        theme: this.globalData.theme,
        isFahrenheit: this.globalData.isFahrenheit
      });
    } catch (e) {}
  },

  setTheme(theme) {
    this.globalData.theme = theme;
    this._saveSettings();
    this.notifyPages();
  },

  getTheme() {
    return this.globalData.theme;
  },

  setTempUnit(isFahrenheit) {
    this.globalData.isFahrenheit = isFahrenheit;
    this._saveSettings();
  },

  getTempUnit() {
    return this.globalData.isFahrenheit;
  },

  _queryInfluxDB(query, timeout, callback) {
    wx.request({
      url: INFLUXDB_URL + '/api/v2/query?org=' + encodeURIComponent(INFLUXDB_ORG),
      method: 'POST',
      timeout: timeout,
      header: {
        'Authorization': 'Token ' + INFLUXDB_TOKEN,
        'Content-Type': 'application/vnd.flux',
        'Accept': 'application/csv'
      },
      data: query,
      success: (res) => callback(null, res),
      fail: (err) => callback(err, null)
    });
  },

  discoverSensors(callback) {
    const query = `import "influxdata/influxdb/schema"
schema.measurements(bucket: "sensor_data")`;

    this._queryInfluxDB(query, 10000, (err, res) => {
      if (!err && res && res.statusCode === 200) {
        const measurements = this._parseMeasurementsCSV(res.data);
        if (measurements.length > 0) {
          const sensors = measurements.map((m, i) => {
            const colors = SENSOR_COLORS[i % SENSOR_COLORS.length];
            return {
              id: m,
              measurement: m,
              label: m,
              shortLabel: m.substring(0, 2).toUpperCase(),
              description: 'Sensor Module',
              ...colors
            };
          });
          this.globalData.sensors = sensors;
          this.globalData.sensorData = Object.fromEntries(sensors.map(s => [s.id, {}]));
          console.log('[discover] 发现', sensors.length, '个传感器:', measurements.join(', '));
          callback();
          return;
        }
      }
      this._fallbackSensors();
      callback();
    });
  },

  _fallbackSensors() {
    console.warn('[discover] 自动发现失败，使用兜底传感器列表');
    if (this.globalData.sensors.length === 0) {
      this.globalData.sensors = ENV_FALLBACK_SENSORS;
    }
    if (Object.keys(this.globalData.sensorData).length === 0) {
      this.globalData.sensorData = Object.fromEntries(
        this.globalData.sensors.map(s => [s.id, {}])
      );
    }
  },

  _parseMeasurementsCSV(csv) {
    const lines = this._splitCSV(csv);
    if (lines.length < 2) return [];
    const headers = lines[0].split(',');
    const valueIdx = headers.indexOf('_value');
    if (valueIdx < 0) return [];
    const result = [];
    for (let i = 1; i < lines.length; i++) {
      const cols = lines[i].split(',');
      const name = (cols[valueIdx] || '').trim();
      if (name) result.push(name);
    }
    return result;
  },

  fetchData() {
    const sensors = this._sensors();
    const measurements = sensors.map(s => s.measurement);
    let completed = 0;
    let successCount = 0;

    measurements.forEach((measurement) => {
      const query = `from(bucket: "sensor_data")
  |> range(start: -5m)
  |> filter(fn: (r) => r._measurement == "${measurement}")
  |> aggregateWindow(every: 5m, fn: last, createEmpty: false)`;

      this._queryInfluxDB(query, 15000, (err, res) => {
        if (!err && res && res.statusCode === 200) {
          const parsed = this.parseCSV(res.data);
          if (parsed.length > 0) {
            successCount++;
            const sensor = sensors.find(s => s.measurement === measurement);
            const key = sensor ? sensor.id : measurement;
            const entry = this.globalData.sensorData[key] || {};
            parsed.forEach(row => {
              entry[row.field] = row.value;
            });
            this.globalData.sensorData[key] = entry;
          } else {
            console.warn('[fetchData]', measurement, '200 OK 但无数据行');
            this.globalData._lastError = measurement + ': 无数据（ESP32 可能未上报）';
          }
        } else {
          const msg = err ? (err.errMsg || '网络错误') : ('HTTP ' + (res ? res.statusCode : '?'));
          console.warn('[fetchData]', measurement, msg);
          this.globalData._lastError = measurement + ': ' + msg;
        }
        completed++;
        if (completed === measurements.length) {
          this.globalData.connected = successCount > 0;
          if (successCount > 0) {
            this.globalData.lastUpdate = new Date().toLocaleTimeString();
            this._saveCache();
          }
          this.notifyPages();
          if (!this.globalData.connected) {
            console.warn('[实时数据] 全部请求失败，最后错误:', this.globalData._lastError);
          }
        }
      });
    });
  },

  _getCSVHeaders(csv) {
    const lines = this._splitCSV(csv);
    if (lines.length < 2) return null;
    return { lines, headers: lines[0].split(',') };
  },

  parseCSV(csv) {
    const parsed = this._getCSVHeaders(csv);
    if (!parsed) return [];
    const { lines, headers } = parsed;
    const fieldIdx = headers.indexOf('_field');
    const valueIdx = headers.indexOf('_value');
    if (fieldIdx < 0 || valueIdx < 0) return [];

    const result = [];
    for (let i = 1; i < lines.length; i++) {
      const cols = lines[i].split(',');
      const val = parseFloat(cols[valueIdx]);
      if (isNaN(val)) continue;
      result.push({
        field: cols[fieldIdx],
        value: val
      });
    }
    return result;
  },

  notifyPages() {
    const pages = getCurrentPages();
    for (let i = pages.length - 1; i >= 0; i--) {
      if (pages[i] && pages[i]._onSensorUpdate) {
        pages[i]._onSensorUpdate(
          this.globalData.sensorData,
          this.globalData.connected,
          this.globalData.lastUpdate
        );
      }
    }
  },

  fetchHistory(range, field, callback) {
    const sensors = this._sensors();
    const measurementFilter = sensors.map(s => `r._measurement == "${s.measurement}"`).join(' or ');
    const query = `from(bucket: "sensor_data")
  |> range(start: -${range})
  |> filter(fn: (r) => ${measurementFilter})
  |> filter(fn: (r) => r._field == "${field}")
  |> aggregateWindow(every: ${this.getWindow(range)}, fn: mean, createEmpty: false)`;

    this._queryInfluxDB(query, 30000, (err, res) => {
      if (!err && res && res.statusCode === 200) {
        const data = this.parseTimeCSV(res.data, range);
        callback(null, data);
      } else {
        callback(err ? (err.errMsg || '网络错误') : ('查询失败: ' + (res ? res.statusCode : '?')), null);
      }
    });
  },

  getWindow(range) {
    const map = { '1h': '1m', '6h': '5m', '24h': '15m', '7d': '1h' };
    return map[range] || '6h';
  },

  parseTimeCSV(csv, range) {
    const parsed = this._getCSVHeaders(csv);
    if (!parsed) return [];
    const { lines, headers } = parsed;
    const timeIdx = headers.indexOf('_time');
    const measurementIdx = headers.indexOf('_measurement');
    const fieldIdx = headers.indexOf('_field');
    const valueIdx = headers.indexOf('_value');

    if (timeIdx < 0 || valueIdx < 0) {
      console.warn('[parseTimeCSV] 缺少 _time/_value 列，跳过数据');
      return [];
    }

    const isLong = range === '24h' || range === '7d';
    const is30d = range === '30d';

    const result = [];
    for (let i = 1; i < lines.length; i++) {
      const cols = lines[i].split(',');
      const timeStr = cols[timeIdx];
      let displayTime = timeStr;
      try {
        const d = new Date(timeStr);
        if (is30d) {
          displayTime = (d.getMonth() + 1) + '/' + d.getDate();
        } else if (isLong) {
          displayTime = (d.getMonth() + 1) + '/' + d.getDate() + ' ' +
            String(d.getHours()).padStart(2, '0') + ':' +
            String(d.getMinutes()).padStart(2, '0');
        } else {
          const now = new Date();
          const isToday = d.getFullYear() === now.getFullYear() &&
            d.getMonth() === now.getMonth() &&
            d.getDate() === now.getDate();
          if (isToday) {
            displayTime = String(d.getHours()).padStart(2, '0') + ':' +
              String(d.getMinutes()).padStart(2, '0');
          } else {
            displayTime = (d.getMonth() + 1) + '/' + d.getDate() + ' ' +
              String(d.getHours()).padStart(2, '0') + ':' +
              String(d.getMinutes()).padStart(2, '0');
          }
        }
      } catch (e) {}

      const val = parseFloat(cols[valueIdx]);
      if (isNaN(val)) continue;

      result.push({
        time: timeStr,
        displayTime: displayTime,
        sensor: cols[measurementIdx] || '',
        field: cols[fieldIdx] || '',
        value: val
      });
    }
    return result;
  }
});