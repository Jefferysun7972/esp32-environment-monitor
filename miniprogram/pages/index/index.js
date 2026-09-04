Page({
  data: {
    am2020dy: {},
    sen68: {},
    connected: false,
    lastUpdate: ''
  },

  onLoad() {
    const app = getApp();
    this.onSensorUpdate = (data, connected, lastUpdate) => {
      this.setData({
        am2020dy: data.am2020dy || {},
        sen68: data.sen68 || {},
        connected: connected,
        lastUpdate: lastUpdate || ''
      });
    };

    this.setData({
      am2020dy: app.globalData.sensorData.am2020dy || {},
      sen68: app.globalData.sensorData.sen68 || {},
      connected: app.globalData.connected
    });
  },

  onRefresh() {
    const app = getApp();
    app.fetchData();
    wx.showToast({ title: '刷新中', icon: 'loading', duration: 1000 });
  }
});