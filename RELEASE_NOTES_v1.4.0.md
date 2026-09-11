# 🎉 v1.4.0 正式版本发布 - 安全增强与UI优化

**发布日期**: 2026-09-10  
**版本类型**: Major Release (重要更新)  
**分支**: main (ce8723a)  
**状态**: 🟢 Production Ready | 🔒 Safe

---

## ✨ 版本亮点

### 🔒 安全增强（⚠️ 重要更新）

此版本最重要的改进是**全面的安全加固**，代码现已可安全公开分享：

- ✅ **代码脱敏**: 所有敏感信息（WiFi、MQTT、InfluxDB 凭证）已替换为 `YOUR_XXX` 占位符
- ✅ **Git 保护**: 添加完善的 `.gitignore` 规则，防止凭证文件意外提交
- ✅ **安全工具**: 创建自动化安全检查脚本 (`scripts/security-check.sh`)
- ✅ **文档完善**: 
  - [CONFIG.example.md](CONFIG.example.md) - 详细的配置指南
  - [SECURITY.md](SECURITY.md) - 完整的安全说明
  - README.md 新增安全章节和 FAQ

**⚠️ 注意**: 使用前必须替换占位符为真实凭证！

---

### 🎨 UI/UX 优化

大幅提升用户体验，特别是历史曲线页面：

- ✅ **导航栏固定**: 历史曲线页面导航栏不再随内容滚动（使用 fixed 定位 + cover-view）
- ✅ **直角设计**: 移除导航栏底部圆角，改为现代直角风格
- ✅ **智能截断**: 传感器名称自动截断为最多 5 个字符
- ✅ **图例优化**: 
  - 右对齐显示
  - 动态计算宽度，适配多传感器
  - 优化项间距，提升可读性
- ✅ **间隙调整**: 导航栏与内容区域间隙优化至最佳视觉比例

---

### 📊 数据可视化增强

- ✅ **统计摘要**: 新增最小值、最大值、平均值显示
- ✅ **布局优化**: 统计摘要移至图表上方，避免被 canvas 覆盖
- ✅ **动态适配**: 多传感器图例宽度自动调整，确保所有名称可见

---

### 🌡️ 数据处理改进

- ✅ **大气压修复**: 完整支持 UART 传感器的大气压数据（`pres` 字段）
- ✅ **字段归一化**: 统一所有大气压字段名为 `pressure`（支持 `pres`/`press` → `pressure`）
- ✅ **查询优化**: InfluxDB 查询支持多字段匹配，提高数据兼容性

---

### 📝 文档全面升级

README.md 从 ~379 行升级至 **694 行（+83%）**，新增：

- 📑 完整目录导航
- 🆕 v1.4.0 特性亮点说明
- 📁 项目结构树形图
- 🚀 快速开始安全提醒
- 📋 详细更新日志 (v1.0.0 - v1.4.0)
- 🔒 安全说明章节
- ❓ 常见问题 FAQ（12 个问题）
- 🤝 贡献指南和开发检查清单

---

### 🔧 代码质量提升

- ✅ 添加 `dependencies.lock` 锁定依赖版本
- ✅ 代码注释完善
- ✅ 文件组织结构优化

---

## 📦 包含内容

### ESP32 固件
- ✅ 多传感器支持（AM2020DY + SEN66/SEN68 自动检测）
- ✅ WiFi 连接 + MQTT 云端上传（EMQX Cloud）
- ✅ InfluxDB 时序数据存储
- ✅ TFT-LCD 显示（ILI9341 240×320）+ LED 告警系统
- ✅ 所有凭证已脱敏，可安全分享

### 微信小程序
- ✅ 实时数据仪表盘（双传感器对比）
- ✅ 历史曲线图表（1h/6h/24h，支持大气压指标）
- ✅ 深色模式 + 高对比度模式 + 系统主题跟随
- ✅ 自定义主题色（6 种）
- ✅ 统计摘要功能
- ✅ 温度单位切换（°C/°F）

