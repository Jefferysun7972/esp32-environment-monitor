# ⚡ 快速开始：本地开发配置（30秒搞定）

**适用场景**：你只想快速配置好凭证，立即开始编译和开发

---

## 📱 配置说明

本项目包含 **两个独立的部分**，需要分别配置：

| 部分 | 配置文件 | 用途 |
|------|---------|------|
| **ESP32 固件** | `credentials.local.h` | WiFi / MQTT / InfluxDB |
| **微信小程序** | `miniprogram/config.local.js` | InfluxDB 查询 |

✅ **好消息**：运行一次配置向导即可同时配置两者！

---

## 🎯 一键配置（推荐）

```bash
# 进入项目目录
cd /Users/jefferysun/esp32/blink

# 运行配置向导（交互式，跟着提示走即可）
bash scripts/setup-credentials.sh

# 编译测试
idf.py build

# 完成！✅
```

---

## 📝 手动配置（2分钟）

### Step 1: 创建配置文件

```bash
cp credentials.local.h.example credentials.local.h
```

### Step 2: 编辑配置文件

```bash
# 使用 VS Code 打开（推荐）
code credentials.local.h

# 或使用 nano
nano credentials.local.h

# 或使用 vim
vim credentials.local.h
```

### Step 3: 填写以下内容

找到所有 `YOUR_XXX` 并替换为真实值：

```c
// ===== WiFi 配置 =====
#define LOCAL_WIFI_SSID      "你的WiFi名称"          // 例: "HUAWEI-5FEC"
#define LOCAL_WIFI_PASS      "你的WiFi密码"          // 例: "your_password"

// ===== MQTT 配置 (EMQX Cloud) =====
#define LOCAL_MQTT_BROKER_URI  "mqtts://你的Broker:8883"  // 从 EMQX 控制台复制
#define LOCAL_MQTT_USERNAME    "你的MQTT用户名"
#define LOCAL_MQTT_PASSWORD    "你的MQTT密码"

// ===== InfluxDB Cloud 配置 =====
#define LOCAL_INFLUXDB_URL    "https://你的URL.cloud2.influxdata.com"  // 从 InfluxDB 控制台复制
#define LOCAL_INFLUXDB_HOST   "你的URL.cloud2.influxdata.com"           // 同上，去掉 https://
#define LOCAL_INFLUXDB_ORG    "你的组织名"           // 例: "Fellowes"
#define LOCAL_INFLUXDB_BUCKET "sensor_data"          // 通常不改
#define LOCAL_INFLUXDB_TOKEN  "你的API Token"        // 很长的字符串，从 InfluxDB 控制板复制
```

### Step 4: 保存并编译

```bash
# 保存文件后
idf.py build

# 如果成功，说明配置正确！🎉
```

---

### Step 4 (可选): 配置微信小程序

如果你需要使用微信小程序查看数据：

```bash
# 从模板创建小程序配置文件
cp miniprogram/config.local.example.js miniprogram/config.local.js

# 编辑配置文件
code miniprogram/config.local.js
```

填写 InfluxDB 信息（与 ESP32 相同）：

```javascript
const INFLUXDB_URL = 'https://us-east-1-1.aws.cloud2.influxdata.com';
const INFLUXDB_ORG = 'Fellowes';
const INFLUXDB_TOKEN = '你的_InfluxDB_Token';
```

**注意**：小程序会自动加载此配置，无需修改 `app.js`！

---

## ✅ 验证配置是否生效

### 方法 1：查看编译输出

```bash
idf.py build 2>&1 | grep -E "(credentials|YOUR_|LOCAL_)"
```

**期望输出**：
- 如果看到 `🔧 WiFi: 使用 credentials.local.h 中的配置` → ✅ 成功！
- 如果看到 `⚠️ WiFi: 使用默认占位符` → ❌ 配置未生效

### 方法 2：运行安全检查

```bash
bash scripts/security-check.sh
```

---

## 🔧 常见问题

### Q1: 忘记 WiFi 密码怎么办？

**A**: 
- 查看路由器设置页面
- 或询问网络管理员
- 或重置路由器（会重置密码）

### Q2: InfluxDB Token 在哪里找？

**A**:
1. 登录 https://cloud2.influxdata.com
2. 左侧菜单 → **Data** → **Tokens**
3. 点击 **Generate API Token**
4. 选择 **Read/Write Token**
5. 复制生成的 Token（100+ 字符）

### Q3: MQTT Broker URI 格式？

**A**:
- 格式：`mqtts://host:port`
- 例：`mqtts://your-deployment.emqxsl.cn:8883`
- 端口：MQTTS=8883, MQTT=1883

### Q4: 配置错了怎么办？

**A**: 重新编辑 `credentials.local.h` 即可，无需其他操作。

### Q5: 多人协作怎么处理？

**A**: 每人维护自己的 `credentials.local.h`（不提交到 Git），共享 `credentials.local.h.example` 模板。

---

## 📚 需要更多帮助？

- 📖 **完整文档**: [LOCAL_DEVELOPMENT.md](LOCAL_DEVELOPMENT.md) （详细教程）
- 🔒 **安全说明**: [SECURITY.md](SECURITY.md)
- ⚙️ **配置指南**: [CONFIG.example.md](CONFIG.example.md)
- 🛡️ **安全检查**: `bash scripts/security-check.sh`

---

## 💡 提示

✅ **一次配置，永久使用** - 之后每次编译自动读取  
✅ **安全隔离** - 凭证文件不会被提交到 Git  
✅ **团队友好** - 每人独立配置，互不影响  

**现在就开始配置吧！** 🚀

---

*需要更详细的说明？查看 [LOCAL_DEVELOPMENT.md](LOCAL_DEVELOPMENT.md)*