/**
 * @file credentials.local.h
 * @brief 本地开发凭证配置（真实值）
 * 
 * ⚠️ 警告：此文件包含敏感信息！
 * - 此文件已被 .gitignore 保护，不会被提交到 Git
 * - 请勿将此文件分享给他人或上传到公开仓库
 * - 仅用于本地编译和开发
 * 
 * 📝 配置说明：
 * 填写你从以下服务获取的真实凭证：
 * - WiFi: 你的网络名称和密码
 * - MQTT: EMQX Cloud 控制台的连接信息
 * - InfluxDB: InfluxDB Cloud 控制台的 API Token
 * 
 * @author Your Name
 * @version 1.0.0 (local)
 * @date 2026-09-10
 */

#ifndef CREDENTIALS_LOCAL_H
#define CREDENTIALS_LOCAL_H

#ifdef __cplusplus
extern "C" {
#endif

/* ===========================================
 * 🌐 WiFi 配置
 * =========================================== */

#define LOCAL_WIFI_SSID      "HUAWEI-5FEC"
#define LOCAL_WIFI_PASS      "97395269973952699739526997395269"
#define LOCAL_WIFI_MAX_RETRY 10


/* ===========================================
 * ☁️ MQTT 配置 (EMQX Cloud)
 * =========================================== */

#define LOCAL_MQTT_BROKER_URI  "mqtts://f4319339.ala.cn-hangzhou.emqxsl.cn:8883"
#define LOCAL_MQTT_USERNAME    "jerrysun"
#define LOCAL_MQTT_PASSWORD    "任意004"


/* ===========================================
 * 📊 InfluxDB Cloud 配置
 * =========================================== */

#define LOCAL_INFLUXDB_URL    "https://us-east-1-1.aws.cloud2.influxdata.com"
#define LOCAL_INFLUXDB_HOST   "us-east-1-1.aws.cloud2.influxdata.com"
#define LOCAL_INFLUXDB_ORG    "Fellowes"
#define LOCAL_INFLUXDB_BUCKET "sensor_data"
#define LOCAL_INFLUXDB_TOKEN  "doR-H4EoxcxidC5AYN0NjzYQB7kJ5cusQvXe16b7j1W_tO4ouL35MlFayhPfTlnxR0djAgCwCFfgOVZSCXyzog=="


/* ===========================================
 * 📱 微信小程序配置
 * =========================================== */

#define LOCAL_MINIPROGRAM_INFLUXDB_URL    "https://us-east-1-1.aws.cloud2.influxdata.com"
#define LOCAL_MINIPROGRAM_INFLUXDB_ORG    "Fellowes"
#define LOCAL_MINIPROGRAM_INFLUXDB_BUCKET "sensor_data"
#define LOCAL_MINIPROGRAM_INFLUXDB_TOKEN  "YOUR_INFLUXDB_TOKEN"


/* ===========================================
 * 🔧 高级配置
 * =========================================== */

/**
 * 调试模式：0=关闭, 1=开启
 * ⚠️ 开启后日志中会显示凭证，仅用于调试！
 */
#define LOCAL_CREDENTIALS_DEBUG 0


#ifdef __cplusplus
}
#endif

#endif /* CREDENTIALS_LOCAL_H */