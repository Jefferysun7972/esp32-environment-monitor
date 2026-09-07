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
    tableData: null,
    stats: null,
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

      // Group by displayTime for table
      const groupMap = {};
      rounded.forEach(d => {
        if (!groupMap[d.displayTime]) {
          groupMap[d.displayTime] = { displayTime: d.displayTime };
        }
        groupMap[d.displayTime][d.sensor] = d.value;
      });
      const tableData = Object.values(groupMap);

      // Compute stats per sensor
      const am2020Vals = rounded.filter(d => d.sensor === 'am2020dy').map(d => d.value);
      const sen68Vals = rounded.filter(d => d.sensor === 'SEN68').map(d => d.value);
      const calcStats = (vals) => {
        if (vals.length === 0) return { min: '-', max: '-', avg: '-' };
        const min = Math.min(...vals);
        const max = Math.max(...vals);
        const sum = vals.reduce((a, b) => a + b, 0);
        const avg = sum / vals.length;
        return { min: min.toFixed(1), max: max.toFixed(1), avg: avg.toFixed(1) };
      };
      const stats = {
        am2020dy: calcStats(am2020Vals),
        sen68: calcStats(sen68Vals)
      };

      this.setData({ chartData: rounded, tableData, stats, loading: false }, () => {
        setTimeout(() => this.drawChart(), 200);
      });
    });
  },

  drawChart(touchPoint) {
    const data = this.data.chartData;
    if (!data || data.length === 0) return;

    const query = wx.createSelectorQuery().in(this);
    query.select('#historyCanvas')
      .fields({ node: true, size: true })
      .exec((res) => {
        if (!res || !res[0] || !res[0].node) {
          setTimeout(() => this.drawChart(), 300);
          return;
        }
        const canvas = res[0].node;
        const W = this.data.canvasWidth;
        const H = this.data.canvasHeight;
        const dpr = wx.getSystemInfoSync().pixelRatio;

        canvas.width = W * dpr;
        canvas.height = H * dpr;

        const ctx = canvas.getContext('2d');
        ctx.scale(dpr, dpr);

        this.renderChart(ctx, data, W, H, touchPoint);
      });
  },

  onCanvasTouch(e) {
    const meta = this._chartMeta;
    if (!meta) return;
    const touch = e.touches[0];
    if (!touch) return;

    const x = touch.x;
    const y = touch.y;

    // Find nearest data point by x
    let bestIdx = 0;
    let bestDist = Infinity;
    for (let i = 0; i < meta.N; i++) {
      const dist = Math.abs(meta.sx(i) - x);
      if (dist < bestDist) { bestDist = dist; bestIdx = i; }
    }

    // Only show if within plot area
    if (x < meta.pad.l || x > meta.pad.l + meta.pw ||
        y < meta.pad.t || y > meta.pad.t + meta.ph) {
      this.drawChart();
      return;
    }

    this.drawChart({ idx: bestIdx, x, y });
  },

  onCanvasTouchEnd() {
    this.drawChart();
  },

  renderChart(ctx, data, W, H, touchPoint) {
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

    const metric = METRICS.find(m => m.key === this.data.selectedMetric);

    ctx.clearRect(0, 0, W, H);

    // Background
    ctx.fillStyle = '#fafafa';
    ctx.fillRect(pad.l, pad.t, pw, ph);

    // Grid lines
    ctx.strokeStyle = '#e8e8e8';
    ctx.lineWidth = 0.5;
    ctx.fillStyle = '#999';
    ctx.font = '10px sans-serif';
    ctx.textAlign = 'right';
    ctx.textBaseline = 'middle';
    for (let i = 0; i <= 4; i++) {
      const v = minV + (maxV - minV) * (i / 4);
      const y = sy(v);
      ctx.beginPath();
      ctx.moveTo(pad.l, y);
      ctx.lineTo(pad.l + pw, y);
      ctx.stroke();
      ctx.fillText(v.toFixed(1) + metric.unit, pad.l - 6, y);
    }

    // X-axis labels
    const isLongRange = this.data.selectedRange === '24h';
    const labelMax = Math.min(5, N);
    const labelStep = Math.max(1, Math.floor(N / labelMax));
    ctx.fillStyle = '#999';
    ctx.font = '10px sans-serif';
    ctx.textAlign = 'center';
    ctx.textBaseline = 'top';
    let seq = 0;
    for (let i = 0; i < N; i += labelStep) {
      ctx.fillText(ref[i].displayTime, sx(i), pad.t + ph + 6 + (isLongRange && seq % 2 ? 14 : 0));
      seq++;
    }
    if (labelMax > 1 && (N - 1) % labelStep !== 0 && N > 1) {
      ctx.fillText(ref[N - 1].displayTime, sx(N - 1), pad.t + ph + 6 + (isLongRange && seq % 2 ? 14 : 0));
    }

    // Draw one series
    const drawSeries = (series, color) => {
      if (series.length === 0) return;
      const M = series.length;
      if (M === 1) {
        ctx.fillStyle = color;
        ctx.beginPath();
        ctx.arc(sx(0), sy(series[0].value), 3, 0, 2 * Math.PI);
        ctx.fill();
        return;
      }
      ctx.strokeStyle = color;
      ctx.lineWidth = 2;
      ctx.beginPath();
      ctx.moveTo(sx(0), sy(series[0].value));
      for (let i = 1; i < M; i++) {
        ctx.lineTo(sx(i * (N - 1) / (M - 1)), sy(series[i].value));
      }
      ctx.stroke();
    };

    drawSeries(am2020, metric.color1);
    drawSeries(sen68, metric.color2);

    // Legend with counts
    ctx.font = '11px sans-serif';
    ctx.textAlign = 'left';
    ctx.textBaseline = 'top';
    ctx.fillStyle = metric.color1;
    ctx.fillRect(pad.l, 8, 14, 10);
    ctx.fillStyle = '#333';
    ctx.fillText('AM2020DY(' + am2020.length + ')', pad.l + 18, 8);
    ctx.fillStyle = metric.color2;
    ctx.fillRect(pad.l + 120, 8, 14, 10);
    ctx.fillStyle = '#333';
    ctx.fillText('SEN68(' + sen68.length + ')', pad.l + 138, 8);

    // Save metadata for touch handler
    this._chartMeta = { am2020, sen68, ref, N, pad, sx, sy, pw, ph, metric };

    // Draw crosshair + tooltip
    if (touchPoint) {
      const { idx, x, y } = touchPoint;
      const cx = sx(idx);
      const t = ref[idx];

      // Crosshair lines
      ctx.strokeStyle = 'rgba(0,0,0,0.25)';
      ctx.lineWidth = 0.5;
      ctx.setLineDash([4, 4]);
      ctx.beginPath();
      ctx.moveTo(pad.l, y);
      ctx.lineTo(pad.l + pw, y);
      ctx.moveTo(cx, pad.t);
      ctx.lineTo(cx, pad.t + ph);
      ctx.stroke();
      ctx.setLineDash([]);

      // Dot on the reference series
      ctx.fillStyle = '#333';
      ctx.beginPath();
      ctx.arc(cx, sy(ref.value), 4, 0, 2 * Math.PI);
      ctx.fill();

      // Tooltip box
      const amVal = am2020.length > 0 && idx < am2020.length
        ? am2020[Math.round(idx * (am2020.length - 1) / Math.max(N - 1, 1))].value : null;
      const seVal = sen68.length > 0 && idx < sen68.length
        ? sen68[Math.round(idx * (sen68.length - 1) / Math.max(N - 1, 1))].value : null;

      const lines = [
        t.displayTime,
        'AM2020DY: ' + (amVal !== null ? amVal.toFixed(1) + metric.unit : '--'),
        'SEN68: ' + (seVal !== null ? seVal.toFixed(1) + metric.unit : '--')
      ];
      const fontH = 14;
      const boxW = 180;
      const boxH = lines.length * fontH + 16;
      let bx = cx + 10;
      let by = y - boxH - 10;
      if (bx + boxW > W) bx = cx - boxW - 10;
      if (by < 0) by = y + 10;

      ctx.fillStyle = 'rgba(0,0,0,0.75)';
      ctx.beginPath();
      ctx.roundRect ? ctx.roundRect(bx, by, boxW, boxH, 6) : ctx.fillRect(bx, by, boxW, boxH);
      ctx.fill();

      ctx.fillStyle = '#fff';
      ctx.font = '12px sans-serif';
      ctx.textAlign = 'left';
      ctx.textBaseline = 'top';
      lines.forEach((line, i) => {
        ctx.fillText(line, bx + 10, by + 8 + i * fontH);
      });
    }
  }
});