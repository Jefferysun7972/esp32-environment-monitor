const app = getApp();

Page({
  data: {
    theme: 'light',
    isFahrenheit: false
  },

  onShow() {
    this.setData({
      theme: app.getTheme(),
      isFahrenheit: app.getTempUnit()
    });

    const pages = getCurrentPages();
    pages.forEach(p => {
      if (p.setData) {
        p.setData({ pageTheme: app.getTheme() });
      }
    });
  },

  onThemeChange(e) {
    const theme = e.detail.value ? 'dark' : 'light';
    app.setTheme(theme);
    this.setData({ theme });

    const pages = getCurrentPages();
    pages.forEach(p => {
      if (p.setData) {
        p.setData({ pageTheme: theme });
      }
    });
  },

  onTempUnitChange(e) {
    const isFahrenheit = e.detail.value;
    app.setTempUnit(isFahrenheit);
    this.setData({ isFahrenheit });

    const pages = getCurrentPages();
    pages.forEach(p => {
      if (p.setData && p._buildCards) {
        p.setData({ isFahrenheit });
        p._buildCards(p.data.sensorData, p.data.sensorFilters);
      }
    });
  },

  onShareAppMessage() {
    return {
      title: '传感器对比测试 - 实时环境监测',
      path: '/pages/index/index'
    };
  }
});