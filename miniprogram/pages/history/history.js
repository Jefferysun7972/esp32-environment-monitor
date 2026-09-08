const { FIELD_LABELS, FIELD_UNITS } = require('../../config/sensors');

const METRICS = [
  { key: 'temp', label: '温度', unit: '°C', color1: '#ff6d00', color2: '#ffab40', thresholds: [
    { value: 26, color: '#ff9800', label: '26°C' },
    { value: 35, color: '#e53935', label: '35°C' }
  ]},
  { key: 'humi', label: '湿度', unit: '%', color1: '#1a73e8', color2: '#64b5f6', thresholds: [
    { value: 70, color: '#ff9800', label: '70%' },
    { value: 90, color: '#e53935', label: '90%' }
  ]},
  { key: 'pm25', label: 'PM2.5', unit: 'µg/m³', color1: '#6a1b9a', color2: '#ce93d8', thresholds: [
    { value: 35, color: '#ff9800', label: '35' },
    { value: 75, color: '#e53935', label: '75' }
  ]},
  { key: 'pm1', label: 'PM1.0', unit: 'µg/m³', color1: '#7b1fa2', color2: '#ba68c8', thresholds: [
    { value: 25, color: '#ff9800', label: '25' },
    { value: 50, color: '#e53935', label: '50' }
  ]},
  { key: 'pm10', label: 'PM10', unit: 'µg/m³', color1: '#4a148c', color2: '#9c27b0', thresholds: [
    { value: 50, color: '#ff9800', label: '50' },
    { value: 150, color: '#e53935', label: '150' }
  ]},
  { key: 'tvoc', label: 'TVOC', unit: 'ppb', color1: '#c62828', color2: '#ef5350', thresholds: [
    { value: 500, color: '#ff9800', label: '500' },
    { value: 1000, color: '#e53935', label: '1000' }
  ]},
  { key: 'hcho', label: 'HCHO', unit: 'µg/m³', color1: '#e65100', color2: '#ff9800', thresholds: [
    { value: 100, color: '#ff9800', label: '100' },
    { value: 200, color: '#e53935', label: '200' }
  ]},
  { key: 'no2', label: 'NO₂', unit: 'µg/m³', color1: '#2e7d32', color2: '#66bb6a', thresholds: [
    { value: 100, color: '#ff9800', label: '100' },
    { value: 200, color: '#e53935', label: '200' }
  ]},
  { key: 'nox', label: 'NOx', unit: 'µg/m³', color1: '#1b5e20', color2: '#4caf50', thresholds: [
    { value: 100, color: '#ff9800', label: '100' },
    { value: 200, color: '#e53935', label: '200' }
  ]},
  { key: 'co2', label: 'CO₂', unit: 'ppm', color1: '#004d40', color2: '#26a69a', thresholds: [
    { value: 1000, color: '#ff9800', label: '1000' },
    { value: 2000, color: '#e53935', label: '2000' }
  ]},
  { key: 'pressure', label: '大气压', unit: 'hPa', color1: '#006064', color2: '#4dd0e1', thresholds: [
    { value: 1000, color: '#ff9800', label: '1000' },
    { value: 1030, color: '#e53935', label: '1030' }
  ]},
];

const RANGES = [
  { key: '1h', label: '1小时' },
  { key: '6h', label: '6小时' },
  { key: '24h', label: '24小时' },
  { key: '7d', label: '7天' },
  { key: '30d', label: '30天' },
];

