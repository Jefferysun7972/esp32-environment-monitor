# 🔐 本地开发配置管理指南

**版本**: 1.0.0  
**更新日期**: 2026-09-10  
**适用版本**: v1.4.0+

---

## 📖 概述

### 问题背景

在 v1.4.0 版本中，为了代码安全，所有敏感信息（WiFi 密码、MQTT 凭证、InfluxDB Token 等）都已替换为占位符（`YOUR_XXX`）。这确保了代码可以安全地公开分享，但给本地开发带来了不便：

❌ **之前的问题**：
- 每次编译前都要手动修改多个配置文件
- 容易忘记还原为占位符（导致安全风险）
- 多人协作时容易冲突
- 无法区分开发和生产环境配置

✅ **现在的解决方案**：
- **一次配置，永久使用** - 只需填写一次 `credentials.local.h`
- **自动识别** - 编译时自动使用本地配置
- **安全隔离** - 本地配置文件被 Git 忽略，不会意外提交
- **多环境支持** - 可轻松切换开发/测试/生产环境

---

## 🎯 核心原理

### 工作流程

```
┌─────────────────────────────────────────────────────┐
│                  编译时自动检测                      │
│                                                     │
│  ┌──────────────────┐    ┌──────────────────────┐  │
│  │ credentials.local.h│    │   默认占位符          │  │
│  │ (真实凭证)        │    │   (YOUR_XXX)         │  │
│  └────────┬─────────┘    └──────────┬───────────┘  │
│           │                         │              │
│           ▼                         ▼              │
│  ┌──────────────────────────────────────────┐     │
│  │         WiFi / MQTT / InfluxDB          │     │
│  │         源代码文件 (.c)                   │     │
│  │                                          │     │
│  │  #ifdef __has_include("credentials...    │     │
│  │      使用 LOCAL_XXX (真实值)             │     │
│  │  #else                                   │     │
│  │      使用 YOUR_XXX (占位符)              │     │
│  │  #endif                                  │     │
│  └──────────────────────────────────────────┘     │
│                     │                            │
│                     ▼                            │
│            ┌─────────────┐                       │
│            │  编译固件    │                       │
│            └─────────────┘                       │
└─────────────────────────────────────────────────────┘
```

### 文件说明

| 文件名 | 用途 | Git 状态 | 说明 |
|--------|------|----------|------|
| `credentials.local.h.example` | 配置模板 | ✅ 可提交 | 包含示例和注释 |
| `credentials.local.h` | 你的真实配置 | ❌ 被忽略 | **包含敏感信息，绝不提交！** |
| `components/wifi_web/wifi_web.c` | WiFi 组件 | ✅ 可提交 | 自动检测并使用本地配置 |
| `components/mqtt_cloud/mqtt_cloud.c` | MQTT 组件 | ✅ 可提交 | 自动检测并使用本地配置 |
| `components/influxdb_writer/influxdb_writer.c` | InfluxDB 组件 | ✅ 可提交 | 自动检测并使用本地配置 |

---

## 🚀 快速开始（3种方式）

### 方式一：使用配置向导（推荐⭐）

**最适合首次配置或需要指导的用户**

```bash
# 1. 运行配置向导
bash scripts/setup-credentials.sh

# 2. 按提示输入各项配置：
#    - WiFi SSID 和密码
#    - MQTT Broker 信息
#    - InfluxDB URL、Org、Token

# 3. 验证配置
bash scripts/security-check.sh

# 4. 编译项目
idf.py build

# 5. 烧录运行
idf.py -p /dev/cu.usbserial-* flash monitor
```

**向导特性**：
- ✅ 交互式引导，逐步完成配置
- ✅ 自动备份现有配置
- ✅ 配置完整性验证
- ✅ 友好的错误提示
- ✅ 自动同步小程序配置

---

### 方式二：手动创建（快速）

**适合有经验的开发者**

