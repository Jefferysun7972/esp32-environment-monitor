const app = getApp();

const ACCENT_COLORS = [
  { name: '蓝', value: '#1a73e8', class: 'blue' },
  { name: '青', value: '#0097a7', class: 'cyan' },
  { name: '绿', value: '#43a047', class: 'green' },
  { name: '橙', value: '#f57c00', class: 'orange' },
  { name: '红', value: '#e53935', class: 'red' },
  { name: '紫', value: '#7b1fa2', class: 'purple' }
];

Page({
  data: {
    theme: 'light',
    rawTheme: 'light',
    pageTheme: 'light',
    isFahrenheit: false,
    accentColor: '#1a73e8',
    highContrast: false,
    accentColors: ACCENT_COLORS
  },

  _updateHeaderLabels() {
    const rawTheme = this.data.rawTheme;
    const theme = this.data.theme;
    let themeLabel;
    if (rawTheme === 'auto') {
      themeLabel = theme === 'dark' ? '深色（跟随系统）' : '浅色（跟随系统）';
    } else {
      themeLabel = theme === 'dark' ? '深色模式' : '浅色模式';
    }
    this.setData({
      themeLabel: themeLabel,
      tempUnitLabel: this.data.isFahrenheit ? '华氏度 °F' : '摄氏度 °C'
    });
  },

  onLoad() {
    const app = getApp();
    const rawTheme = app.getRawTheme();
    const effectiveTheme = app.getTheme();
    const isDark = effectiveTheme === 'dark';

    this.setData({
      theme: effectiveTheme,
      rawTheme: rawTheme,
      pageTheme: effectiveTheme,
      isFahrenheit: app.getTempUnit(),
      accentColor: app.getAccentColor(),
      highContrast: app.isHighContrast()
    });

    this._updateHeaderLabels();
    this._applyPageBackground(isDark);
  },

  onShow() {
    const app = getApp();
    const rawTheme = app.getRawTheme();
    const effectiveTheme = app.getTheme();
    const isDark = effectiveTheme === 'dark';
    
    this.setData({
      theme: effectiveTheme,
      rawTheme: rawTheme,
      pageTheme: effectiveTheme,
      isFahrenheit: app.getTempUnit(),
      accentColor: app.getAccentColor(),
      highContrast: app.isHighContrast()
    });
    
    this._updateHeaderLabels();
    this._applyPageBackground(isDark);

    const pages = getCurrentPages();
    pages.forEach(p => {
      if (p.setData) {
        p.setData({ pageTheme: effectiveTheme });
      }
    });
  },

  onThemeChange(e) {
    const theme = e.detail.value ? 'dark' : 'light';
    app.setTheme(theme);
    this.setData({ 
      theme: theme,
      rawTheme: theme,
      pageTheme: theme 
    });
    
    this._updateHeaderLabels();
    this._applyPageBackground(theme === 'dark');

    const pages = getCurrentPages();
    pages.forEach(p => {
      if (p.setData) {
        p.setData({ pageTheme: theme });
      }
    });
  },

  onAutoThemeChange(e) {
    const useAuto = e.detail.value;
    const theme = useAuto ? 'auto' : (app.getTheme() === 'dark' ? 'dark' : 'light');
    app.setTheme(theme);
    
    const effectiveTheme = app.getTheme();
    this.setData({
      theme: effectiveTheme,
      rawTheme: theme,
      pageTheme: effectiveTheme
    });
    
    this._updateHeaderLabels();
    this._applyPageBackground(effectiveTheme === 'dark');

    const pages = getCurrentPages();
    pages.forEach(p => {
      if (p.setData) {
        p.setData({ pageTheme: effectiveTheme });
      }
    });
  },

  onAccentColorTap(e) {
    const color = e.currentTarget.dataset.color;
    app.setAccentColor(color);
    this.setData({ accentColor: color });
  },

  onHighContrastChange(e) {
    const enabled = e.detail.value;
    app.setHighContrast(enabled);
    this.setData({ highContrast: enabled });

    const pages = getCurrentPages();
    pages.forEach(p => {
      if (p.setData) {
        p.setData({ highContrast: enabled });
      }
    });
  },

  onTempUnitChange(e) {
    const isFahrenheit = e.detail.value;
    app.setTempUnit(isFahrenheit);
    this.setData({ isFahrenheit });
    this._updateHeaderLabels();

    const pages = getCurrentPages();
    pages.forEach(p => {
      if (p.setData && p._buildCards) {
        p.setData({ isFahrenheit });
        p._buildCards(p.data.sensorData, p.data.sensorFilters);
      }
    });
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
  },

  onPrivacyTap() {
    wx.showModal({
      title: '隐私政策',
      content: '本小程序用于展示 ESP32 环境传感器的实时数据和历史曲线。\n\n数据说明：\n• 传感器数据存储在 InfluxDB Cloud（加密传输）\n• 数据仅用于室内空气质量监测及空气质量传感器比较测试\n• 不会分享给第三方或用于商业用途\n• 用户无法查看他人的传感器数据\n\n云函数代理：\n• 所有数据库查询通过微信云函数代理\n• 敏感凭证存储在云端环境变量中\n• 前端代码不包含任何真实密钥',
      showCancel: false,
      confirmText: '我已了解'
    });
  }
});