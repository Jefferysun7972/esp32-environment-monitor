const METRICS = [
  { key: 'temp', label: '温度', unit: '°C', color1: '#ff6d00', color2: '#ffab40' },
  { key: 'humi', label: '湿度', unit: '%', color1: '#1a73e8', color2: '#64b5f6' },
  { key: 'pm25', label: 'PM2.5', unit: 'µg/m³', color1: '#6a1b9a', color2: '#ce93d8' },
  { key: 'pm1', label: 'PM1.0', unit: 'µg/m³', color1: '#7b1fa2', color2: '#ba68c8' },
  { key: 'pm10', label: 'PM10', unit: 'µg/m³', color1: '#4a148c', color2: '#9c27b0' },
  { key: 'tvoc', label: 'TVOC', unit: 'ppb', color1: '#c62828', color2: '#ef5350' },
  { key: 'hcho', label: 'HCHO', unit: 'µg/m³', color1: '#e65100', color2: '#ff9800' },
  { key: 'no2', label: 'NO₂', unit: 'µg/m³', color1: '#2e7d32', color2: '#66bb6a' },
  { key: 'nox', label: 'NOx', unit: 'µg/m³', color1: '#1b5e20', color2: '#4caf50' },
];

const RANGES = [
  { key: '1h', label: '1小时' },
  { key: '6h', label: '6小时' },
  { key: '24h', label: '24小时' },
];