### 文档与工具
- 📖 **README.md** - 生产级完整文档（694 行）
- 🔒 **SECURITY.md** - 安全使用指南
- ⚙️ **CONFIG.example.md** - 详细配置教程
- 🛡️ **scripts/security-check.sh** - 自动化安全检查脚本
- 📄 **.gitignore** - 全面的忽略规则

---

## ⚠️ 使用前必读（重要！）

### 🔐 第一步：配置凭证（必须完成）

**请务必按顺序完成以下配置：**

#### 1️⃣ 阅读配置指南
```bash
cat CONFIG.example.md
```
或在线查看: [CONFIG.example.md](CONFIG.example.md)

#### 2️⃣ 替换占位符

编辑以下文件，将所有 `YOUR_XXX` 替换为你的真实凭证：

| 文件 | 配置项 | 示例 |
|------|--------|------|
| `components/wifi_web/wifi_web.c` | WiFi SSID/密码 | `YOUR_WIFI_SSID`, `YOUR_WIFI_PASS` |
| `components/mqtt_cloud/mqtt_cloud.c` | MQTT 配置 | `MQTT_BROKER_URI`, `MQTT_USERNAME`, `MQTT_PASSWORD` |
| `components/influxdb_writer/influxdb_writer.c` | InfluxDB 配置 | `INFLUXDB_URL`, `INFLUXDB_ORG`, `INFLUXDB_TOKEN` |
| `miniprogram/app.js` | 小程序 InfluxDB | `INFLUXDB_URL`, `INFLUXDB_ORG`, `INFLUXDB_TOKEN` |

#### 3️⃣ 运行安全检查
```bash
bash scripts/security-check.sh
```
确保输出无错误提示。

#### 4️⃣ 本地测试
```bash
idf.py build
idf.py -p /dev/cu.usbserial-* flash monitor
```

✅ 测试通过后即可部署！

---

## 🔄 从 v1.3.0 升级指南

如果你正在使用 v1.3.0 或更早版本，请注意：

### ⚠️ 重要变更

1. **必须重新配置凭证**
   - 此版本所有凭证已替换为占位符
   - 需要重新填写你的真实配置信息

2. **建议备份**
   - 升级前备份你的自定义配置
   - 特别注意 `wifi_web.c`, `mqtt_cloud.c`, `influxdb_writer.c` 的配置

3. **查看变更日志**
   - 详细了解本版本的所有改进
   - 见下方"📋 完整更新日志"章节

### 📦 升级步骤

```bash
# 1. 备份当前配置（可选但推荐）
cp components/wifi_web/wifi_web.c wifi_web.c.backup
cp components/mqtt_cloud/mqtt_cloud.c mqtt_cloud.c.backup
cp components/influxdb_writer/influxdb_writer.c influxdb_writer.c.backup

# 2. 拉取最新代码
git fetch origin
git checkout main
git pull origin main

# 3. 重新配置凭证（参考上方"使用前必读"）

# 4. 运行安全检查
bash scripts/security-check.sh

# 5. 重新编译和烧录
idf.py fullclean
idf.py build
idf.py -p /dev/cu.usbserial-* flash monitor
```

---

## 🐛 已修复问题

| 问题 | 严重程度 | 修复方案 | 状态 |
|------|---------|---------|------|
| ❌ 大气压数据不显示 | 高 | UART 传感器 `pres` 字段识别 + 归一化 | ✅ 已修复 |
| ❌ 历史曲线导航栏随内容滚动 | 中 | Fixed 定位 + Cover-view + DOM 结构优化 | ✅ 已修复 |
| ❌ 导航栏底部圆角不符合设计 | 低 | 移除 border-radius 属性 | ✅ 已修复 |
| ❌ 多传感器名称显示不全 | 中 | 智能截断（5字符）+ 动态宽度图例 | ✅ 已修复 |
| ❌ 图例位置不当 | 低 | 右对齐 + 左侧预留空间 | ✅ 已修复 |
| ❌ 敏感信息泄露风险 | **严重** | 全面脱敏 + .gitignore + 安全检查 | ✅ 已修复 |
| ❌ 文档不够完善 | 中 | 升级至生产级文档（694行） | ✅ 已修复 |

