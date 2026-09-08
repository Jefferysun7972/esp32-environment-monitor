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
      const allMetrics = s.fields.map(f => ({
        key: f,
        label: FIELD_LABELS[f] || f,
        unit: FIELD_UNITS[f] || '',
        cssClass: FIELD_CSS_CLASS[f] || '',
        value: data[f] !== undefined && data[f] !== null && !isNaN(data[f]) ? data[f] : '--'
      }));

      // Split metrics into rows according to rowLayout
      const rows = [];
      let cursor = 0;
      (s.rowLayout || [s.fields.length]).forEach((cols, idx) => {
        rows.push({
          cols: cols,
          metrics: allMetrics.slice(cursor, cursor + cols),
          rowKey: 'row' + idx
        });
        cursor += cols;
      });

      return { ...s, rows };
    });
    this.setData({ sensorCards: cards });
  },

  onRefresh() {
    const app = getApp();
    app.fetchData();
    wx.showToast({ title: '刷新中', icon: 'loading', duration: 1000 });
  }
});