Page({
  data: {
    metrics: [],
    ranges: RANGES,
    sensors: [],
    selectedMetric: 'temp',
    selectedRange: '1h',
    loading: false,
    chartData: null,
    tableData: null,
    stats: null,
    canvasWidth: 0,
    canvasHeight: 220,
    metricUnit: '°C',
    tempUnit: '°C',
    tempDropdownOpen: false,
    showSensor: [],
    exportModalOpen: false,
    exportMetrics: [],
    exportRange: '1h',
    exporting: false
  },

  _getSensors() {
    const app = getApp();
    return app._sensors ? app._sensors() : [];
  },

  onLoad() {
    this._requestSeq = 0;
    const sysInfo = wx.getSystemInfoSync();
    const sensors = this._getSensors();
    this.setData({
      canvasWidth: sysInfo.windowWidth - 48,
      sensors: sensors,
      showSensor: sensors.map(() => true)
    });
    this._buildMetrics();
    this.loadData();
  },

  _buildMetrics() {
    const app = getApp();
    const sensorData = app.globalData.sensorData || {};
    const metrics = METRICS.map(m => {
      let supported = false;
      for (const sid in sensorData) {
        if (sensorData[sid][m.key] !== undefined && sensorData[sid][m.key] !== null && !isNaN(sensorData[sid][m.key])) {
          supported = true;
          break;
        }
      }
      return { ...m, supported };
    });

    // 自动发现 METRICS 中未定义但 ESP32 实际在报的新字段
    const knownKeys = new Set(METRICS.map(m => m.key));
    const discovered = new Set();
    for (const sid in sensorData) {
      for (const key in sensorData[sid]) {
        if (!knownKeys.has(key) && sensorData[sid][key] !== undefined && !isNaN(sensorData[sid][key])) {
          discovered.add(key);
        }
      }
    }
    discovered.forEach(key => {
      metrics.push({
        key,
        label: FIELD_LABELS[key] || key,
        unit: FIELD_UNITS[key] || '',
        color1: '#607d8b',
        color2: '#90a4ae',
        thresholds: [],
        supported: true
      });
    });

    this.setData({ metrics });
  },

  onShow() {
    const sensors = this._getSensors();
    if (sensors.length > 0 && this.data.sensors.length !== sensors.length) {
      this.setData({ sensors, showSensor: sensors.map(() => true) });
    }
    this._buildMetrics();
    if (this.data.chartData) {
      setTimeout(() => this.drawChart(), 50);
    }
  },

  onPullDownRefresh() {
    this.loadData(() => {
      wx.stopPullDownRefresh();
    });
  },

  onMetricTap(e) {
    const key = e.currentTarget.dataset.key;
    const metric = this.data.metrics.find(m => m.key === key);
    if (!metric || !metric.supported) return;
    this.setData({ selectedMetric: key, tempDropdownOpen: false });
    this.loadData();
  },

  onRangeTap(e) {
    const key = e.currentTarget.dataset.key;
    this.setData({ selectedRange: key, tempDropdownOpen: false });
    this.loadData();
  },

  onTempChipTap() {
    if (this.data.selectedMetric !== 'temp') {
      this.setData({ selectedMetric: 'temp', tempDropdownOpen: false });
      this.loadData();
    } else {
      this.setData({ tempDropdownOpen: !this.data.tempDropdownOpen });
    }
  },

  loadData(callback) {
    this.setData({ loading: true });
    const seq = ++this._requestSeq;
    const app = getApp();
    app.fetchHistory(this.data.selectedRange, this.data.selectedMetric, (err, data) => {
      if (seq !== this._requestSeq) return;
      if (err) {
        this.setData({ loading: false });
        wx.showToast({ title: err, icon: 'none' });
        if (callback) callback();
        return;
      }
      this._rawData = data || [];
      this._processRawData(callback);
    });
  },

  _processRawData(callback) {
    const rawData = this._rawData || [];
    const isTemp = this.data.selectedMetric === 'temp';
    const toFahrenheit = isTemp && this.data.tempUnit === '°F';

    const convertVal = (v) => {
      if (typeof v !== 'number' || isNaN(v)) return null;
      if (toFahrenheit) return Math.round((v * 9 / 5 + 32) * 10) / 10;
      return Math.round(v * 10) / 10;
    };

    const rounded = rawData.map(d => ({
      ...d,
      value: convertVal(d.value)
    })).filter(d => d.value !== null);

    // Group by displayTime for table
    const groupMap = {};
    rounded.forEach(d => {
      if (!groupMap[d.displayTime]) {
        groupMap[d.displayTime] = { displayTime: d.displayTime };
      }
      groupMap[d.displayTime][d.sensor] = d.value;
    });
    const sensors = this._getSensors();
    const tableData = Object.values(groupMap).map(row => ({
      displayTime: row.displayTime,
      measurements: sensors.map(s => row[s.measurement] !== undefined ? row[s.measurement] : null)
    }));

    // Compute stats per sensor (keyed by sensor ID)
    const stats = {};
    sensors.forEach(s => {
      const vals = rounded.filter(d => d.sensor === s.measurement).map(d => d.value);
      if (vals.length === 0) {
        stats[s.id] = { min: '-', max: '-', avg: '-' };
      } else {
        const min = Math.min(...vals);
        const max = Math.max(...vals);
        const sum = vals.reduce((a, b) => a + b, 0);
        const avg = sum / vals.length;
        stats[s.id] = { min: min.toFixed(1), max: max.toFixed(1), avg: avg.toFixed(1) };
      }
    });

    const metric = METRICS.find(m => m.key === this.data.selectedMetric);
    const metricUnit = isTemp && toFahrenheit ? '°F' : (metric ? metric.unit : '');

    this.setData({ chartData: rounded, tableData, stats, metricUnit, loading: false }, () => {
      wx.nextTick(() => this.drawChart());
      if (callback) callback();
    });
  },

  onTempUnitChange(e) {
    const unit = e.currentTarget.dataset.unit;
    this.setData({ tempDropdownOpen: false });
    if (unit === this.data.tempUnit) return;
    this.setData({ tempUnit: unit });
    if (this._rawData) {
      this._processRawData();
    }
  },

  drawChart(touchPoint) {
    const data = this.data.chartData;
    if (!data || data.length === 0) {
      return;
    }

    const query = wx.createSelectorQuery().in(this);
    query.select('#historyCanvas')
      .fields({ node: true, size: true })
      .exec((res) => {
        if (!res || !res[0] || !res[0].node) {
          this._drawRetries = (this._drawRetries || 0) + 1;
          if (this._drawRetries > 10) {
            console.warn('[drawChart] Canvas 节点获取失败，已重试10次');
            return;
          }
          console.warn('[drawChart] Canvas 节点未找到，第', this._drawRetries, '次重试');
          setTimeout(() => this.drawChart(touchPoint), 30);
          return;
        }
        this._drawRetries = 0;
        const canvas = res[0].node;
        const W = this.data.canvasWidth;
        const H = this.data.canvasHeight;

        const dpr = wx.getSystemInfoSync().pixelRatio;

        canvas.width = W * dpr;
        canvas.height = H * dpr;

        const ctx = canvas.getContext('2d');
        ctx.scale(dpr, dpr);

        try {
          this.renderChart(ctx, data, W, H, touchPoint);
        } catch (e) {
          console.error('[drawChart] renderChart 异常:', e);
        }
      });
  },

  onCanvasTouch(e) {
    if (this.data.tempDropdownOpen) {
      this.setData({ tempDropdownOpen: false });
    }

    const touches = e.touches;

    // Pinch-to-zoom: 双指缩放
    if (touches.length >= 2) {
      this._handlePinch(e, touches);
      return;
    }

    // Single touch: 十字线 + 工具提示
    const meta = this._chartMeta;
    if (!meta) return;
    const touch = touches[0];
    if (!touch) return;

    const now = Date.now();
    if (e.type === 'touchmove' && this._lastTouchTime && now - this._lastTouchTime < 50) {
      this._pendingTouch = { e, touch };
      if (!this._touchTimer) {
        this._touchTimer = setTimeout(() => {
          this._touchTimer = null;
          if (this._pendingTouch) {
            this._processTouch(this._pendingTouch.e, this._pendingTouch.touch);
            this._pendingTouch = null;
          }
        }, 50);
      }
      return;
    }
    this._lastTouchTime = now;
    this._processTouch(e, touch);
  },

  _handlePinch(e, touches) {
    const x1 = touches[0].x, y1 = touches[0].y;
    const x2 = touches[1].x, y2 = touches[1].y;
    const dist = Math.sqrt((x2 - x1) ** 2 + (y2 - y1) ** 2);

    if (e.type === 'touchstart') {
      this._pinchStartDist = dist;
      this._pinchTriggered = false;
      return;
    }

    if (e.type === 'touchmove' && this._pinchStartDist) {
      const ratio = dist / this._pinchStartDist;
      if (!this._pinchTriggered && (ratio > 1.35 || ratio < 0.7)) {
        this._pinchTriggered = true;
        const ranges = RANGES.map(r => r.key);
        const curIdx = ranges.indexOf(this.data.selectedRange);
        if (ratio > 1.35 && curIdx > 0) {
          // 双指外扩 → 放大 → 更短时间范围
          this.setData({ selectedRange: ranges[curIdx - 1] });
          this.loadData();
        } else if (ratio < 0.7 && curIdx < ranges.length - 1) {
          // 双指内收 → 缩小 → 更长时间范围
          this.setData({ selectedRange: ranges[curIdx + 1] });
          this.loadData();
        }
      }
    }
  },

  _processTouch(e, touch) {
    const meta = this._chartMeta;
    if (!meta) return;

    const x = touch.x;
    const y = touch.y;

    let bestIdx = 0;
    let bestDist = Infinity;
    for (let i = 0; i < meta.N; i++) {
      const dist = Math.abs(meta.sx(i) - x);
      if (dist < bestDist) { bestDist = dist; bestIdx = i; }
    }

    if (x < meta.pad.l || x > meta.pad.l + meta.pw ||
        y < meta.pad.t || y > meta.pad.t + meta.ph) {
      this.drawChart();
      return;
    }

    this.drawChart({ idx: bestIdx, x, y });
  },

  onCanvasTouchEnd(e) {
    const wasPinch = this._pinchTriggered;
    this._pinchStartDist = 0;
    this._pinchTriggered = false;
    this._touchPending = null;
    if (this._touchTimer) {
      clearTimeout(this._touchTimer);
      this._touchTimer = null;
    }
    this._pendingTouch = null;
    this._lastTouchTime = 0;

    if (wasPinch) return;  // 缩放已触发，跳过单击逻辑

    const changedTouches = e.changedTouches;
    if (!changedTouches || changedTouches.length === 0) {
      this.drawChart();
      return;
    }
    const touch = changedTouches[0];
    const meta = this._chartMeta;

    // Check if tap on legend
    if (meta && touch) {
      const sensors = this._getSensors();
      const legX = meta.pad.l + meta.pw - 195;
      for (let i = 0; i < sensors.length; i++) {
        const offsetX = i * 120;
        if (touch.x >= legX + offsetX && touch.x <= legX + offsetX + 105 &&
            touch.y >= 6 && touch.y <= 26) {
          const showSensor = [...this.data.showSensor];
          showSensor[i] = !showSensor[i];
          this.setData({ showSensor }, () => this.drawChart());
          return;
        }
      }
    }

    this.drawChart();
  },

  renderChart(ctx, data, W, H, touchPoint) {
    const pad = { t: 20, r: 12, b: 38, l: 44 };
    const pw = W - pad.l - pad.r;
    const ph = H - pad.t - pad.b;

    const sensors = this._getSensors();
    const series = sensors.map(s => data.filter(d => d.sensor === s.measurement));

    const ref = series.reduce((a, b) => a.length >= b.length ? a : b, series[0]);
    const N = Math.max(ref.length, 1);

    const visible = [];
    sensors.forEach((s, i) => {
      if (this.data.showSensor[i]) visible.push(...series[i]);
    });
    const vals = visible.length > 0 ? visible.map(d => d.value) : data.map(d => d.value);
    const cleanVals = vals.filter(v => typeof v === 'number' && !isNaN(v));
    if (cleanVals.length === 0) {
      console.warn('[renderChart] 无有效数值，跳过绘制');
      return;
    }
    let minV = Math.min(...cleanVals), maxV = Math.max(...cleanVals);
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
      ctx.fillText(v.toFixed(1), pad.l - 6, y);
    }

    // Unit label at top of Y-axis
    const isTemp = this.data.selectedMetric === 'temp';
    const isF = isTemp && this.data.tempUnit === '°F';
    const displayUnit = isF ? '°F' : (metric && metric.unit) || '';
    ctx.fillStyle = '#999';
    ctx.font = '9px sans-serif';
    ctx.textAlign = 'center';
    ctx.textBaseline = 'bottom';
    ctx.fillText(displayUnit, pad.l - 6, pad.t - 10);

    // Threshold lines
    if (metric && metric.thresholds) {
      metric.thresholds.forEach(t => {
        const tVal = isF ? t.value * 9 / 5 + 32 : t.value;
        if (tVal < minV || tVal > maxV) return;
        const ty = sy(tVal);
        ctx.strokeStyle = t.color;
        ctx.lineWidth = 1;
        ctx.setLineDash([4, 4]);
        ctx.beginPath();
        ctx.moveTo(pad.l, ty);
        ctx.lineTo(pad.l + pw, ty);
        ctx.stroke();
        ctx.setLineDash([]);
        ctx.fillStyle = t.color;
        ctx.font = '9px sans-serif';
        ctx.textAlign = 'left';
        ctx.textBaseline = 'bottom';
        const tLabel = isF ? tVal.toFixed(1) + '°F' : t.label;
        ctx.fillText(tLabel, pad.l + pw + 4, ty);
      });
    }

    // X-axis labels
    const isLongRange = this.data.selectedRange === '24h' || this.data.selectedRange === '7d';
    const is30d = this.data.selectedRange === '30d';
    const labelMax = Math.min(5, N);
    const labelStep = Math.max(1, Math.ceil(N / labelMax));
    ctx.textAlign = 'center';
    ctx.textBaseline = 'top';

    const drawLabel = (i) => {
      const label = ref[i].displayTime;
      if (isLongRange) {
        const parts = label.split(' ');
        ctx.fillStyle = '#bbb';
        ctx.font = '9px sans-serif';
        ctx.fillText(parts[0] || '', sx(i), pad.t + ph + 6);
        ctx.fillStyle = '#999';
        ctx.font = '10px sans-serif';
        ctx.fillText(parts[1] || '', sx(i), pad.t + ph + 20);
      } else {
        ctx.fillStyle = '#999';
        ctx.font = '10px sans-serif';
        ctx.fillText(label, sx(i), pad.t + ph + 6);
      }
    };

    for (let i = 0; i < N; i += labelStep) {
      drawLabel(i);
    }
    if (labelMax > 1 && (N - 1) % labelStep !== 0 && N > 1) {
      const prevIdx = Math.floor((N - 1) / labelStep) * labelStep;
      if ((N - 1) - prevIdx >= labelStep * 0.4) {
        drawLabel(N - 1);
      }
    }

    // Draw one series (smooth bezier for 3+ points, straight for 2)
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
      const xs = (i) => sx(i * (N - 1) / Math.max(M - 1, 1));
      const ys = (i) => sy(series[i].value);
      ctx.moveTo(xs(0), ys(0));
      if (M === 2) {
        ctx.lineTo(xs(1), ys(1));
      } else {
        for (let i = 0; i < M - 1; i++) {
          const x0 = i > 0 ? xs(i - 1) : xs(0);
          const y0 = i > 0 ? ys(i - 1) : ys(0);
          const x1 = xs(i);
          const y1 = ys(i);
          const x2 = xs(i + 1);
          const y2 = ys(i + 1);
          const x3 = i + 2 < M ? xs(i + 2) : xs(M - 1);
          const y3 = i + 2 < M ? ys(i + 2) : ys(M - 1);
          const cp1x = x1 + (x2 - x0) / 6;
          const cp1y = y1 + (y2 - y0) / 6;
          const cp2x = x2 - (x3 - x1) / 6;
          const cp2y = y2 - (y3 - y1) / 6;
          ctx.bezierCurveTo(cp1x, cp1y, cp2x, cp2y, x2, y2);
        }
      }
      ctx.stroke();
    };

    sensors.forEach((s, i) => {
      if (this.data.showSensor[i]) {
        drawSeries(series[i], s.color);
      }
    });

    // Legend with counts (tap to toggle)
    const legX = pad.l + pw - 195;
    ctx.font = '11px sans-serif';
    ctx.textAlign = 'left';
    ctx.textBaseline = 'top';
    sensors.forEach((s, i) => {
      const offsetX = i * 120;
      ctx.fillStyle = this.data.showSensor[i] ? s.color : '#ccc';
      ctx.fillRect(legX + offsetX, 8, 14, 10);
      ctx.fillStyle = this.data.showSensor[i] ? '#333' : '#ccc';
      ctx.fillText(s.label + '(' + series[i].length + ')', legX + offsetX + 18, 8);
    });

    // Save metadata for touch handler
    this._chartMeta = { series, ref, N, pad, sx, sy, pw, ph, metric };

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

      // Tooltip box - find each sensor's value at closest time to the touched point
      const refTime = new Date(ref[idx].time).getTime();
      const findClosest = (s) => {
        if (!s || s.length === 0) return null;
        let best = s[0];
        let bestDist = Math.abs(new Date(s[0].time).getTime() - refTime);
        for (let i = 1; i < s.length; i++) {
          const dist = Math.abs(new Date(s[i].time).getTime() - refTime);
          if (dist < bestDist) { bestDist = dist; best = s[i]; }
        }
        return best.value;
      };
      const sensorVals = series.map(findClosest);

      const unit = (metric && metric.unit) || '';
      const lines = [t.displayTime];
      sensors.forEach((s, i) => {
        const val = sensorVals[i];
        lines.push(s.label + ': ' + (val !== null ? val.toFixed(1) + unit : '--'));
      });
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
  },

  onExportCsv() {
    const { tableData, metrics, selectedRange } = this.data;
    if (!tableData || tableData.length === 0) return;

    const exportMetrics = metrics
      .filter(m => m.supported)
      .map(m => ({ ...m, checked: m.key === this.data.selectedMetric }));
    this.setData({ exportModalOpen: true, exportMetrics, exportRange: selectedRange });
  },

  onExportMetricToggle(e) {
    const idx = e.currentTarget.dataset.index;
    const exportMetrics = [...this.data.exportMetrics];
    exportMetrics[idx].checked = !exportMetrics[idx].checked;
    this.setData({ exportMetrics });
  },

  onExportSelectAll() {
    const exportMetrics = this.data.exportMetrics.map(m => ({ ...m, checked: true }));
    this.setData({ exportMetrics });
  },

  onExportCancel() {
    this.setData({ exportModalOpen: false });
  },

  onExportRangeTap(e) {
    this.setData({ exportRange: e.currentTarget.dataset.key });
  },

  onExportModalNoop() {},

  onExportConfirm() {
    const selected = this.data.exportMetrics.filter(m => m.checked);
    if (selected.length === 0) {
      wx.showToast({ title: '请至少选择一个指标', icon: 'none' });
      return;
    }

    this.setData({ exporting: true });
    const app = getApp();
    const range = this.data.exportRange;
    let completed = 0;
    const allData = [];

    selected.forEach(m => {
      app.fetchHistory(range, m.key, (err, data) => {
        completed++;
        if (!err && data && data.length > 0) {
          allData.push({ metric: m, data: data });
        }
        if (completed === selected.length) {
          this._generateExportCsv(allData);
        }
      });
    });
  },

  _generateExportCsv(metricDataList) {
    this.setData({ exporting: false, exportModalOpen: false });

    if (metricDataList.length === 0) {
      wx.showToast({ title: '无数据可导出', icon: 'none' });
      return;
    }

    const { exportRange } = this.data;

    const sensors = this._getSensors();
    const timeMap = {};
    metricDataList.forEach(({ metric, data }) => {
      data.forEach(d => {
        if (!timeMap[d.displayTime]) {
          timeMap[d.displayTime] = { displayTime: d.displayTime };
        }
        sensors.forEach(s => {
          if (d.sensor === s.measurement) {
            timeMap[d.displayTime][`${s.label}_${metric.key}`] = d.value;
          }
        });
      });
    });

    const times = Object.keys(timeMap).sort();
    const columns = [];
    metricDataList.forEach(({ metric }) => {
      sensors.forEach(s => {
        columns.push(`${s.label} ${metric.label}(${metric.unit})`);
      });
    });

    let csv = '\uFEFF时间,' + columns.join(',') + '\n';
    times.forEach(t => {
      const row = timeMap[t];
      const vals = [];
      metricDataList.forEach(({ metric }) => {
        sensors.forEach(s => {
          const key = `${s.label}_${metric.key}`;
          const v = row[key];
          vals.push(v !== undefined && v !== null && !isNaN(v) ? v : '');
        });
      });
      csv += row.displayTime + ',' + vals.join(',') + '\n';
    });

    const fs = wx.getFileSystemManager();
    const metricKeys = metricDataList.map(m => m.metric.key).join('_');
    const fileName = `sensor_${metricKeys}_${exportRange}_${Date.now()}.csv`;
    const filePath = `${wx.env.USER_DATA_PATH}/${fileName}`;
    fs.writeFileSync(filePath, csv, 'utf8');

    wx.shareFileMessage({
      filePath: filePath,
      fileName: fileName,
      success: () => {},
      fail: (err) => {
        wx.showToast({ title: '导出失败: ' + (err.errMsg || ''), icon: 'none' });
      }
    });
  }
});