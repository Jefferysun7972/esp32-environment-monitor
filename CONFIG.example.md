# 🔐 敏感信息配置指南

**重要提示**：本项目包含需要您自行配置的敏感信息占位符。在编译和运行前，请务必替换以下配置！

---

## 📋 需要配置的文件清单

### 1. **ESP32 固件 (blink)**

#### 📶 WiFi 配置
**文件**: `components/wifi_web/wifi_web.c`

```c
#define WIFI_SSID      "YOUR_WIFI_SSID"        // ← 替换为您的 WiFi 名称
#define WIFI_PASS      "YOUR_WIFI_PASSWORD"    // ← 替换为您的 WiFi 密码
```

**示例**:
```c
#define WIFI_SSID      "MyHomeNetwork"
#define WIFI_PASS      "MySecurePassword123"
```

---

#### 🌐 MQTT Broker 配置
**文件**: `components/mqtt_cloud/mqtt_cloud.c`

```c
#define MQTT_BROKER_URI  "mqtts://YOUR_MQTT_BROKER:8883"  // ← 替换为您的 MQTT 服务器地址
#define MQTT_USERNAME    "YOUR_MQTT_USERNAME"              // ← 替换为用户名
#define MQTT_PASSWORD    "YOUR_MQTT_PASSWORD"              // ← 替换为密码
```

**示例** (EMQX Cloud):
```c
#define MQTT_BROKER_URI  "mqtts://your-instance.emqxsl.cn:8883"
#define MQTT_USERNAME    "your_username"
#define MQTT_PASSWORD    "your_secure_password"
```

**常用 MQTT 服务商**:
- EMQX Cloud: `mqtts://xxx.emqxsl.cn:8883`
- Mosquitto: `mqtt://your-server:1883`
- HiveMQ: `mqtts://xxx.hivemq.cloud:8883`

---

#### 📊 InfluxDB 配置
**文件**: `components/influxdb_writer/influxdb_writer.c`

```c
#define INFLUXDB_URL    "https://YOUR_INFLUXDB_URL"     // ← InfluxDB Cloud URL
#define INFLUXDB_ORG    "YOUR_ORG_NAME"                 // ← 组织名称
#define INFLUXDB_BUCKET "sensor_data"                    // ✅ 可保持默认
#define INFLUXDB_TOKEN  "YOUR_INFLUXDB_TOKEN"           // ← API Token（非常重要！）
```

**示例** (InfluxDB Cloud):
```c
#define INFLUXDB_URL    "https://us-east-1-1.aws.cloud2.influxdata.com"
#define INFLUXDB_ORG    "my-org@company.com"
#define INFLUXDB_BUCKET "sensor_data"
#define INFLUXDB_TOKEN  "your-token-here-xxxxxxxxxxxxxxxxxxxxxx=="
```

**获取 Token 步骤**:
1. 登录 [InfluxDB Cloud](https://cloud2.influxdata.com/)
2. 进入 **Data > Tokens**
3. 点击 **Generate API Token**
4. 选择权限：**Read/Write**
5. 复制生成的 Token

---

### 2. **微信小程序 (miniprogram)**

#### 📊 InfluxDB 查询配置
**文件**: `miniprogram/app.js`

```javascript
const INFLUXDB_URL = 'https://YOUR_INFLUXDB_URL';   // ← 与 ESP32 相同的 URL
const INFLUXDB_ORG = 'YOUR_ORG_NAME';               // ← 与 ESP32 相同的组织名
const INFLUXDB_TOKEN = 'YOUR_INFLUXDB_TOKEN';       // ← 与 ESP32 相同的 Token
```

**⚠️ 重要**: 小程序中的配置必须与 ESP32 固件中的 **完全一致**！

---

## 🔒 安全最佳实践

### ✅ 推荐做法

1. **不要将真实凭证提交到 Git**
   ```bash
   # 确保已替换所有占位符
   grep -r "YOUR_" components/ miniprogram/
   # 应该只显示占位符，不应有真实值
   ```

2. **使用环境变量（高级）**
   - 创建 `.env` 文件（不提交到 Git）
   - 在编译时读取环境变量
   
3. **定期轮换密钥**
   - 每 90 天更换一次密码和 Token
   - 使用强密码（至少 16 位，包含大小写字母、数字、特殊字符）

4. **限制权限**
   - MQTT: 只授予必要的 topic 权限
   - InfluxDB: 使用最小权限原则（Read/Write 即可）

### ❌ 绝对避免

- ❌ 将真实密码硬编码在代码中并推送到公开仓库
- ❌ 在日志中打印敏感信息
- ❌ 使用简单密码（如 "12345678", "password"）
- ❌ 在多个服务中使用相同的密码

---

## 🛠️ 配置验证

完成配置后，使用以下命令验证：

### ESP32 端
```bash
# 编译项目
idf.py build

# 检查是否有未替换的占位符
grep -rn "YOUR_" build/
# 如果有输出，说明还有遗漏！
```

### 小程序端
```bash
# 检查 app.js
grep "INFLUXDB_URL\|INFLUXDB_TOKEN" miniprogram/app.js
# 应显示 YOUR_ 开头的占位符
```

---

## 🆘 常见问题

### Q: 忘记替换会怎样？
**A**: 
- WiFi: 无法连接网络
- MQTT: 数据无法上传到云端
- InfluxDB: 历史数据无法存储和查询

### Q: 如何测试配置是否正确？
**A**: 
1. 先用 MQTT 客户端工具（如 MQTTX）测试连接
2. 使用 InfluxDB CLI 测试写入：
   ```bash
   influx write 'sensor_data,device=test temp=25.5'
   ```
3. 检查小程序是否能查询到数据

### Q: 可以使用本地 InfluxDB 吗？
**A**: 可以！将 URL 改为：
```c
#define INFLUXDB_URL "http://192.168.1.100:8086"  // 本地 IP
```

---

## 📞 需要帮助？

如果遇到配置问题，请查看：
- [ESP-IDF 官方文档](https://docs.espressif.com/projects/esp-idf/)
- [InfluxDB 文档](https://docs.influxdata.com/influxdb/cloud/reference/api/)
- [EMQX 文档](https://docs.emqx.com/)

---

## ✅ 配置检查清单

在首次运行前，请确认已完成以下所有步骤：

- [ ] 替换 WiFi SSID 和密码
- [ ] 替换 MQTT Broker 地址、用户名、密码
- [ ] 替换 InfluxDB URL、组织名、Token
- [ ] 确保 ESP32 和小程序使用相同的 InflxDB 配置
- [ ] 测试 WiFi 连接
- [ ] 测试 MQTT 连接
- [ ] 测试 InfluxDB 写入
- [ ] 测试小程序数据查询

**全部完成后，即可正常使用本系统！** 🎉

---

**最后更新**: 2026-09-10  
**版本**: v1.4.0  
**维护者**: ESP32 Environment Monitor Team