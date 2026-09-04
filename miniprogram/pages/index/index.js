Page({
  data: {
    am2020dy: {},
    sen68: {},
    connected: false,
    lastUpdate: ''
  },

  onLoad() {
    const app = getApp();
    this.onSensorUpdate = (data) => {
      this.setData({
        am2020dy: data.am2020dy || {},
        sen68: data.sen68 || {},
        connected: true,
        lastUpdate: new Date().toLocaleTimeString()
      });
    };

    if (app.globalData.sensorData) {
      this.setData({
        am2020dy: app.globalData.sensorData.am2020dy || {},
        sen68: app.globalData.sensorData.sen68 || {},
        connected: true
      });
    }
  }
});