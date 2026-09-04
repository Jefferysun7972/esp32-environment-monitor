const mqtt = require('mqtt');

App({
  globalData: {
    mqttClient: null,
    sensorData: {
      am2020dy: {},
      sen68: {}
    }
  },

  onLaunch() {
    this.connectMQTT();
  },

  connectMQTT() {
    const client = mqtt.connect(
      'wss://f4319339.ala.cn-hangzhou.emqxsl.cn:8084/mqtt',
      {
        username: 'jerrysun',
        password: 'renyi1004',
        clientId: 'wx_' + Math.random().toString(16).substr(2, 10),
        reconnectPeriod: 5000,
        connectTimeout: 10000,
      }
    );

    client.on('connect', () => {
      console.log('MQTT connected');
      client.subscribe(['sensor/am2020dy', 'sensor/sen68']);
    });

    client.on('message', (topic, payload) => {
      const data = JSON.parse(payload.toString());
      const key = topic.split('/')[1];

      this.globalData.sensorData[key] = data;

      const pages = getCurrentPages();
      const currentPage = pages[pages.length - 1];
      if (currentPage && currentPage.onSensorUpdate) {
        currentPage.onSensorUpdate(this.globalData.sensorData);
      }
    });

    client.on('reconnect', () => {
      console.log('MQTT reconnecting...');
    });

    client.on('error', (err) => {
      console.error('MQTT error:', err);
    });

    this.globalData.mqttClient = client;
  }
});