---

## 📋 完整更新日志

### 🔒 安全（5 项）
- ✅ 替换所有硬编码凭证为占位符（WiFi、MQTT、InfluxDB）
- ✅ 添加 `.gitignore` 防止敏感文件提交
- ✅ 创建 `scripts/security-check.sh` 自动化安全检查
- ✅ 添加 `CONFIG.example.md` 配置指南
- ✅ 添加 `SECURITY.md` 安全说明文档

### 🎨 UI/UX 改进（6 项）
- ✅ 修复历史曲线页面导航栏随内容滚动问题（fixed + cover-view）
- ✅ 移除导航栏底部圆角，改为直角设计
- ✅ 调整导航栏与内容区域间隙至最佳视觉比例
- ✅ 传感器名称智能截断（最多 5 个字符）
- ✅ 图例右对齐，动态计算宽度
- ✅ 优化图例项间距

### 📊 数据可视化（3 项）
- ✅ 添加统计摘要显示（最小值、最大值、平均值）
- ✅ 将统计摘要移至图表上方，避免被覆盖
- ✅ 多传感器图例动态宽度适配

### 🌡️ 数据处理（3 项）
- ✅ 修复 UART 传感器大气压数据识别（`pres` → `pressure`）
- ✅ InfluxDB 查询支持多字段匹配（`pressure`/`pres`/`press`）
- ✅ 统一所有传感器的大气压字段名为 `pressure`

### 📝 文档（5 项）
- ✅ 更新 README.md 添加安全说明和 v1.4.0 特性
- ✅ 添加常见问题解答（FAQ）- 12 个问题
- ✅ 添加贡献指南和开发检查清单
- ✅ 添加项目结构说明
- ✅ 更新快速开始指南，强调安全配置步骤

### 🔧 代码质量（3 项）
- ✅ 添加 `dependencies.lock` 锁定依赖版本
- ✅ 完善代码注释
- ✅ 优化文件组织结构

**总计**: 25 项改进 ✅

---

## 🌟 新功能演示

### 1. 安全检查脚本
```bash
$ bash scripts/security-check.sh

🔍 ESP32 Environment Monitor - 安全检查工具
==========================================

✅ .gitignore 存在且包含必要规则
✅ 未发现敏感文件被跟踪
✅ 未检测到硬编码凭证
✅ 依赖锁定文件存在

🎉 安全检查通过！代码可以安全提交。
```

### 2. 历史曲线页面优化
- 导航栏固定在顶部，不随内容滚动
- 传感器名称智能截断："AM2020D..." "SEN68..."
- 图例右对齐，清晰展示所有传感器
- 统计摘要显示在图表上方

### 3. 大气压数据支持
```
选择指标: [温度] [湿度] [PM2.5] [大气压] ✅ ← 现在可选！
时间范围: [1小时] [6小时] [24小时]

图表显示:
┌─────────────────────────────┐
│  统计摘要                   │
│  最小: 1012.3 hPa           │
│  最大: 1018.7 hPa           │
│  平均: 1015.2 hPa           │
├─────────────────────────────┤
│        📈 大气压曲线         │
│    ╱╲    ╱╲                │
│   ╱  ╲  ╱  ╲               │
│  ╱    ╲╱    ╲              │
│ ─────────────────          │
│  10:00  12:00  14:00       │
└─────────────────────────────┘
```

---

## 📊 版本统计

| 指标 | 数值 |
|------|------|
| **总代码变更** | +322 行（README.md） |
| **文档行数** | 694 行（从 379 行提升 83%） |
| **新增文件** | 4 个（SECURITY.md, CONFIG.example.md, security-check.sh, RELEASE_NOTES） |
| **修改文件** | 15+ 个 |
| **新增功能** | 8 项主要功能 |
| **修复问题** | 7 个（含 1 个严重问题） |
| **安全改进** | 5 项 |
| **文档更新** | 5 项 |

