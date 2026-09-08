const { SENSORS, FIELD_LABELS, FIELD_UNITS, FIELD_CSS_CLASS } = require('../../config/sensors');

Page({
  data: {
    sensorData: {},
    sensorCards: [],
    connected: false,
    lastUpdate: ''
  },

  onLoad() {
    const app = getApp();
    this._onSensorUpdate = (data, connected, lastUpdate) => {
      this.setData({
        sensorData: data,
        connected: connected,
        lastUpdate: lastUpdate || ''
      });
      this._buildCards(data);
    };

    this.setData({
      sensorData: app.globalData.sensorData,
      connected: app.globalData.connected
    });
    this._buildCards(app.globalData.sensorData);
  },

  onShow() {
    const app = getApp();
    this.setData({
      sensorData: app.globalData.sensorData,
      connected: app.globalData.connected,
      lastUpdate: app.globalData.lastUpdate || ''
    });
    this._buildCards(app.globalData.sensorData);
  },

  _buildCards(sensorData) {
    const cards = SENSORS.map(s => {
      const data = sensorData[s.id] || {};
      const fields = Object.keys(data);
      const allMetrics = fields.map(f => ({
        key: f,
        label: FIELD_LABELS[f] || f,
        unit: FIELD_UNITS[f] || '',
        cssClass: FIELD_CSS_CLASS[f] || '',
        value: data[f] !== undefined && data[f] !== null && !isNaN(data[f]) ? data[f] : '--'
      }));

      const rows = [];
      const perRow = 3;
      for (let i = 0; i < allMetrics.length; i += perRow) {
        rows.push({
          cols: perRow,
          metrics: allMetrics.slice(i, i + perRow),
          rowKey: 'row' + Math.floor(i / perRow)
        });
      }

      return { ...s, rows, paramCount: fields.length };
    });
    this.setData({ sensorCards: cards });
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