const INFLUXDB_URL = 'https://us-east-1-1.aws.cloud2.influxdata.com';
const INFLUXDB_ORG = 'Fellowes';
const INFLUXDB_TOKEN = 'doR-H4EoxcxidC5AYN0NjzYQB7kJ5cusQvXe16b7j1W_tO4ouL35MlFayhPfTlnxR0djAgCwCFfgOVZSCXyzog==';
const REFRESH_INTERVAL = 20000;

const { SENSORS, ALL_FIELDS } = require('./config/sensors');

App({
  globalData: {
    sensorData: Object.fromEntries(SENSORS.map(s => [s.id, {}])),
    connected: false,
    lastUpdate: ''
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
    this.fetchData();
    setInterval(() => this.fetchData(), REFRESH_INTERVAL);
  },

  fetchData() {
    const measurements = SENSORS.map(s => s.measurement);
    let completed = 0;
    let successCount = 0;

    measurements.forEach((measurement) => {
      const fieldFilter = ALL_FIELDS.map(f => `r._field == "${f}"`).join(' or ');
      const query = `from(bucket: "sensor_data")
  |> range(start: -5m)
  |> filter(fn: (r) => r._measurement == "${measurement}")
  |> filter(fn: (r) => ${fieldFilter})
  |> aggregateWindow(every: 5m, fn: last, createEmpty: false)`;

      wx.request({
        url: INFLUXDB_URL + '/api/v2/query?org=' + encodeURIComponent(INFLUXDB_ORG),
        method: 'POST',
        timeout: 15000,
        header: {
          'Authorization': 'Token ' + INFLUXDB_TOKEN,
          'Content-Type': 'application/vnd.flux',
          'Accept': 'application/csv'
        },
        data: query,
        success: (res) => {
          if (res.statusCode === 200) {
            const parsed = this.parseCSV(res.data);
            if (parsed.length > 0) {
              successCount++;
              const sensor = SENSORS.find(s => s.measurement === measurement);
              const key = sensor ? sensor.id : measurement;
              const entry = this.globalData.sensorData[key] || {};
              parsed.forEach(row => {
                entry[row.field] = row.value;
              });
              this.globalData.sensorData[key] = entry;
            } else {
              console.warn('[fetchData]', measurement, '200 OK 但无数据行，原始响应:', res.data.substring(0, 200));
              this.globalData._lastError = measurement + ': 无数据（ESP32 可能未上报）';
            }
          } else {
            console.warn('[fetchData]', measurement, 'HTTP', res.statusCode, '原始响应:', res.data.substring(0, 200));
            this.globalData._lastError = measurement + ': HTTP ' + res.statusCode;
          }
        },
        fail: (err) => {
          const msg = (err && err.errMsg) ? err.errMsg : '请求被拦截或超时';
          console.error('fetch error for', measurement, err);
          this.globalData._lastError = measurement + ': ' + msg;
        },
        complete: () => {
          completed++;
          if (completed === measurements.length) {
            this.globalData.connected = successCount > 0;
            if (successCount > 0) {
              this.globalData.lastUpdate = new Date().toLocaleTimeString();
            }
            this.notifyPages();
            if (!this.globalData.connected) {
              console.warn('[实时数据] 全部请求失败，最后错误:', this.globalData._lastError);
            }
          }
        }
      });
    });
  },

  parseCSV(csv) {
    const lines = this._splitCSV(csv);
    if (lines.length < 2) return [];

    const headers = lines[0].split(',');
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
    const measurementFilter = SENSORS.map(s => `r._measurement == "${s.measurement}"`).join(' or ');
    const query = `from(bucket: "sensor_data")
  |> range(start: -${range})
  |> filter(fn: (r) => ${measurementFilter})
  |> filter(fn: (r) => r._field == "${field}")
  |> aggregateWindow(every: ${this.getWindow(range)}, fn: mean, createEmpty: false)`;

    wx.request({
      url: INFLUXDB_URL + '/api/v2/query?org=' + encodeURIComponent(INFLUXDB_ORG),
      method: 'POST',
      timeout: 30000,
      header: {
        'Authorization': 'Token ' + INFLUXDB_TOKEN,
        'Content-Type': 'application/vnd.flux',
        'Accept': 'application/csv'
      },
      data: query,
      success: (res) => {
        if (res.statusCode === 200) {
          const data = this.parseTimeCSV(res.data, range);
          callback(null, data);
        } else {
          callback('查询失败: ' + res.statusCode, null);
        }
      },
      fail: (err) => {
        callback(err.errMsg || '网络错误', null);
      }
    });
  },

  getWindow(range) {
    if (range === '1h') return '1m';
    if (range === '6h') return '5m';
    if (range === '24h') return '15m';
    if (range === '7d') return '1h';
    return '6h';
  },

  parseTimeCSV(csv, range) {
    const lines = this._splitCSV(csv);
    if (lines.length < 2) return [];

    const headers = lines[0].split(',');
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