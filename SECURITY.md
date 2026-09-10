# 🔐 ESP32 Environment Monitor - 安全使用指南

## 📌 目录

- [安全特性](#-安全特性)
- [文件结构](#-文件结构)
- [快速开始](#-快速开始)
- [配置凭证](#-配置凭证)
- [安全检查](#-安全检查)
- [最佳实践](#-最佳实践)
- [常见问题](#-常见问题)

---

## ✨ 安全特性

### 🔒 已实施的安全措施

| 特性 | 描述 | 状态 |
|------|------|------|
| **敏感信息替换** | 所有真实凭证已替换为占位符 | ✅ 完成 |
| **.gitignore 规则** | 防止敏感文件被意外提交 | ✅ 已配置 |
| **自动化检查脚本** | 提交前自动检测硬编码凭证 | ✅ 可用 |
| **配置文档** | 详细的配置指南和示例 | ✅ 已提供 |
| **占位符标准化** | 统一使用 `YOUR_XXX` 格式 | ✅ 一致 |

---

## 📁 文件结构

```
esp32-environment-monitor/
├── .gitignore                    # Git 忽略规则（防止泄露）
├── CONFIG.example.md             # 详细配置指南
├── SECURITY.md                   # 本文件（安全说明）
├── scripts/
│   └── security-check.sh         # 自动化安全检查
│
├── components/                   # ESP32 固件代码
│   ├── wifi_web/
│   │   └── wifi_web.c           # WiFi 配置 (YOUR_WIFI_*)
│   ├── mqtt_cloud/
│   │   └── mqtt_cloud.c         # MQTT 配置 (YOUR_MQTT_*)
│   └── influxdb_writer/
│       └── influxdb_writer.c    # InfluxDB 配置 (YOUR_INFLUXDB_*)
│
└── miniprogram/                  # 微信小程序代码
    └── app.js                    # InfluxDB 查询配置 (YOUR_INFLUXDB_*)
```

---

## 🚀 快速开始

### 1️⃣ 克隆项目

```bash
git clone https://github.com/YOUR_USERNAME/esp32-environment-monitor.git
cd esp32-environment-monitor
```

### 2️⃣ 运行安全检查（推荐）

```bash
chmod +x scripts/security-check.sh
./scripts/security-check.sh
```

**预期输出**：
```
✅ 所有检查通过！代码安全，可以提交
🎉 准备提交...
```

### 3️⃣ 配置凭证

参考 [CONFIG.example.md](CONFIG.example.md) 填写真实凭证。

### 4️⃣ 编译和运行

```bash
# ESP32 固件
idf.py build
idf.py flash

# 微信小程序
# 在微信开发者工具中打开 miniprogram 目录
```

---

## 🔑 配置凭证

### 需要配置的占位符清单

#### ESP32 固件 (blink)

| 文件 | 占位符 | 说明 | 示例值 |
|------|--------|------|--------|
| `wifi_web.c` | `YOUR_WIFI_SSID` | WiFi 名称 | `"MyHomeNetwork"` |
| `wifi_web.c` | `YOUR_WIFI_PASSWORD` | WiFi 密码 | `"MySecurePassword123"` |
| `mqtt_cloud.c` | `YOUR_MQTT_BROKER` | MQTT 服务器地址 | `"mqtts://xxx.emqxsl.cn:8883"` |
| `mqtt_cloud.c` | `YOUR_MQTT_USERNAME` | MQTT 用户名 | `"your_username"` |
| `mqtt_cloud.c` | `YOUR_MQTT_PASSWORD` | MQTT 密码 | `"your_password"` |
| `influxdb_writer.c` | `YOUR_INFLUXDB_URL` | InfluxDB URL | `"us-east-1-1.aws.cloud2.influxdata.com"` |
| `influxdb_writer.c` | `YOUR_INFLUXDB_HOST` | InfluxDB 主机名 | `"us-east-1-1.aws.cloud2.influxdata.com"` |
| `influxdb_writer.c` | `YOUR_ORG_NAME` | 组织名称 | `"my-org@company.com"` |
| `influxdb_writer.c` | `YOUR_INFLUXDB_TOKEN` | API Token | `"your-token-here..."` |

#### 微信小程序 (miniprogram)

| 文件 | 占位符 | 说明 | 重要提示 |
|------|--------|------|----------|
| `app.js` | `YOUR_INFLUXDB_URL` | InfluxDB URL | **必须与 ESP32 相同** |
| `app.js` | `YOUR_ORG_NAME` | 组织名称 | **必须与 ESP32 相同** |
| `app.js` | `YOUR_INFLUXDB_TOKEN` | API Token | **必须与 ESP32 相同** |

> ⚠️ **重要**: ESP32 和小程序的 InfluxDB 配置必须完全一致！

---

## 🔍 安全检查

### 方法 1：手动运行脚本

```bash
./scripts/security-check.sh
```

### 方法 2：作为 Git Hook 自动运行

```bash
# 安装为 pre-commit hook
cp scripts/security-check.sh .git/hooks/pre-commit
chmod +x .git/hooks/pre-commit

# 现在每次 git commit 都会自动运行安全检查
```

### 安全检查内容

✅ **检查项**:
1. `.gitignore` 是否存在且包含关键规则
2. 是否有未忽略的敏感文件（.env, credentials 等）
3. 代码中是否包含硬编码的真实凭证
4. 占位符是否正确使用
5. Git 暂存区是否包含敏感信息

---

## 💡 最佳实践

### ✅ 推荐做法

#### 1. 使用环境变量（高级用户）

创建 `.env.local` 文件（不会被提交）：

```bash
# .env.local (不要提交到 Git!)
WIFI_SSID="MyWiFi"
WIFI_PASS="MyPassword"
MQTT_BROKER="mqtts://xxx.emqxsl.cn:8883"
INFLUXDB_TOKEN="your-token-here"
```

#### 2. 定期轮换密钥

```bash
# 建议每 90 天更换一次：
- WiFi 密码
- MQTT 密码
- InfluxDB Token
```

#### 3. 使用强密码

```
✅ 正确示例:
- "Xk9#mP2$vL5@nQ8!"
- "MySecureESP32-2026!"

❌ 错误示例:
- "password"
- "12345678"
- "admin"
```

#### 4. 最小权限原则

- MQTT: 只授予必要的 topic 权限
- InfluxDB: 使用 Read/Write 权限（不要用 All Access）
- WiFi: 使用独立的 IoT 网络（如果可能）

### ❌ 绝对避免

- ❌ 将填写后的真实凭证提交到 Git
- ❌ 在日志中打印敏感信息
- ❌ 在多个服务中使用相同密码
- ❌ 使用简单或常见密码
- ❌ 将凭证硬编码在代码中并推送到公开仓库

---

## ❓ 常见问题

### Q1: 忘记替换占位符会怎样？

**A**: 
- WiFi: 无法连接网络 ❌
- MQTT: 数据无法上传到云端 ❌
- InfluxDB: 无法存储和查询历史数据 ❌

**解决方法**: 运行 `./scripts/security-check.sh` 检查是否还有占位符。

---

### Q2: 如何验证配置是否正确？

**A**: 

1. **测试 WiFi 连接**:
   ```bash
   idf.py monitor
   # 查看 WiFi 连接日志
   ```

2. **测试 MQTT**:
   ```bash
   # 使用 MQTTX 或类似工具连接到你的 Broker
   # 订阅 topic: sensor/#
   ```

3. **测试 InfluxDB**:
   ```bash
   # 使用 InfluxDB CLI
   influx query 'from(bucket:"sensor_data") |> range(start:-1h)'
   ```

---

### Q3: 可以使用本地 InfluxDB 吗？

**A**: 可以！修改配置：

```c
// influxdb_writer.c
#define INFLUXDB_URL    "http://192.168.1.100:8086"  // 本地 IP
#define INFLUXDB_HOST   "192.168.1.100"                // 本地 IP
#define INFLUXDB_ORG    "my-local-org"
#define INFLUXDB_TOKEN  "your-local-token"
```

**注意**: 本地部署时使用 `http://` 而非 `https://`。

---

### Q4: 如何撤销已提交的敏感信息？

**A**: 如果不慎将真实凭证提交到了 Git：

```bash
# 1. 立即修改凭证（在服务商处重置密码/token）

# 2. 从 Git 历史中删除（使用 BFG 或 git filter-branch）
# 注意：这会重写 Git 历史，需要 force push

# 3. 通知所有协作者重新 clone 仓库

# 示例：使用 BFG Repo Cleaner
bfg --replace-text passwords.txt repo.git
cd repo.git
git reflog expire --expire=now --all && git gc --prune=now --aggressive
git push --force
```

> ⚠️ **警告**: 重写 Git 历史是危险操作，请先备份！

---

### Q5: 如何添加新的敏感信息保护？

**A**: 

1. **编辑 .gitignore** 添加新规则：

   ```bash
   echo "new_sensitive_file" >> .gitignore
   ```

2. **更新安全检查脚本**:

   编辑 `scripts/security-check.sh`，在 `REAL_CREDENTIALS` 数组中添加新模式。

3. **更新配置文档**:

   在 `CONFIG.example.md` 中记录新的配置项。

---

## 🛠️ 故障排除

### 问题：安全检查失败

**错误信息**:
```
❌ 发现 1 个错误
🚨 请修复以上错误后再提交代码！
```

**解决方案**:
1. 查看具体错误信息
2. 根据提示修复（通常是硬编码的凭证）
3. 重新运行 `./scripts/security-check.sh`

---

### 问题：Git 仍然跟踪了敏感文件

**症状**: `git status` 显示已被忽略的文件

**解决方案**:
```bash
# 从 Git 缓存中移除（保留本地文件）
git rm --cached <sensitive-file>

# 确认 .gitignore 包含该文件
grep "<sensitive-file>" .gitignore

# 提交更改
git commit -m "chore: stop tracking sensitive file"
```

---

### 问题：IDE 显示红色警告（占位符未定义）

**原因**: IDE 可能无法识别宏定义

**解决方案**:
- 这是正常的，不影响编译
- 可以在 IDE 中配置路径或忽略警告
- 或者创建一个 `config_local.h` 文件（已加入 .gitignore）用于本地开发

---

## 📊 安全审计清单

在发布或分享代码前，请确认完成以下所有检查：

### 代码安全
- [ ] 所有占位符已替换为真实凭证（本地副本）
- [ ] 无硬编码的密码、token、API key
- [ ] 日志中不打印敏感信息
- [ ] `.gitignore` 规则完善且有效

### Git 仓库
- [ ] 运行 `./scripts/security-check.sh` 通过
- [ ] `git log` 中无真实凭证
- [ ] 无意外的敏感文件被跟踪
- [ ] 分支权限设置正确（如果是团队协作）

### 凭证管理
- [ ] 凭证存储在安全位置（密码管理器）
- [ ] 不同服务使用不同密码
- [ ] 已设置定期轮换提醒
- [ ] 有凭证泄露应急方案

### 文档
- [ ] `CONFIG.example.md` 已更新
- [ ] `SECURITY.md` 已更新（本文件）
- [ ] README 中有安全说明链接

---

## 📞 获取帮助

如果遇到安全问题：

1. **查看文档**:
   - [CONFIG.example.md](CONFIG.example.md) - 配置指南
   - 本文件 - 安全最佳实践

2. **运行诊断**:
   ```bash
   ./scripts/security-check.sh --verbose  # 如果支持详细模式
   ```

3. **检查官方资源**:
   - [ESP-IDF 安全指南](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/security.html)
   - [InfluxDB 安全最佳实践](https://docs.influxdata.com/influxdb/cloud/reference/security/)
   - [EMQX 安全文档](https://docs.emqx.com/enterprise/v4.3/zh/deployment/security/)

4. **报告安全问题**:
   - 如果发现安全漏洞，请通过私有渠道报告
   - 不要在公开 Issue 中讨论安全细节

---

## 📝 更新日志

### v1.4.0 (2026-09-10)

**新增**:
- ✅ 完整的 `.gitignore` 规则
- ✅ 自动化安全检查脚本 (`scripts/security-check.sh`)
- ✅ 详细的配置指南 (`CONFIG.example.md`)
- ✅ 本安全使用指南 (`SECURITY.md`)

**改进**:
- 🔧 移除所有硬编码的敏感信息
- 🔧 标准化占位符命名 (`YOUR_XXX` 格式)
- 🔧 添加 DNS 解析的主机名分离（`INFLUXDB_HOST`）

**修复**:
- 🐛 修复 `influxdb_writer.c` 中遗漏的硬编码 URL

---

## 📄 许可证

本项目采用 MIT 许可证。详见 [LICENSE](LICENSE) 文件。

---

## 👥 贡献者

- **主开发者**: Jeffery Sun
- **安全审查**: Security Team
- **文档编写**: Documentation Team

---

**最后更新**: 2026-09-10  
**版本**: v1.4.0  
**状态**: ✅ 安全检查通过  
**维护者**: ESP32 Environment Monitor Team

---

💡 **提示**: 定期访问此页面获取最新的安全建议和最佳实践！