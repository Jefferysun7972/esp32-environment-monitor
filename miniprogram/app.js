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
    isFahrenheit: false,
    accentColor: '#1a73e8',
    highContrast: false
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
    
    // 如果主题为 auto，检测系统深色模式
    if (this.globalData.theme === 'auto') {
      const sysInfo = wx.getSystemInfoSync();
      this.globalData.theme = sysInfo.theme === 'dark' ? 'dark' : 'light';
    }
    
    // ✨ 立即设置全局导航栏和背景色，避免页面闪烁
    this._applyGlobalTheme();
    
    // 监听前台/后台切换，确保从后台恢复时立即设置样式
    if (wx.onAppShow) {
      wx.onAppShow(() => {
        this._applyGlobalTheme();
      });
    }
    
    // 监听系统主题变化
    if (wx.onThemeChange) {
      wx.onThemeChange((res) => {
        if (this.globalData.theme === 'auto') {
          const newTheme = res.theme === 'dark' ? 'dark' : 'light';
          this.globalData.theme = newTheme;
          this._applyTheme(newTheme);
          this.notifyPages();
        }
      });
    }
    
    // ✨ 优化：异步发现传感器，不阻塞页面渲染
    // 先使用缓存或兜底传感器，让页面立即显示
    this._initSensorsAndFetchData();
  },
  
  _initSensorsAndFetchData() {
    // 立即使用已有的传感器（缓存或兜底）
    if (this.globalData.sensors.length === 0) {
      this.globalData.sensors = ENV_FALLBACK_SENSORS;
    }
    
    console.log('[App] ✅ 初始化完成，传感器列表已就绪');
    
    // ⚡ 不在这里调用 fetchData，让页面自己按需加载
    // 各页面的 onShow 会负责获取数据
    
    // 延迟启动定时器（给页面时间先渲染）
    setTimeout(() => {
      this.fetchData(); // 首次数据获取
      setInterval(() => this.fetchData(), REFRESH_INTERVAL); // 定时刷新
      
      // 异步发现传感器（后台）
      this.discoverSensors((newSensors) => {
        if (newSensors && newSensors.length > 0) {
          console.log('[App] 发现到新传感器，更新列表');
          this.fetchData();
        }
      });
    }, 500); // 延迟500ms，确保页面已渲染
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
        if (settings.accentColor) this.globalData.accentColor = settings.accentColor;
        if (settings.highContrast !== undefined) this.globalData.highContrast = settings.highContrast;
      }
    } catch (e) {}
  },

  _saveSettings() {
    try {
      wx.setStorageSync('app_settings', {
        theme: this.globalData.theme,
        isFahrenheit: this.globalData.isFahrenheit,
        accentColor: this.globalData.accentColor,
        highContrast: this.globalData.highContrast
      });
    } catch (e) {}
  },

  _applyGlobalTheme() {
    const isDark = this.globalData.theme === 'dark';
    const accent = this.globalData.accentColor;
    
    // ✨ 立即设置导航栏颜色（无动画，避免闪烁）
    if (wx.setNavigationBarColor) {
      wx.setNavigationBarColor({
        frontColor: isDark ? '#ffffff' : '#000000',
        backgroundColor: isDark ? '#1a1a2e' : '#ffffff',
        animation: {
          duration: 0,
          timingFunc: 'linear'
        }
      });
    }
    
    // 设置 tabBar 样式
    if (wx.setTabBarStyle) {
      wx.setTabBarStyle({
        color: isDark ? '#777' : '#999',
        selectedColor: accent,
        backgroundColor: isDark ? '#1a1a2e' : '#fff',
        borderStyle: isDark ? 'white' : 'black'
      });
    }
    
    // 设置全局背景色
    if (wx.setBackgroundColor) {
      wx.setBackgroundColor({
        backgroundColor: isDark ? '#1a1a2e' : '#f5f5f5',
        backgroundColorTop: isDark ? '#1a1a2e' : '#f5f5f5',
        backgroundColorBottom: isDark ? '#1a1a2e' : '#f5f5f5'
      });
    }
    
    // 设置 page 元素样式
    if (wx.setPageStyle) {
      wx.setPageStyle({
        style: {
          background: isDark ? '#1a1a2e' : '#f5f5f5'
        }
      });
    }
  },

  _applyTheme(theme) {
    const isDark = theme === 'dark';
    const accent = this.globalData.accentColor;
    
    // 导航栏保持中性色，不跟随强调色，避免突兀
    wx.setNavigationBarColor({
      frontColor: isDark ? '#ffffff' : '#000000',
      backgroundColor: isDark ? '#1a1a2e' : '#ffffff',
      animation: {
        duration: 200,
        timingFunc: 'easeIn'
      }
    });
    
    wx.setTabBarStyle({
      color: isDark ? '#777' : '#999',
      selectedColor: accent,
      backgroundColor: isDark ? '#1a1a2e' : '#fff',
      borderStyle: isDark ? 'white' : 'black'
    });
    
    if (wx.setBackgroundColor) {
      wx.setBackgroundColor({
        backgroundColor: isDark ? '#1a1a2e' : '#f5f5f5',
        backgroundColorTop: isDark ? '#1a1a2e' : '#f5f5f5',
        backgroundColorBottom: isDark ? '#1a1a2e' : '#f5f5f5'
      });
    }
    
    // 设置 page 元素样式，确保背景色正确
    if (wx.setPageStyle) {
      wx.setPageStyle({
        style: {
          background: isDark ? '#1a1a2e' : '#f5f5f5'
        }
      });
    }
  },

  setTheme(theme) {
    this.globalData.theme = theme;
    this._saveSettings();
    
    // 如果设置为 auto，检测当前系统主题
    let effectiveTheme = theme;
    if (theme === 'auto') {
      const sysInfo = wx.getSystemInfoSync();
      effectiveTheme = sysInfo.theme === 'dark' ? 'dark' : 'light';
    }
    
    this._applyTheme(effectiveTheme);
    this.notifyPages();
  },

  getTheme() {
    // 如果主题是 auto，返回实际生效的主题
    if (this.globalData.theme === 'auto') {
      const sysInfo = wx.getSystemInfoSync();
      return sysInfo.theme === 'dark' ? 'dark' : 'light';
    }
    return this.globalData.theme;
  },

  getRawTheme() {
    return this.globalData.theme;
  },

  setTempUnit(isFahrenheit) {
    this.globalData.isFahrenheit = isFahrenheit;
    this._saveSettings();
  },

  getTempUnit() {
    return this.globalData.isFahrenheit;
  },

  setAccentColor(color) {
    this.globalData.accentColor = color;
    this._saveSettings();
    this._applyTheme(this.getTheme());
    this.notifyPages();
  },

  getAccentColor() {
    return this.globalData.accentColor;
  },

  setHighContrast(enabled) {
    this.globalData.highContrast = enabled;
    this._saveSettings();
    this.notifyPages();
  },

  isHighContrast() {
    return this.globalData.highContrast;
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

    console.log('[discover] 开始自动发现传感器...');
    this._queryInfluxDB(query, 10000, (err, res) => {
      if (!err && res && res.statusCode === 200) {
        console.log('[discover] 查询成功，响应数据长度:', res.data ? res.data.length : 0);
        const measurements = this._parseMeasurementsCSV(res.data);
        console.log('[discover] 解析到测量值:', measurements);
        
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
          console.log('[discover] ✅ 发现', sensors.length, '个传感器:', measurements.join(', '));
          callback();
          return;
        } else {
          console.warn('[discover] ⚠️ 查询成功但未找到任何 measurement');
        }
      } else {
        const msg = err ? (err.errMsg || '网络错误') : ('HTTP ' + (res ? res.statusCode : '?'));
        console.error('[discover] ❌ 自动发现失败:', msg);
      }
      
      console.log('[discover] 使用兜底传感器列表');
      this._fallbackSensors();
      callback();
    });
  },

  _fallbackSensors() {
    console.warn('[_fallbackSensors] ⚠️ 自动发现失败，使用兜底传感器列表');
    console.log('[_fallbackSensors] 兜底传感器:', ENV_FALLBACK_SENSORS.map(s => s.id).join(', '));
    
    if (this.globalData.sensors.length === 0) {
      this.globalData.sensors = ENV_FALLBACK_SENSORS;
      console.log('[_fallbackSensors] 已设置全局传感器为兜底列表');
    } else {
      console.log('[_fallbackSensors] 已有传感器，保持不变:', this.globalData.sensors.map(s => s.id).join(', '));
    }
    
    if (Object.keys(this.globalData.sensorData).length === 0) {
      this.globalData.sensorData = Object.fromEntries(
        this.globalData.sensors.map(s => [s.id, {}])
      );
      console.log('[_fallbackSensors] 已初始化空 sensorData');
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
    console.log('[fetchHistory] 可用传感器:', JSON.stringify(sensors.map(s => ({id: s.id, measurement: s.measurement}))));
    console.log('[fetchHistory] globalData.sensors:', JSON.stringify(this.globalData.sensors.map(s => s.id)));
    
    const measurementFilter = sensors.map(s => `r._measurement == "${s.measurement}"`).join(' or ');
    const query = `from(bucket: "sensor_data")
  |> range(start: -${range})
  |> filter(fn: (r) => ${measurementFilter})
  |> filter(fn: (r) => r._field == "${field}")
  |> aggregateWindow(every: ${this.getWindow(range)}, fn: mean, createEmpty: false)`;

    console.log('[fetchHistory] 查询参数:');
    console.log('  - 时间范围:', range);
    console.log('  - 字段:', field);
    console.log('  - 传感器过滤器:', measurementFilter);
    console.log('  - 完整查询:\n', query);
    
    this._queryInfluxDB(query, 30000, (err, res) => {
      if (!err && res && res.statusCode === 200) {
        const csvLen = res.data ? res.data.length : 0;
        console.log('[fetchHistory] ✅ 请求成功:');
        console.log('  - HTTP 状态码:', res.statusCode);
        console.log('  - CSV 数据长度:', csvLen, '字节');
        
        if (csvLen > 0) {
          console.log('  - CSV 前200字符:', res.data.substring(0, 200));
          console.log('  - CSV 行数:', (res.data.match(/\n/g) || []).length);
        }
        
        const data = this.parseTimeCSV(res.data, range);
        console.log('[fetchHistory] 解析结果:', data.length, '条数据');
        
        if (data.length > 0) {
          console.log('  - 第一条数据:', JSON.stringify(data[0]));
          console.log('  - 最后一条数据:', JSON.stringify(data[data.length - 1]));
        }
        
        callback(null, data);
      } else {
        const msg = err ? (err.errMsg || '网络错误') : ('HTTP ' + (res ? res.statusCode : '?'));
        console.error('[fetchHistory] ❌ 请求失败:');
        console.log('  - 错误信息:', msg);
        if (err) {
          console.log('  - 完整错误对象:', JSON.stringify(err));
        }
        if (res) {
          console.log('  - 响应状态码:', res.statusCode);
          console.log('  - 响应数据:', res.data ? res.data.substring(0, 500) : 'null');
        }
        callback(msg, null);
      }
    });
  },

  getWindow(range) {
    const map = { '1h': '1m', '6h': '5m', '24h': '15m', '7d': '1h' };
    return map[range] || '6h';
  },

  parseTimeCSV(csv, range) {
    console.log('[parseTimeCSV] 开始解析 CSV...');
    
    const parsed = this._getCSVHeaders(csv);
    if (!parsed) {
      console.warn('[parseTimeCSV] ❌ CSV 解析失败：无法获取表头');
      return [];
    }
    
    const { lines, headers } = parsed;
    console.log('[parseTimeCSV] 表头:', headers.join(' | '));
    console.log('[parseTimeCSV] 总行数:', lines.length, '(含表头)');
    
    const timeIdx = headers.indexOf('_time');
    const measurementIdx = headers.indexOf('_measurement');
    const fieldIdx = headers.indexOf('_field');
    const valueIdx = headers.indexOf('_value');

    console.log('[parseTimeCSV] 列索引: _time=' + timeIdx + ', _measurement=' + measurementIdx + ', _field=' + fieldIdx + ', _value=' + valueIdx);

    if (timeIdx < 0 || valueIdx < 0) {
      console.warn('[parseTimeCSV] ❌ 缺少必要列: _time=' + timeIdx + ', _value=' + valueIdx);
      return [];
    }

    const isLong = range === '24h' || range === '7d';
    const is30d = range === '30d';

    const result = [];
    let skipCount = 0;
    
    for (let i = 1; i < lines.length; i++) {
      const cols = lines[i].split(',');
      
      if (cols.length <= Math.max(timeIdx, valueIdx)) {
        skipCount++;
        continue;
      }
      
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
      if (isNaN(val)) {
        skipCount++;
        continue;
      }

      result.push({
        time: timeStr,
        displayTime: displayTime,
        sensor: cols[measurementIdx] || '',
        field: cols[fieldIdx] || '',
        value: val
      });
    }
    
    console.log('[parseTimeCSV] ✅ 解析完成:');
    console.log('  - 有效数据:', result.length, '条');
    console.log('  - 跳过行数:', skipCount, '条');
    
    if (result.length > 0) {
      const sensors = [...new Set(result.map(r => r.sensor))];
      console.log('  - 包含传感器:', sensors.join(', '));
    }
    
    return result;
  },

  testInfluxDBConnection() {
    console.log('\n========== InfluxDB 连接测试 ==========');
    
    const testQuery = `from(bucket: "sensor_data")
  |> range(start: -1h)
  |> limit(n: 5)`;
    
    console.log('测试查询:', testQuery);
    
    this._queryInfluxDB(testQuery, 10000, (err, res) => {
      if (!err && res && res.statusCode === 200) {
        console.log('✅ InfluxDB 连接成功!');
        console.log('响应数据 (前500字符):');
        console.log(res.data ? res.data.substring(0, 500) : 'null');
        
        if (res.data) {
          const lines = res.data.split('\n');
          console.log('总行数:', lines.length);
          if (lines.length > 0) {
            console.log('表头:', lines[0]);
          }
          if (lines.length > 1) {
            console.log('示例数据:', lines[1]);
          }
        }
      } else {
        console.error('❌ InfluxDB 连接失败!');
        console.error('错误:', err);
        console.error('响应:', res);
      }
      console.log('======================================\n');
    });
  }
});