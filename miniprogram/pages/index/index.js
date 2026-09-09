const { FIELD_LABELS, FIELD_UNITS, FIELD_CSS_CLASS, FIELD_THRESHOLDS } = require('../../config/sensors');

Page({
  data: {
    sensorData: {},
    sensorCards: [],
    sensorFilters: [],
    connected: false,
    lastUpdate: '',
    dataCached: false,
    isFahrenheit: false,
    pageTheme: 'light',
    highContrast: false,
    accentColor: '#1a73e8',
    isLoading: true,  // ✨ 添加加载状态
    isFirstLoad: true // ✨ 标记是否首次加载
  },

  _getSensors() {
    const app = getApp();
    return app._sensors ? app._sensors() : [];
  },

  onLoad() {
    const app = getApp();
    
    const isDark = app.getTheme() === 'dark';
    
    this.setData({ pageTheme: isDark ? 'dark' : 'light' });
    
    this._applyPageBackground(isDark);
    
    this._onSensorUpdate = (data, connected, lastUpdate) => {
      const app = getApp();
      this.setData({
        sensorData: data,
        connected: connected,
        lastUpdate: lastUpdate || '',
        dataCached: app.globalData.dataCached || false,
        isFahrenheit: app.getTempUnit(),
        pageTheme: app.getTheme(),
        highContrast: app.isHighContrast(),
        accentColor: app.getAccentColor(),
        isLoading: false  // ✨ 数据加载完成
      });
      this._buildCards(data, this.data.sensorFilters);
    };

    const sensors = this._getSensors();
    const filters = sensors.map(s => ({
      id: s.id,
      label: s.label,
      displayLabel: s.label.length > 5 ? s.label.substring(0, 5) + '…' : s.label,
      color: s.color,
      active: true
    }));

    this.setData({ sensorFilters: filters });
    this._buildCards(app.globalData.sensorData, filters);
  },

  onShow() {
    const app = getApp();
    
    const isDark = app.getTheme() === 'dark';
    const theme = isDark ? 'dark' : 'light';
    
    this._applyPageBackground(isDark);
    
    const sensors = this._getSensors();
    const filters = (this.data.sensorFilters.length === sensors.length && sensors.length > 0)
      ? this.data.sensorFilters
      : sensors.map(s => ({
        id: s.id, label: s.label,
        displayLabel: s.label.length > 5 ? s.label.substring(0, 5) + '…' : s.label,
        color: s.color, active: true
      }));
    if (this.data.sensorFilters.length !== sensors.length) {
      this.setData({ sensorFilters: filters });
    }
    this.setData({
      sensorData: app.globalData.sensorData,
      connected: app.globalData.connected,
      lastUpdate: app.globalData.lastUpdate || '',
      dataCached: app.globalData.dataCached || false,
      isFahrenheit: app.getTempUnit(),
      pageTheme: theme,
      highContrast: app.isHighContrast(),
      accentColor: app.getAccentColor(),
      isLoading: false  // ✨ 数据加载完成
    });
    this._buildCards(app.globalData.sensorData, filters);
  },

  _buildCards(sensorData, sensorFilters) {
    const app = getApp();
    const sensors = this._getSensors();
    const filters = sensorFilters || this.data.sensorFilters || [];
    const activeSet = new Set(filters.filter(f => f.active).map(f => f.id));
    const cards = sensors.map(s => {
      const data = sensorData[s.id] || {};
      const fields = Object.keys(data);
      const allMetrics = fields.map(f => {
        const rawVal = data[f];
        const val = rawVal !== undefined && rawVal !== null && !isNaN(rawVal) ? rawVal : null;
        let status = '';
        if (val !== null && FIELD_THRESHOLDS[f]) {
          const [warn, danger] = FIELD_THRESHOLDS[f];
          if (val >= danger) status = 'danger';
          else if (val >= warn) status = 'warn';
        }
        let displayVal = val !== null ? val : '--';
        let displayUnit = FIELD_UNITS[f] || '';
        if (f === 'temp' && val !== null && app.getTempUnit()) {
          displayVal = (val * 9 / 5 + 32).toFixed(1);
          displayUnit = '°F';
        }
        return {
          key: f,
          label: FIELD_LABELS[f] || f,
          unit: displayUnit,
          cssClass: FIELD_CSS_CLASS[f] || '',
          value: displayVal,
          status: status
        };
      });

      const rows = [];
      const perRow = 3;
      for (let i = 0; i < allMetrics.length; i += perRow) {
        rows.push({
          cols: perRow,
          metrics: allMetrics.slice(i, i + perRow),
          rowKey: 'row' + Math.floor(i / perRow)
        });
      }

      return { ...s, rows, paramCount: fields.length, visible: activeSet.has(s.id) };
    });
    this.setData({ sensorCards: cards });
  },

  onSensorToggle(e) {
    const id = e.currentTarget.dataset.id;
    const filters = this.data.sensorFilters.map(f => {
      if (f.id === id) return { ...f, active: !f.active };
      return f;
    });
    this.setData({ sensorFilters: filters });
    this._buildCards(this.data.sensorData, filters);
  },

  onRefresh() {
    const app = getApp();
    app.fetchData();
    wx.showToast({ title: '刷新中', icon: 'loading', duration: 1000 });
  },

  onPullDownRefresh() {
    const app = getApp();
    app.fetchData();
    setTimeout(() => wx.stopPullDownRefresh(), 1000);
  },

  _applyPageBackground(isDark) {
    const app = getApp();
    if (app.applyPageBackground) {
      app.applyPageBackground(isDark);
    }
  },

  onShareAppMessage() {
    return {
      title: '传感器对比测试 - 实时环境监测',
      path: '/pages/index/index'
    };
  }
});