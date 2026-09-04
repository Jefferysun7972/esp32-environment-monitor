const mqtt = require('./mqtt');

App({
  globalData: {
    sensorData: {
      am2020dy: {},
      sen68: {}
    },
    connected: false
  },

  onLaunch() {
    mqtt.connect({
      onConnected: () => {
        this.globalData.connected = true;
        this.notifyPages();
      },
      onDisconnected: () => {
        this.globalData.connected = false;
        this.notifyPages();
      },
      onMessage: (topic, payload) => {
        try {
          const data = JSON.parse(payload);
          const key = topic.split('/')[1];
          this.globalData.sensorData[key] = data;
          this.notifyPages();
        } catch (e) {
          console.error('parse error:', e);
        }
      }
    });
  },

  notifyPages() {
    const pages = getCurrentPages();
    const currentPage = pages[pages.length - 1];
    if (currentPage && currentPage.onSensorUpdate) {
      currentPage.onSensorUpdate(
        this.globalData.sensorData,
        this.globalData.connected
      );
    }
  }
});