#ifndef WEB_DASHBOARD_H
#define WEB_DASHBOARD_H

static const char INDEX_HTML[] = R"=====(
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>ESP32 Environment Monitor</title>
<style>
*{margin:0;padding:0;box-sizing:border-box}
body{font-family:-apple-system,BlinkMacSystemFont,'Segoe UI',sans-serif;background:#0d1117;color:#c9d1d9;min-height:100vh}
.header{background:#161b22;border-bottom:1px solid #30363d;padding:16px 20px;text-align:center}
.header h1{font-size:20px;color:#58a6ff;margin-bottom:4px}
.header .sub{font-size:12px;color:#8b949e}
.header .ip{font-size:11px;color:#484f58;margin-top:2px}
.container{max-width:680px;margin:0 auto;padding:16px}
.alert-bar{padding:10px 16px;border-radius:8px;margin-bottom:16px;font-weight:600;text-align:center;font-size:14px;display:none}
.alert-bar.show{display:block}
.alert-normal{background:#0d3320;color:#3fb950;border:1px solid #238636}
.alert-warning{background:#332b0d;color:#d2991d;border:1px solid #9e6a03}
.alert-danger{background:#330d0d;color:#f85149;border:1px solid #da3633}
.comp-table{display:grid;grid-template-columns:1fr 1fr;gap:1px;background:#30363d;border-radius:8px;overflow:hidden;margin-bottom:16px}
.comp-table .col-header{padding:10px 12px;font-size:13px;font-weight:700;text-align:center}
.comp-table .col-header.am{background:#1a2332;color:#58a6ff}
.comp-table .col-header.sen{background:#1a2332;color:#3fb950}
.comp-table .row-label{background:#161b22;padding:10px 12px;font-size:12px;color:#8b949e;font-weight:600;display:flex;align-items:center}
.comp-table .row-label .unit{font-size:10px;color:#484f58;margin-left:4px}
.comp-table .val{background:#161b22;padding:10px 12px;font-size:14px;font-weight:700;text-align:right;font-variant-numeric:tabular-nums}
.comp-table .val.good{color:#3fb950}
.comp-table .val.warn{color:#d2991d}
.comp-table .val.bad{color:#f85149}
.comp-table .val.na{color:#484f58;font-weight:400}
.bar-section{margin-bottom:16px}
.bar-section h3{font-size:13px;color:#8b949e;margin-bottom:8px}
.bar-row{display:flex;align-items:center;margin-bottom:6px;gap:8px}
.bar-row .label{font-size:11px;color:#8b949e;width:72px;text-align:right;flex-shrink:0}
.bar-row .bar-wrap{flex:1;background:#21262d;border-radius:4px;height:18px;position:relative;overflow:hidden}
.bar-row .bar-fill{height:100%;border-radius:4px;transition:width .5s ease;min-width:2px}
.bar-row .bar-fill.am{background:linear-gradient(90deg,#1f6feb,#58a6ff)}
.bar-row .bar-fill.sen{background:linear-gradient(90deg,#238636,#3fb950)}
.bar-row .bar-val{font-size:11px;color:#c9d1d9;width:48px;text-align:right;flex-shrink:0;font-variant-numeric:tabular-nums}
.footer{text-align:center;padding:12px;font-size:10px;color:#484f58;border-top:1px solid #21262d;margin-top:16px}
.footer .dot{display:inline-block;width:6px;height:6px;border-radius:50%;background:#3fb950;margin-right:4px;animation:pulse 1.5s infinite}
@keyframes pulse{0%,100%{opacity:1}50%{opacity:.3}}
.refresh{font-size:10px;color:#484f58}
</style>
</head>
<body>
<div class="header">
  <h1>ESP32 Environment Monitor</h1>
  <div class="sub">AM2020DY vs <span id="senName">SEN68</span></div>
  <div class="ip" id="ipAddr"></div>
</div>
<div class="container">
  <div class="alert-bar" id="alertBar"></div>
  <div class="comp-table" id="compTable"></div>
  <div class="bar-section">
    <h3>PM2.5 Comparison</h3>
    <div class="bar-row">
      <span class="label">AM2020DY</span>
      <div class="bar-wrap"><div class="bar-fill am" id="barAm" style="width:0%"></div></div>
      <span class="bar-val" id="barAmVal">--</span>
    </div>
    <div class="bar-row">
      <span class="label" id="senLabel">SEN68</span>
      <div class="bar-wrap"><div class="bar-fill sen" id="barSen" style="width:0%"></div></div>
      <span class="bar-val" id="barSenVal">--</span>
    </div>
  </div>
  <div class="refresh">Auto-refresh every 5s &middot; <span class="dot"></span> Live</div>
</div>
<div class="footer">ESP32 Environment Monitor &middot; v1.1.0</div>
<script>
const THRESHOLDS = {
  temp:  { ok:[18,26], warn:[10,35] },
  humi:  { ok:[40,70], warn:[20,90] },
  pm1:   { ok:25, warn:50 },
  pm25:  { ok:35, warn:75 },
  pm10:  { ok:50, warn:100 },
  tvoc:  { ok:150, warn:300 },
  nox:   { ok:100, warn:200 },
  co2:   { ok:800, warn:1200 },
  hcho:  { ok:80, warn:150 },
  no2:   { ok:50, warn:100 }
};

function cls(val,key){
  if(val<0||val==0&&key!='temp')return'na';
  var t=THRESHOLDS[key];
  if(!t)return'good';
  if(Array.isArray(t.ok)){
    if(val>=t.ok[0]&&val<=t.ok[1])return'good';
    if(val>=t.warn[0]&&val<=t.warn[1])return'warn';
    return'bad';
  }
  if(val<=t.ok)return'good';
  if(val<=t.warn)return'warn';
  return'bad';
}

function fmt(v,d){return v>=0?v.toFixed(d||1):'--'}

var PM25_MAX=200;

function build(){
  var d=window._data;
  if(!d)return;
  document.getElementById('senName').textContent=d.sen_ready?d.sen_name:'(not detected)';
  document.getElementById('senLabel').textContent=d.sen_ready?d.sen_name:'SEN';
  document.getElementById('ipAddr').textContent='IP: '+d.ip;

  var a=document.getElementById('alertBar');
  a.className='alert-bar show';
  a.textContent=d.alert_msg||'Normal';
  if(d.alert_level==0)a.classList.add('alert-normal');
  else if(d.alert_level==1)a.classList.add('alert-warning');
  else a.classList.add('alert-danger');

  var rows=[
    ['Temperature','temp','°C',d.am2020dy_temp,d.sen_ready?d.sen_temp:-1],
    ['Humidity','humi','%',d.am2020dy_humi,d.sen_ready?d.sen_humi:-1],
    ['PM1.0','pm1','µg/m³',d.am2020dy_pm1,d.sen_ready?d.sen_pm1:-1],
    ['PM2.5','pm25','µg/m³',d.am2020dy_pm25,d.sen_ready?d.sen_pm25:-1],
    ['PM10','pm10','µg/m³',d.am2020dy_pm10,d.sen_ready?d.sen_pm10:-1],
    ['TVOC','tvoc','idx',d.am2020dy_tvoc,d.sen_ready?d.sen_tvoc:-1]
  ];
  if(d.sen_ready&&d.sen_nox>=0) rows.push(['NOx','nox','idx',d.am2020dy_no2>=0?d.am2020dy_no2:-1,d.sen_nox]);
  else rows.push(['NO2','no2','ppb',d.am2020dy_no2,-1]);
  if(d.sen_ready&&d.sen_co2>=0) rows.push(['CO2','co2','ppm',-1,d.sen_co2]);
  if(d.sen_ready&&d.sen_hcho>=0) rows.push(['HCHO','hcho','ppb',d.am2020dy_hcho,d.sen_hcho]);

  var h='<div class="col-header am">AM2020DY</div><div class="col-header sen">'+(d.sen_ready?d.sen_name:'SEN')+'</div>';
  rows.forEach(function(r){
    h+='<div class="row-label">'+r[0]+'<span class="unit">'+r[2]+'</span></div>';
    h+='<div class="val '+cls(r[3],r[1])+'">'+fmt(r[3])+'</div>';
    h+='<div class="val '+cls(r[4],r[1])+'">'+fmt(r[4])+'</div>';
  });
  document.getElementById('compTable').innerHTML=h;

  var pm25a=d.am2020dy_pm25>0?d.am2020dy_pm25:0;
  var pm25s=(d.sen_ready&&d.sen_pm25>0)?d.sen_pm25:0;
  var maxPm=Math.max(pm25a,pm25s,PM25_MAX);
  document.getElementById('barAm').style.width=(pm25a/maxPm*100)+'%';
  document.getElementById('barSen').style.width=(pm25s/maxPm*100)+'%';
  document.getElementById('barAmVal').textContent=fmt(pm25a);
  document.getElementById('barSenVal').textContent=fmt(pm25s);
}

function fetchData(){
  var x=new XMLHttpRequest();
  x.onload=function(){
    try{window._data=JSON.parse(x.responseText);build()}catch(e){}
  };
  x.open('GET','/api/sensors');
  x.send();
}

fetchData();
setInterval(fetchData,5000);
</script>
</body>
</html>
)=====";

#endif