```bash
# 1. 从模板复制
cp credentials.local.h.example credentials.local.h

# 2. 编辑配置文件
nano credentials.local.h  # 或使用 VS Code、其他编辑器

# 3. 填写所有 YOUR_XXX 占位符：

# WiFi 配置
#define LOCAL_WIFI_SSID      "你的WiFi名称"
#define LOCAL_WIFI_PASS      "你的WiFi密码"

# MQTT 配置
#define LOCAL_MQTT_BROKER_URI  "mqtts://your-broker:8883"
#define LOCAL_MQTT_USERNAME    "你的用户名"
#define LOCAL_MQTT_PASSWORD    "你的密码"

# InfluxDB 配置
#define LOCAL_INFLUXDB_URL    "https://your-region.cloud2.influxdata.com"
#define LOCAL_INFLUXDB_HOST   "your-region.cloud2.influxdata.com"
#define LOCAL_INFLUXDB_ORG    "你的组织名"
#define LOCAL_INFLUXDB_BUCKET "sensor_data"
#define LOCAL_INFLUXDB_TOKEN  "你的API Token（很长的字符串）"

# 4. 保存文件，然后编译
idf.py build
```

---

### 方式三：从备份恢复

**如果之前已配置过**

```bash
# 1. 查找备份文件
ls -la credentials.local.h.backup.*

# 2. 从最新的备份恢复
cp credentials.local.h.backup.20260910_150000 credentials.local.h

# 3. 验证并编译
idf.py build
```

---

## 📝 配置项详解

### 🌐 WiFi 配置

```c
#define LOCAL_WIFI_SSID      "HUAWEI-5FEC"        // WiFi 网络名称
#define LOCAL_WIFI_PASS      "your_wifi_password" // WiFi 密码
#define LOCAL_WIFI_MAX_RETRY 10                  // 最大重试次数（可选）
```

**获取方式**：
- 查看路由器管理页面
- 或询问网络管理员

**常见问题**：
- Q: 支持 5GHz WiFi 吗？  
  A: ESP32 仅支持 2.4GHz WiFi
- Q: 密码包含特殊字符怎么办？  
  A: 用引号包裹即可，如 `"p@ssw0rd!"`

---

### ☁️ MQTT 配置（EMQX Cloud）

```c
#define LOCAL_MQTT_BROKER_URI  "mqtts://your-broker.emqxsl.cn:8883"
#define LOCAL_MQTT_USERNAME    "your_username"
#define LOCAL_MQTT_PASSWORD    "your_password"
```

