const { FIELD_LABELS, FIELD_UNITS, FIELD_CSS_CLASS } = require('../../config/sensors');

// 阈值定义：{ key: [warn_value, danger_value] }
const THRESHOLDS = {
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

Page({
  data: {
    sensorData: {},
    sensorCards: [],
    sensorFilters: [],
    connected: false,
    lastUpdate: '',
    dataCached: false
  },

  _getSensors() {
    const app = getApp();
    return app._sensors ? app._sensors() : [];
  },

  onLoad() {
    const app = getApp();
    this._onSensorUpdate = (data, connected, lastUpdate) => {
      const app = getApp();
      this.setData({
        sensorData: data,
        connected: connected,
        lastUpdate: lastUpdate || '',
        dataCached: app.globalData.dataCached || false
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
      dataCached: app.globalData.dataCached || false
    });
    this._buildCards(app.globalData.sensorData, filters);
  },

  _buildCards(sensorData, sensorFilters) {
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
        if (val !== null && THRESHOLDS[f]) {
          const [warn, danger] = THRESHOLDS[f];
          if (val >= danger) status = 'danger';
          else if (val >= warn) status = 'warn';
        }
        return {
          key: f,
          label: FIELD_LABELS[f] || f,
          unit: FIELD_UNITS[f] || '',
          cssClass: FIELD_CSS_CLASS[f] || '',
          value: val !== null ? val : '--',
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
  }
});