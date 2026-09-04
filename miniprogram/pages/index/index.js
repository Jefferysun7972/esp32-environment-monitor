Page({
  data: {
    am2020dy: {},
    sen68: {},
    connected: false,
    lastUpdate: ''
  },

  onLoad() {
    const app = getApp();
    this.onSensorUpdate = (data, connected) => {
      this.setData({
        am2020dy: data.am2020dy || {},
        sen68: data.sen68 || {},
        connected: connected,
        lastUpdate: new Date().toLocaleTimeString()
      });
    };

    this.setData({
      am2020dy: app.globalData.sensorData.am2020dy || {},
      sen68: app.globalData.sensorData.sen68 || {},
      connected: app.globalData.connected
    });
  }
});