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

    const query = wx.createSelectorQuery();
    query.select('#historyCanvas')
      .fields({ node: true, size: true })
      .exec((res) => {
        if (!res[0] || !res[0].node) {
          // Retry once after layout
          setTimeout(() => this.drawChart(), 200);
          return;
        }
        const canvas = res[0].node;
        const ctx = canvas.getContext('2d');
        const dpr = wx.getSystemInfoSync().pixelRatio;
        const width = res[0].width;
        const height = res[0].height;
        canvas.width = width * dpr;
        canvas.height = height * dpr;
        ctx.scale(dpr, dpr);

        this.renderChart(ctx, data, width, height);
      });
  },

  renderChart(ctx, data, width, height) {
    const padding = { top: 25, right: 16, bottom: 40, left: 48 };
    const plotW = width - padding.left - padding.right;
    const plotH = height - padding.top - padding.bottom;

    // Separate by sensor
    const am2020 = data.filter(d => d.sensor === 'am2020dy');
    const sen68 = data.filter(d => d.sensor === 'SEN68');

    // Use the larger series as reference for x-axis labels
    const xRef = am2020.length >= sen68.length ? am2020 : sen68;
    const total = Math.max(xRef.length, 1);

    const allValues = data.map(d => d.value);
    if (allValues.length === 0) return;
    let minVal = Math.min(...allValues);
    let maxVal = Math.max(...allValues);
    if (minVal === maxVal) { minVal -= 1; maxVal += 1; }
    const valRange = maxVal - minVal;
    minVal -= valRange * 0.05;
    maxVal += valRange * 0.05;

    const scaleX = (i) => padding.left + (i / Math.max(total - 1, 1)) * plotW;
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
    ctx.textBaseline = 'middle';
    for (let i = 0; i <= 4; i++) {
      const val = minVal + (maxVal - minVal) * (i / 4);
      const y = scaleY(val);
      ctx.beginPath();
      ctx.moveTo(padding.left, y);
      ctx.lineTo(padding.left + plotW, y);
      ctx.stroke();
      ctx.fillText(val.toFixed(1), padding.left - 6, y);
    }

    // X axis labels from reference series (evenly spaced, ~5 labels)
    const labelCount = Math.min(5, total);
    const step = Math.max(1, Math.floor(total / labelCount));
    ctx.fillStyle = '#999';
    ctx.font = '10px sans-serif';
    ctx.textAlign = 'center';
    ctx.textBaseline = 'top';
    let labelSeq = 0;
    for (let i = 0; i < total; i += step) {
      const x = scaleX(i);
      const yOffset = (labelSeq % 2 === 0) ? 0 : 14;
      ctx.fillText(xRef[i].displayTime, x, padding.top + plotH + 6 + yOffset);
      labelSeq++;
    }
    // Always show last label
    const lastIdx = total - 1;
    if (labelCount > 1 && lastIdx % step !== 0 && lastIdx > 0) {
      const x = scaleX(lastIdx);
      const yOffset = (labelSeq % 2 === 0) ? 0 : 14;
      ctx.fillText(xRef[lastIdx].displayTime, x, padding.top + plotH + 6 + yOffset);
    }

    // Draw a series
    const drawSeries = (series, color, dash) => {
      if (series.length < 2) return;
      ctx.strokeStyle = color;
      ctx.lineWidth = 2;
      ctx.setLineDash(dash ? [6, 4] : []);
      ctx.beginPath();
      ctx.moveTo(scaleX(0), scaleY(series[0].value));
      for (let i = 1; i < series.length; i++) {
        const xi = scaleX(i * (total - 1) / Math.max(series.length - 1, 1));
        ctx.lineTo(xi, scaleY(series[i].value));
      }
      ctx.stroke();
      ctx.setLineDash([]);

      // Points
      series.forEach((d, i) => {
        const xi = scaleX(i * (total - 1) / Math.max(series.length - 1, 1));
        ctx.fillStyle = color;
        ctx.beginPath();
        ctx.arc(xi, scaleY(d.value), 2.5, 0, 2 * Math.PI);
        ctx.fill();
      });
    };

    const metric = METRICS.find(m => m.key === this.data.selectedMetric);

    drawSeries(am2020, metric.color1, false);
    drawSeries(sen68, metric.color2, true);

    // Legend
    const legY = 10;
    ctx.font = '11px sans-serif';
    ctx.textAlign = 'left';
    ctx.textBaseline = 'top';

    ctx.fillStyle = metric.color1;
    ctx.fillRect(padding.left, legY, 14, 10);
    ctx.fillStyle = '#333';
    ctx.fillText('AM2020DY', padding.left + 18, legY);

    ctx.fillStyle = metric.color2;
    ctx.fillRect(padding.left + 100, legY, 14, 10);
    ctx.fillStyle = '#333';
    ctx.fillText('SEN68', padding.left + 118, legY);
  }
});