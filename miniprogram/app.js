const INFLUXDB_URL = 'https://us-east-1-1.aws.cloud2.influxdata.com';
const INFLUXDB_ORG = 'Fellowes';
const INFLUXDB_TOKEN = 'doR-H4EoxcxidC5AYN0NjzYQB7kJ5cusQvXe16b7j1W_tO4ouL35MlFayhPfTlnxR0djAgCwCFfgOVZSCXyzog==';

const { SENSORS, ALL_FIELDS } = require('./config/sensors');

App({
  globalData: {
    sensorData: Object.fromEntries(SENSORS.map(s => [s.id, {}])),
    connected: false,
    lastUpdate: ''
  },

  onLaunch() {
    this.fetchData();
    setInterval(() => this.fetchData(), 20000);
  },

  fetchData() {
    const measurements = SENSORS.map(s => s.measurement);
    let completed = 0;

    measurements.forEach((measurement) => {
      const fieldFilter = ALL_FIELDS.map(f => `r._field == "${f}"`).join(' or ');
      const query = `from(bucket: "sensor_data")
  |> range(start: -1m)
  |> filter(fn: (r) => r._measurement == "${measurement}")
  |> filter(fn: (r) => ${fieldFilter})
  |> last()`;

      wx.request({
        url: INFLUXDB_URL + '/api/v2/query?org=' + encodeURIComponent(INFLUXDB_ORG),
        method: 'POST',
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
              const sensor = SENSORS.find(s => s.measurement === measurement);
              const key = sensor ? sensor.id : measurement;
              const entry = this.globalData.sensorData[key] || {};
              parsed.forEach(row => {
                entry[row.field] = parseFloat(row.value);
              });
              this.globalData.sensorData[key] = entry;
            }
          }
        },
        fail: (err) => {
          console.error('fetch error for', measurement, err);
        },
        complete: () => {
          completed++;
          if (completed === measurements.length) {
            this.globalData.connected = true;
            this.globalData.lastUpdate = new Date().toLocaleTimeString();
            this.notifyPages();
          }
        }
      });
    });
  },

  parseCSV(csv) {
    const lines = csv.trim().split('\n');
    if (lines.length < 2) return [];
    const headers = lines[0].split(',');
    const fieldIdx = headers.indexOf('_field');
    const valueIdx = headers.indexOf('_value');
    if (fieldIdx < 0 || valueIdx < 0) return [];

    const result = [];
    for (let i = 1; i < lines.length; i++) {
      const cols = lines[i].split(',');
      result.push({
        field: cols[fieldIdx],
        value: cols[valueIdx]
      });
    }
    return result;
  },

  notifyPages() {
    const pages = getCurrentPages();
    const currentPage = pages[pages.length - 1];
    if (currentPage && currentPage.onSensorUpdate) {
      currentPage.onSensorUpdate(
        this.globalData.sensorData,
        this.globalData.connected,
        this.globalData.lastUpdate
      );
    }
  },

  fetchHistory(range, field, callback) {
    const measurementFilter = SENSORS.map(s => `r._measurement == "${s.measurement}"`).join(' or ');
    const query = `from(bucket: "sensor_data")
  |> range(start: -${range})
  |> filter(fn: (r) => ${measurementFilter})
  |> filter(fn: (r) => r._field == "${field}")
  |> aggregateWindow(every: ${this.getWindow(range)}, fn: mean)`;

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
    return '15m';
  },

  parseTimeCSV(csv, range) {
    const lines = csv.trim().split('\n');
    if (lines.length < 2) return [];

    const headers = lines[0].split(',');
    const timeIdx = headers.indexOf('_time');
    const measurementIdx = headers.indexOf('_measurement');
    const fieldIdx = headers.indexOf('_field');
    const valueIdx = headers.indexOf('_value');

    if (timeIdx < 0 || valueIdx < 0) return [];

    const is24h = range === '24h';

    const result = [];
    for (let i = 1; i < lines.length; i++) {
      const cols = lines[i].split(',');
      const timeStr = cols[timeIdx];
      let displayTime = timeStr;
      try {
        const d = new Date(timeStr);
        if (is24h) {
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

      result.push({
        time: timeStr,
        displayTime: displayTime,
        sensor: cols[measurementIdx] || '',
        field: cols[fieldIdx] || '',
        value: parseFloat(cols[valueIdx])
      });
    }
    return result;
  }
});