Page({
  data: {
    metrics: METRICS,
    ranges: RANGES,
    selectedMetric: 'temp',
    selectedRange: '1h',
    metricLabel: '温度',
    metricUnit: '°C',
    loading: false,
    chartData: null,
    canvasWidth: 0,
    canvasHeight: 0
  },

  onLoad() {
    const sysInfo = wx.getSystemInfoSync();
    this.setData({
      canvasWidth: sysInfo.windowWidth - 48,
      canvasHeight: 220
    });
    this.loadData();
  },

  onShow() {
    if (this.data.chartData) {
      setTimeout(() => this.drawChart(), 300);
    }
  },

  onMetricTap(e) {
    const key = e.currentTarget.dataset.key;
    const metric = METRICS.find(m => m.key === key);
    this.setData({
      selectedMetric: key,
      metricLabel: metric.label,
      metricUnit: metric.unit
    });
    this.loadData();
  },

  onRangeTap(e) {
    const key = e.currentTarget.dataset.key;
    this.setData({ selectedRange: key });
    this.loadData();
  },

  loadData() {
    this.setData({ loading: true });
    const app = getApp();
    app.fetchHistory(this.data.selectedRange, this.data.selectedMetric, (err, data) => {
      this.setData({ loading: false });
      if (err) {
        wx.showToast({ title: err, icon: 'none' });
        return;
      }
      const rounded = (data || []).map(d => ({
        ...d,
        value: Math.round(d.value * 10) / 10
      }));
      this.setData({ chartData: rounded });
      setTimeout(() => this.drawChart(), 300);
    });
  },

  drawChart() {
    const data = this.data.chartData;
    if (!data || data.length === 0) return;

    const ctx = wx.createCanvasContext('historyCanvas', this);
    const width = this.data.canvasWidth;
    const height = this.data.canvasHeight;
    const padding = { top: 25, right: 16, bottom: 40, left: 48 };
    const plotW = width - padding.left - padding.right;
    const plotH = height - padding.top - padding.bottom;

    // Separate by sensor
    const am2020Data = data.filter(d => d.sensor === 'am2020dy');
    const sen68Data = data.filter(d => d.sensor === 'SEN68');

    const allValues = data.map(d => d.value);
    let minVal = Math.min(...allValues);
    let maxVal = Math.max(...allValues);
    if (minVal === maxVal) { minVal -= 1; maxVal += 1; }
    const valRange = maxVal - minVal;
    minVal -= valRange * 0.05;
    maxVal += valRange * 0.05;

    const scaleX = (i, total) => padding.left + (i / Math.max(total - 1, 1)) * plotW;
    const scaleY = (v) => padding.top + plotH - ((v - minVal) / (maxVal - minVal)) * plotH;

    // Background
    ctx.setFillStyle('#fafafa');
    ctx.fillRect(padding.left, padding.top, plotW, plotH);

    // Grid lines + Y labels
    ctx.setStrokeStyle('#e8e8e8');
    ctx.setLineWidth(0.5);
    ctx.setFillStyle('#999');
    ctx.setFontSize(10);
    ctx.setTextAlign('right');
    ctx.setTextBaseline('middle');
    for (let i = 0; i <= 4; i++) {
      const val = minVal + (maxVal - minVal) * (i / 4);
      const y = scaleY(val);
      ctx.beginPath();
      ctx.moveTo(padding.left, y);
      ctx.lineTo(padding.left + plotW, y);
      ctx.stroke();
      ctx.fillText(val.toFixed(1), padding.left - 6, y);
    }

    // X axis labels - pick ~5 evenly spaced
    const labelCount = Math.min(5, data.length);
    const step = Math.max(1, Math.floor(data.length / labelCount));
    const labelIndices = [];
    for (let i = 0; i < data.length; i += step) {
      labelIndices.push(i);
    }
    if (labelIndices[labelIndices.length - 1] !== data.length - 1) {
      labelIndices.push(data.length - 1);
    }

    ctx.setFillStyle('#999');
    ctx.setFontSize(10);
    ctx.setTextAlign('center');
    ctx.setTextBaseline('top');
    labelIndices.forEach((idx, i) => {
      const d = data[idx];
      const x = scaleX(idx, data.length);
      // Stagger: even indices on first line, odd on second
      const yOffset = (i % 2 === 0) ? 0 : 14;
      ctx.fillText(d.displayTime, x, padding.top + plotH + 6 + yOffset);
    });

    // Draw lines
    const drawLine = (series, color, dash) => {
      if (series.length < 2) return;
      ctx.setStrokeStyle(color);
      ctx.setLineWidth(2);
      if (dash) ctx.setLineDash([6, 4], 0);
      else ctx.setLineDash([], 0);
      ctx.beginPath();
      const firstIdx = data.indexOf(series[0]);
      ctx.moveTo(scaleX(firstIdx, data.length), scaleY(series[0].value));
      for (let i = 1; i < series.length; i++) {
        const idx = data.indexOf(series[i]);
        ctx.lineTo(scaleX(idx, data.length), scaleY(series[i].value));
      }
      ctx.stroke();
      ctx.setLineDash([], 0);
    };

    // Draw points
    const drawPoints = (series, color) => {
      series.forEach(d => {
        const idx = data.indexOf(d);
        const x = scaleX(idx, data.length);
        const y = scaleY(d.value);
        ctx.setFillStyle(color);
        ctx.beginPath();
        ctx.arc(x, y, 2.5, 0, 2 * Math.PI);
        ctx.fill();
      });
    };

    const metric = METRICS.find(m => m.key === this.data.selectedMetric);

    drawLine(am202Data, metric.color1, false);
    drawPoints(am202Data, metric.color1);
    drawLine(sen68Data, metric.color2, true);
    drawPoints(sen68Data, metric.color2);

    // Legend
    const legY = 10;
    ctx.setFontSize(11);
    ctx.setTextAlign('left');
    ctx.setTextBaseline('top');

    ctx.setFillStyle(metric.color1);
    ctx.fillRect(padding.left, legY, 14, 10);
    ctx.setFillStyle('#333');
    ctx.fillText('AM2020DY', padding.left + 18, legY);

    ctx.setFillStyle(metric.color2);
    ctx.fillRect(padding.left + 100, legY, 14, 10);
    ctx.setFillStyle('#333');
    ctx.fillText('SEN68', padding.left + 118, legY);

    ctx.draw();
  }
});