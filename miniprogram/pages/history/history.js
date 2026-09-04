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
    loading: false,
    chartData: null,
    canvasWidth: 0,
    canvasHeight: 220
  },

  onLoad() {
    const sysInfo = wx.getSystemInfoSync();
    this.setData({ canvasWidth: sysInfo.windowWidth - 48 });
    this.loadData();
  },

  onShow() {
    if (this.data.chartData) {
      this.drawChart();
    }
  },

  onMetricTap(e) {
    const key = e.currentTarget.dataset.key;
    this.setData({ selectedMetric: key });
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
      if (err) {
        this.setData({ loading: false });
        wx.showToast({ title: err, icon: 'none' });
        return;
      }
      const rounded = (data || []).map(d => ({
        ...d,
        value: Math.round(d.value * 10) / 10
      }));
      this.setData({ chartData: rounded, loading: false }, () => {
        setTimeout(() => this.drawChart(), 300);
      });
    });
  },

  drawChart(retryCount) {
    retryCount = retryCount || 0;
    const data = this.data.chartData;
    if (!data || data.length === 0) return;

    const ctx = wx.createCanvasContext('historyCanvas', this);
    if (!ctx) {
      if (retryCount < 3) {
        setTimeout(() => this.drawChart(retryCount + 1), 300);
      }
      return;
    }
    const W = this.data.canvasWidth;
    const H = this.data.canvasHeight;
    const pad = { t: 20, r: 12, b: 38, l: 44 };
    const pw = W - pad.l - pad.r;
    const ph = H - pad.t - pad.b;

    const am2020 = data.filter(d => d.sensor === 'am2020dy');
    const sen68 = data.filter(d => d.sensor === 'SEN68');
    const ref = am2020.length >= sen68.length ? am2020 : sen68;
    const N = Math.max(ref.length, 1);

    // Show data point counts in legend
    this.setData({
      am2020Count: am2020.length,
      sen68Count: sen68.length
    });

    const vals = data.map(d => d.value);
    let minV = Math.min(...vals), maxV = Math.max(...vals);
    if (minV === maxV) { minV -= 1; maxV += 1; }
    const rng = maxV - minV;
    minV -= rng * 0.08;
    maxV += rng * 0.08;

    const sx = (i) => pad.l + (i / Math.max(N - 1, 1)) * pw;
    const sy = (v) => pad.t + ph - ((v - minV) / (maxV - minV)) * ph;

    ctx.clearRect(0, 0, W, H);

    // Background
    ctx.setFillStyle('#fafafa');
    ctx.fillRect(pad.l, pad.t, pw, ph);

    // Grid lines
    ctx.setStrokeStyle('#e8e8e8');
    ctx.setLineWidth(0.5);
    ctx.setFillStyle('#999');
    ctx.setFontSize(10);
    ctx.setTextAlign('right');
    ctx.setTextBaseline('middle');
    for (let i = 0; i <= 4; i++) {
      const v = minV + (maxV - minV) * (i / 4);
      const y = sy(v);
      ctx.beginPath();
      ctx.moveTo(pad.l, y);
      ctx.lineTo(pad.l + pw, y);
      ctx.stroke();
      ctx.fillText(v.toFixed(1), pad.l - 6, y);
    }

    // X-axis labels
    const labelMax = Math.min(5, N);
    const labelStep = Math.max(1, Math.floor(N / labelMax));
    ctx.setFillStyle('#999');
    ctx.setFontSize(10);
    ctx.setTextAlign('center');
    ctx.setTextBaseline('top');
    let seq = 0;
    for (let i = 0; i < N; i += labelStep) {
      ctx.fillText(ref[i].displayTime, sx(i), pad.t + ph + 6 + (seq % 2 ? 14 : 0));
      seq++;
    }
    if (labelMax > 1 && (N - 1) % labelStep !== 0 && N > 1) {
      ctx.fillText(ref[N - 1].displayTime, sx(N - 1), pad.t + ph + 6 + (seq % 2 ? 14 : 0));
    }

    // Draw one series
    const drawSeries = (series, color) => {
      if (series.length === 0) return;
      const M = series.length;
      if (M === 1) {
        // Single point: draw a dot
        ctx.setFillStyle(color);
        ctx.beginPath();
        ctx.arc(sx(0), sy(series[0].value), 3, 0, 2 * Math.PI);
        ctx.fill();
        return;
      }
      ctx.setStrokeStyle(color);
      ctx.setLineWidth(2);
      ctx.beginPath();
      ctx.moveTo(sx(0), sy(series[0].value));
      for (let i = 1; i < M; i++) {
        ctx.lineTo(sx(i * (N - 1) / (M - 1)), sy(series[i].value));
      }
      ctx.stroke();
    };

    const metric = METRICS.find(m => m.key === this.data.selectedMetric);
    drawSeries(am2020, metric.color1);
    drawSeries(sen68, metric.color2);

    // Legend with counts
    ctx.setFontSize(11);
    ctx.setTextAlign('left');
    ctx.setTextBaseline('top');
    ctx.setFillStyle(metric.color1);
    ctx.fillRect(pad.l, 8, 14, 10);
    ctx.setFillStyle('#333');
    ctx.fillText('AM2020DY(' + am2020.length + ')', pad.l + 18, 8);
    ctx.setFillStyle(metric.color2);
    ctx.fillRect(pad.l + 120, 8, 14, 10);
    ctx.setFillStyle('#333');
    ctx.fillText('SEN68(' + sen68.length + ')', pad.l + 138, 8);

    ctx.draw();
  }
});