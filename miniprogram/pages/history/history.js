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
    const width = sysInfo.windowWidth - 48;
    this.setData({ canvasWidth: width, canvasHeight: 220 });
    this.loadData();
  },

  onShow() {
    if (this.data.chartData) {
      this.drawChart();
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
      this.setData({ chartData: data });
      this.drawChart();
    });
  },

  drawChart() {
    const data = this.data.chartData;
    if (!data || data.length === 0) return;

    const query = wx.createSelectorQuery();
    query.select('#historyCanvas')
      .fields({ node: true, size: true })
      .exec((res) => {
        if (!res[0] || !res[0].node) return;
        const canvas = res[0].node;
        const ctx = canvas.getContext('2d');
        const width = this.data.canvasWidth;
        const height = this.data.canvasHeight;

        const dpr = wx.getSystemInfoSync().pixelRatio;
        canvas.width = width * dpr;
        canvas.height = height * dpr;
        ctx.scale(dpr, dpr);

        this.renderChart(ctx, data, width, height);
      });
  },

  renderChart(ctx, data, width, height) {
    const padding = { top: 20, right: 20, bottom: 40, left: 50 };
    const plotW = width - padding.left - padding.right;
    const plotH = height - padding.top - padding.bottom;

    // Separate data by sensor
    const am2020Data = data.filter(d => d.sensor === 'am2020dy');
    const sen68Data = data.filter(d => d.sensor === 'SEN68');

    // Find min/max across all data
    let allValues = data.map(d => d.value);
    let minVal = Math.min(...allValues);
    let maxVal = Math.max(...allValues);
    if (minVal === maxVal) { minVal -= 1; maxVal += 1; }
    const range = maxVal - minVal;
    minVal -= range * 0.05;
    maxVal += range * 0.05;

    const scaleX = (i, total) => padding.left + (i / (total - 1)) * plotW;
    const scaleY = (v) => padding.top + plotH - ((v - minVal) / (maxVal - minVal)) * plotH;

    // Clear
    ctx.clearRect(0, 0, width, height);

    // Background
    ctx.fillStyle = '#fafafa';
    ctx.fillRect(padding.left, padding.top, plotW, plotH);

    // Grid lines + Y labels
    ctx.strokeStyle = '#e8e8e8';
    ctx.lineWidth = 0.5;
    ctx.fillStyle = '#999';
    ctx.font = '10px sans-serif';
    ctx.textAlign = 'right';
    for (let i = 0; i <= 4; i++) {
      const val = minVal + (maxVal - minVal) * (i / 4);
      const y = scaleY(val);
      ctx.beginPath();
      ctx.moveTo(padding.left, y);
      ctx.lineTo(padding.left + plotW, y);
      ctx.stroke();
      ctx.fillText(val.toFixed(1), padding.left - 6, y + 4);
    }

    // X axis labels
    ctx.fillStyle = '#999';
    ctx.font = '10px sans-serif';
    ctx.textAlign = 'center';
    const xLabels = this.getXLabels(data, 5);
    xLabels.forEach(l => {
      const idx = data.findIndex(d => d.displayTime === l.label);
      if (idx >= 0) {
        const x = scaleX(idx, data.length);
        ctx.fillText(l.label, x, padding.top + plotH + 18);
      }
    });

    // Draw lines
    const drawLine = (series, color, dash) => {
      if (series.length < 2) return;
      ctx.strokeStyle = color;
      ctx.lineWidth = 2;
      if (dash) ctx.setLineDash([4, 3]);
      else ctx.setLineDash([]);
      ctx.beginPath();
      const firstIdx = data.indexOf(series[0]);
      ctx.moveTo(scaleX(firstIdx, data.length), scaleY(series[0].value));
      for (let i = 1; i < series.length; i++) {
        const idx = data.indexOf(series[i]);
        ctx.lineTo(scaleX(idx, data.length), scaleY(series[i].value));
      }
      ctx.stroke();
      ctx.setLineDash([]);
    };

    // Draw points
    const drawPoints = (series, color) => {
      series.forEach((d, i) => {
        const idx = data.indexOf(d);
        const x = scaleX(idx, data.length);
        const y = scaleY(d.value);
        ctx.fillStyle = color;
        ctx.beginPath();
        ctx.arc(x, y, 3, 0, 2 * Math.PI);
        ctx.fill();
      });
    };

    const metric = METRICS.find(m => m.key === this.data.selectedMetric);

    drawLine(am202Data, metric.color1, false);
    drawPoints(am202Data, metric.color1);
    drawLine(sen68Data, metric.color2, true);
    drawPoints(sen68Data, metric.color2);

    // Legend
    const legendY = 10;
    ctx.font = '11px sans-serif';

    ctx.fillStyle = metric.color1;
    ctx.fillRect(padding.left, legendY, 14, 10);
    ctx.fillStyle = '#333';
    ctx.textAlign = 'left';
    ctx.fillText('AM2020DY', padding.left + 18, legendY + 9);

    ctx.fillStyle = metric.color2;
    ctx.fillRect(padding.left + 90, legendY, 14, 10);
    ctx.fillStyle = '#333';
    ctx.fillText('SEN68', padding.left + 108, legendY + 9);
  },

  getXLabels(data, count) {
    if (data.length <= count) {
      return data.map(d => ({ label: d.displayTime }));
    }
    const step = Math.floor(data.length / (count - 1));
    const labels = [];
    for (let i = 0; i < data.length; i += step) {
      labels.push({ label: data[i].displayTime });
    }
    if (labels.length < count) {
      labels.push({ label: data[data.length - 1].displayTime });
    }
    return labels.slice(0, count);
  }
});