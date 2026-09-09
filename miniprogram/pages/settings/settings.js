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
    const bgColor = isDark ? '#1a1a2e' : '#f5f5f5';
    
    // 注意：导航栏颜色由 app.js 统一管理，避免重复设置导致闪烁
    
    // 设置页面根元素背景色
    if (wx.setBackgroundColor) {
      wx.setBackgroundColor({
        backgroundColor: bgColor,
        backgroundColorTop: bgColor,
        backgroundColorBottom: bgColor
      });
    }
    
    // 设置 page 元素样式，确保背景色正确
    if (wx.setPageStyle) {
      wx.setPageStyle({
        style: {
          background: bgColor
        }
      });
    }
  },

  onShareAppMessage() {
    return {
      title: '传感器对比测试 - 实时环境监测',
      path: '/pages/index/index'
    };
  }
});