**获取方式**：
1. 登录 [EMQX Cloud 控制台](https://www.emqx.com/cloud)
2. 进入你的部署详情页
3. 找到"连接信息"
4. 复制 Broker URI、用户名、密码

**格式说明**：
- URI 格式：`mqtts://host:port`
- 端口通常是：`8883` (MQTTS) 或 `1883` (MQTT)
- 用户名/密码在控制台的"认证"部分设置

---

### 📊 InfluxDB Cloud 配置

```c
#define LOCAL_INFLUXDB_URL    "https://us-east-1-1.aws.cloud2.influxdata.com"
#define LOCAL_INFLUXDB_HOST   "us-east-1-1.aws.cloud2.influxdata.com"  // URL 的主机名部分
#define LOCAL_INFLUXDB_ORG    "Fellowes"
#define LOCAL_INFLUXDB_BUCKET "sensor_data"
#define LOCAL_INFLUXDB_TOKEN  "your_long_api_token_string_here=="
```

**获取方式**：
1. 登录 [InfluxDB Cloud 控制台](https://cloud2.influxdata.com)
2. 进入 **Data → Tokens** 页面
3. 点击 **Generate API Token**
4. 选择权限（建议：Read/Write for specific bucket）
5. 复制生成的 Token（很长！）

**重要提示**：
- ⚠️ Token 通常有 **100+ 字符**，确保完整复制
- ⚠️ INFLUXDB_HOST 是 URL 的主机名部分（去掉 `https://`）
- ⚠️ 如果忘记保存 Token，需要重新生成（旧 Token 会失效）

---

## 🔧 高级用法

### 多环境配置

你可以为不同环境维护不同的配置文件：

```bash
# 开发环境（默认）
credentials.local.h

# 测试环境
credentials.staging.h

# 生产环境
credentials.production.h
```

**切换方法**：

**方法 A：符号链接**
```bash
# 切换到生产环境
ln -sf credentials.production.h credentials.local.h

# 切换回开发环境
ln -sf credentials.development.h credentials.local.h
```

**方法 B：复制**
```bash
# 备份当前配置
cp credentials.local.h credentials.local.h.dev

# 切换到生产
cp credentials.production.h credentials.local.h

# 切换回开发
cp credentials.local.h.dev credentials.local.h
```

---

### 调试模式

开启后会在编译日志中显示使用的配置来源：

```c
// 在 credentials.local.h 中修改
#define LOCAL_CREDENTIALS_DEBUG 1  // 0=关闭, 1=开启
```

**输出示例**：
```
🔧 WiFi: 使用 credentials.local.h 中的配置
🔧 MQTT: 使用 credentials.local.h 中的配置
🔧 InfluxDB: 使用 credentials.local.h 中的配置
```

⚠️ **警告**：仅用于调试！不要在生产环境开启！

---

### 团队协作

**场景**：多人共享同一仓库，各自有不同的凭证

**解决方案**：
1. 每人维护自己的 `credentials.local.h`（不提交）
2. 将 `credentials.local.h.example` 作为模板提交到 Git
3. 新成员克隆仓库后运行：
   ```bash
   bash scripts/setup-credentials.sh
   ```

**Git 配置**（确保 `.gitignore` 包含）：
```gitignore
# 凭证文件（绝对不能提交！）
credentials.local.h
credentials.*.local.h
*credentials*.h
!credentials.local.h.example
```

---

## 🛡️ 安全最佳实践

### ✅ DO（推荐做法）

1. **使用配置向导**
   ```bash
   bash scripts/setup-credentials.sh
   ```

2. **定期验证安全性**
   ```bash
   bash scripts/security-check.sh
   ```

3. **使用不同的凭证**
   - 开发环境和生产环境使用不同的 Token
   - 定期轮换 Token（建议每 90 天）

4. **限制 Token 权限**
   - InfluxDB Token 仅授予必要的 bucket 权限
   - MQTT 账号仅授予发布/订阅必要 topic 的权限

5. **使用环境变量（进阶）**
   ```bash
   # 导出为环境变量（临时）
   export INFLUXDB_TOKEN="your_token"
   
   # 或添加到 ~/.bashrc（持久化）
   echo 'export INFLUXDB_TOKEN="your_token"' >> ~/.bashrc
   ```

---

### ❌ DON'T（禁止做法）

1. **❌ 绝对不要提交 credentials.local.h**
   ```bash
   # 错误！不要执行：
   git add credentials.local.h
   git commit -m "add credentials"
   ```

2. **❌ 不要在日志中打印凭证**
   ```c
   // 错误！不要这样做：
   ESP_LOGI(TAG, "Token: %s", INFLUXDB_TOKEN);
   
   // 如果必须调试，使用条件编译：
   #if LOCAL_CREDENTIALS_DEBUG
       ESP_LOGI(TAG, "Using local credentials");
   #endif
   ```

3. **❌ 不要硬编码在源码中**
   ```c
   // 错误！不要这样做：
   #define TOKEN "my_real_token_12345"
   
   // 正确做法：使用 credentials.local.h
   #define TOKEN LOCAL_INFLUXDB_TOKEN
   ```

4. **❌ 不要通过不安全渠道分享**
   - ❌ 邮件明文
   - ❌ 即时消息（微信、Slack 等）
   - ❌ 公开 GitHub Issue
   
   ✅ 安全方式：
   - 密码管理器（1Password、LastPass）
   - 加密文件（GPG、7z with password）
   - 面对面告知

---

## 🐛 故障排除

### 问题 1：编译时出现 "undefined reference"

**症状**：
```
error: undefined reference to 'LOCAL_WIFI_SSID'
```

**原因**：`credentials.local.h` 不存在或路径不对

**解决**：
```bash
# 检查文件是否存在
ls -la credentials.local.h

# 如果不存在，创建它
bash scripts/setup-credentials.sh

# 或者手动从模板复制
cp credentials.local.h.example credentials.local.h
```

---

### 问题 2：仍然使用占位符

**症状**：
```
warning: ⚠️ WiFi: 使用默认占位符
```

**原因**：编译器不支持 `__has_include` 或文件未被正确包含

**解决**：

**检查 1：确认文件存在**
```bash
ls credentials.local.h
```

**检查 2：查看编译器警告**
```bash
idf.py build 2>&1 | grep -i credential
```

**检查 3：手动验证宏定义**
```bash
# 在 wifi_web.c 顶部临时添加：
#pragma message("Testing include")
#include "credentials.local.h"
#pragma message("Included successfully")
```

---

### 问题 3：连接失败

**症状**：WiFi/MQTT/InfluxDB 连接超时

**排查步骤**：

1. **检查凭证是否正确**
   ```bash
   # 查看 WiFi 配置
   grep "LOCAL_WIFI_SSID" credentials.local.h
   
   # 应该看到真实的值，而不是 YOUR_WIFI_SSID
   ```

2. **检查网络连通性**
   ```bash
   # 测试 InfluxDB 连通性
   curl -I https://your-influxdb-url.com
   
   # 测试 MQTT Broker
   nc -zv your-broker-host 8883
   ```

3. **查看设备日志**
   ```bash
   idf.py monitor
   
   # 查找以下关键字：
   # - WiFi: "connected", "retry", "fail"
   # - MQTT: "connected", "disconnected", "error"
   # - InfluxDB: "write", "error", "status"
   ```

4. **验证 Token 有效性**
   - 登录 InfluxDB Cloud 控制板
   - 检查 Token 是否已启用、未过期
   - 尝试重新生成新 Token

---

### 问题 4：Git 提交了凭证文件

**症状**：
```bash
git status
# 显示 credentials.local.h 已被暂存
```

**紧急处理**：

```bash
# 1. 立即取消暂存
git reset HEAD credentials.local.h

# 2. 从历史记录中删除（如果已经提交）
git filter-branch --force --index-filter \
  'git rm --cached --ignore-unmatch credentials.local.h' \
  --prune-empty --tag-name-filter cat -- --all

# 3. 强制推送（谨慎！会重写历史）
git push origin --force --all

# 4. 立即轮换所有凭证！
# - WiFi 密码
# - MQTT 密码
# - InfluxDB Token
```

**预防措施**：
```bash
# 添加 pre-commit hook
cat > .git/hooks/pre-commit << 'EOF'
#!/bin/bash
if git diff --cached --name-only | grep -q "credentials.local.h"; then
    echo "❌ 错误：试图提交 credentials.local.h！"
    echo "此文件包含敏感信息，不应提交到 Git。"
    exit 1
fi
EOF
chmod +x .git/hooks/pre-commit
```

---

## 📊 配置清单

### 首次配置检查表

- [ ] 复制 `credentials.local.h.example` 为 `credentials.local.h`
- [ ] 填写 WiFi SSID 和密码
- [ ] 填写 MQTT Broker URI、用户名、密码
- [ ] 填写 InfluxDB URL、Host、Org、Bucket、Token
- [ ] 运行 `bash scripts/security-check.sh` 验证
- [ ] 运行 `idf.py build` 测试编译
- [ ] 运行 `idf.py flash monitor` 测试连接
- [ ] 确认 `.gitignore` 包含 `credentials.local.h`
- [ ] （可选）设置 pre-commit hook 防止误提交

### 定期维护检查表（每月）

- [ ] 检查 InfluxDB Token 是否即将过期
- [ ] 检查 MQTT 凭证是否需要更换
- [ ] 运行安全检查脚本
- [ ] 备份当前配置
- [ ] 更新文档（如有变更）

---

## 🔄 升级指南

### 从手动配置迁移

如果你之前是直接修改源码中的占位符：

```bash
# 1. 备份当前的修改
git diff > my_credentials_backup.patch

# 2. 还原源码到原始状态
git checkout -- components/wifi_web/wifi_web.c
git checkout -- components/mqtt_cloud/mqtt_cloud.c
git checkout -- components/influxdb_writer/influxdb_writer.c

# 3. 创建本地配置文件
bash scripts/setup-credentials.sh

# 4. 从备份中提取凭证信息（手动或使用 patch）
# 查看备份文件中的真实值，填入 credentials.local.h
```

---

## 📚 相关文档

- **🔒 安全说明**: [SECURITY.md](SECURITY.md)
- **⚙️ 配置教程**: [CONFIG.example.md](CONFIG.example.md)
- **🛡️ 安全检查**: [scripts/security-check.sh](scripts/security-check.sh)
- **📖 项目文档**: [README.md](README.md)
- **🤝 贡献指南**: README.md 中的"贡献指南"章节

---

## 💡 技术细节

### `__has_include` 检测原理

```c
#ifdef __has_include
    #if __has_include("credentials.local.h")
        // 文件存在，使用本地配置
        #include "credentials.local.h"
        #define USE_LOCAL_CREDENTIALS 1
    #else
        // 文件不存在，使用占位符
        #define USE_LOCAL_CREDENTIALS 0
    #endif
#else
    // 编译器不支持 __has_include，回退到占位符
    #define USE_LOCAL_CREDENTIALS 0
#endif
```

**支持情况**：
- ✅ GCC 5+ / Clang 3.6+ / MSVC 2017+
- ✅ ESP-IDF 工具链（基于 GCC）
- ❌ 极老版本的编译器（ESP-IDF v4.0 以下可能不支持）

### 编译器警告机制

当未检测到 `credentials.local.h` 时，会发出警告：

```c
#warning "⚠️ WiFi: 使用默认占位符，请配置 credentials.local.h"
```

这有助于：
- 提醒开发者配置本地凭证
- 避免因忘记配置导致连接失败
- 在 CI/CD 环境中快速定位问题

---

## 📞 获取帮助

遇到问题？

1. **📖 查看本文档的故障排除章节**
2. **🔍 运行诊断命令**：
   ```bash
   # 检查配置文件状态
   ls -lah credentials.local.h*
   
   # 检查 Git 忽略规则
   git check-ignore -v credentials.local.h
   
   # 检查编译器支持
   echo '__has_include("test")' | gcc -xc - -o /dev/null 2>&1
   ```

3. **💬 社区支持**：
   - [GitHub Discussions](https://github.com/Jefferysun7972/esp32-environment-monitor/discussions)
   - [GitHub Issues](https://github.com/Jefferysun7972/esp32-environment-monitor/issues)

4. **📧 联系维护者**：
   - 创建 GitHub Issue 并标记 `help wanted`

---

## 📜 更新日志

### v1.0.0 (2026-09-10)
- ✅ 初始版本
- ✅ 支持本地凭证配置
- ✅ 自动检测和回退机制
- ✅ 交互式配置向导
- ✅ 完整的安全保护
- ✅ 多环境配置支持

---

**🎉 现在你只需要配置一次，之后每次编译都会自动使用真实凭证！**

**最后更新**: 2026-09-10 | **作者**: ESP32 Environment Monitor Team