---

## 🙏 致谢

感谢以下贡献者和测试者：
- 所有提供反馈的用户
- 报告 Bug 的社区成员
- 测试预览版本的志愿者

**特别感谢**: 对安全性的重视和建议，让这个项目变得更加安全可靠！

---

## 📌 下一步计划 (v1.5.0 Roadmap)

我们计划在下一版本中加入以下功能：

### 🎯 高优先级
- 📊 **阈值参考线**: 在图表上显示正常/警告/危险阈值线
- 👁️ **可切换图例**: 点击图例显示/隐藏对应曲线
- 📥 **CSV 数据导出**: 导出历史数据为 CSV 文件

### 🌟 中优先级
- 〰️ **贝塞尔曲线平滑**: 让图表曲线更加平滑美观
- 📱 **小程序性能优化**: 减少内存占用，提升渲染速度
- 🔔 **推送通知**: 异常数据告警推送

### 💡 低优先级
- 🌍 **多语言支持**: 英文/中文界面切换
- 📊 **更多图表类型**: 饼图、柱状图等
- ☁️ **OTA 升级**: 远程固件升级功能

**💡 有功能建议？欢迎在 [Issues](https://github.com/Jefferysun7972/esp32-environment-monitor/issues) 提出！**

---

## 🔗 相关链接

### 📖 文档
- **📄 完整文档**: [README.md](https://github.com/Jefferysun7972/esp32-environment-monitor/blob/main/README.md)
- **🔒 安全说明**: [SECURITY.md](https://github.com/Jefferysun7972/esp32-environment-monitor/blob/main/SECURITY.md)
- **⚙️ 配置指南**: [CONFIG.example.md](https://github.com/Jefferysun7972/esp32-environment-monitor/blob/main/CONFIG.example.md)

### 🔧 工具
- **🛡️ 安全检查脚本**: [scripts/security-check.sh](https://github.com/Jefferysun7972/esp32-environment-monitor/blob/main/scripts/security-check.sh)
- **📦 Release 创建脚本**: [scripts/create-release.sh](https://github.com/Jefferysun7972/esp32-environment-monitor/blob/main/scripts/create-release.sh)

### 📊 历史版本
- [v1.3.0 - Dark Mode & Accessibility](https://github.com/Jefferysun7972/esp32-environment-monitor/releases/tag/v1.3.0)
- [v1.2.0 - WeChat Mini Program](https://github.com/Jefferysun7972/esp32-environment-monitor/releases/tag/v1.2.0)
- [v1.1.0 - Cloud Integration](https://github.com/Jefferysun7972/esp32-environment-monitor/releases/tag/v1.1.0)
- [v1.0.0 - Initial Release](https://github.com/Jefferysun7972/esp32-environment-monitor/releases/tag/v1.0.0)

---

## 📞 支持

遇到问题？需要帮助？

- 🐛 **报告 Bug**: [GitHub Issues](https://github.com/Jefferysun7972/esp32-environment-monitor/issues/new?template=bug_report.md)
- 💡 **功能建议**: [Feature Request](https://github.com/Jefferysun7972/esp32-environment-monitor/issues/new?template=feature_request.md)
- ❓ **使用问题**: [ Discussions](https://github.com/Jefferysun7972/esp32-environment-monitor/discussions)
- 📖 **查看文档**: 先阅读 [FAQ](https://github.com/Jefferysun7972/esp32-environment-monitor/blob/main/README.md#-常见问题-faq)

---

## 📜 许可证

本项目基于 [MIT License](LICENSE) 开源。

---

**🎉 感谢使用 ESP32 Environment Monitor！**

**Last Updated**: 2026-09-10 | **Maintainer**: [@Jefferysun7972](https://github.com/Jefferysun7972) | **Version**: v1.4.0