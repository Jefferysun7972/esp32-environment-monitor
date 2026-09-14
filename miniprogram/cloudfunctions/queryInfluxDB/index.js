const cloud = require('wx-server-sdk');

cloud.init({ env: cloud.DYNAMIC_CURRENT_ENV });

// ── InfluxDB 配置 ──
// 安全：凭证仅存在于云函数环境，不会分发到客户端
// 部署后在微信云开发控制台 → 云函数 → queryInfluxDB → 环境变量 中设置
const INFLUXDB_URL = process.env.INFLUXDB_URL || '';
const INFLUXDB_ORG = process.env.INFLUXDB_ORG || '';
const INFLUXDB_TOKEN = process.env.INFLUXDB_TOKEN || '';

function httpPost(url, headers, body, timeout) {
  return new Promise((resolve, reject) => {
    const https = require('https');
    const { URL } = require('url');
    const parsed = new URL(url);

    const options = {
      hostname: parsed.hostname,
      port: parsed.port || 443,
      path: parsed.pathname + parsed.search,
      method: 'POST',
      headers: Object.assign({}, headers, {
        'Content-Length': Buffer.byteLength(body)
      }),
      timeout: timeout || 30000
    };

    const req = https.request(options, (res) => {
      let data = '';
      res.on('data', chunk => { data += chunk; });
      res.on('end', () => {
        resolve({ statusCode: res.statusCode, data: data, headers: res.headers });
      });
    });

    req.on('error', (e) => reject(e));
    req.on('timeout', () => { req.destroy(); reject(new Error('请求超时')); });
    req.write(body);
    req.end();
  });
}

exports.main = async (event, context) => {
  const { query, org, timeout } = event;

  if (!INFLUXDB_URL || !INFLUXDB_TOKEN) {
    return { success: false, error: '云函数未配置 InfluxDB 凭证，请在环境变量中设置 INFLUXDB_URL / INFLUXDB_ORG / INFLUXDB_TOKEN' };
  }

  if (!query) {
    return { success: false, error: '缺少查询参数 query' };
  }

  const orgParam = org || INFLUXDB_ORG;
  const url = INFLUXDB_URL + '/api/v2/query?org=' + encodeURIComponent(orgParam);

  try {
    const response = await httpPost(url, {
      'Authorization': 'Token ' + INFLUXDB_TOKEN,
      'Content-Type': 'application/vnd.flux',
      'Accept': 'application/csv'
    }, query, timeout || 30000);

    if (response.statusCode >= 200 && response.statusCode < 300) {
      return { success: true, data: response.data };
    } else {
      return {
        success: false,
        statusCode: response.statusCode,
        error: 'InfluxDB 返回错误: ' + response.statusCode,
        data: response.data
      };
    }
  } catch (err) {
    return { success: false, error: err.message || '网络请求失败' };
  }
};