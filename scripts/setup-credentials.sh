#!/bin/bash

# ===========================================
# 🔐 ESP32 Environment Monitor - 凭证配置向导
# ===========================================
# 
# 用途：快速配置本地开发凭证
# 功能：
#   - 检测并创建 credentials.local.h
#   - 交互式填写各项配置
#   - 验证配置完整性
#   - 备份现有配置
#
# 使用方法：
#   bash scripts/setup-credentials.sh
#
# 作者: ESP32 Environment Monitor Team
# 版本: 1.0.0
# 日期: 2026-09-10
# ===========================================

set -e

# 颜色定义
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
NC='\033[0m' # No Color
BOLD='\033[1m'

# 项目根目录
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"

# 配置文件路径
CREDENTIALS_EXAMPLE="${PROJECT_DIR}/credentials.local.h.example"
CREDENTIALS_LOCAL="${PROJECT_DIR}/credentials.local.h"
CREDENTIALS_BACKUP="${PROJECT_DIR}/credentials.local.h.backup.$(date +%Y%m%d_%H%M%S)"

# ===========================================
# 🛠️ 工具函数
# ===========================================

print_header() {
    echo ""
    echo -e "${CYAN}==========================================${NC}"
    echo -e "${BOLD}${BLUE}  🔐 ESP32 Environment Monitor${NC}"
    echo -e "${BOLD}${BLUE}  凭证配置向导${NC}"
    echo -e "${CYAN}==========================================${NC}"
    echo ""
}

print_success() {
    echo -e "${GREEN}✅ $1${NC}"
}

print_error() {
    echo -e "${RED}❌ $1${NC}"
}

print_warning() {
    echo -e "${YELLOW}⚠️  $1${NC}"
}

print_info() {
    echo -e "${BLUE}ℹ️  $1${NC}"
}

print_step() {
    echo ""
    echo -e "${CYAN}📌 步骤 $1: $2${NC}"
    echo -e "${CYAN}----------------------------------------${NC}"
}

# ===========================================
# 🔍 前置检查
# ===========================================

check_prerequisites() {
    print_step "1/6" "前置检查"
    
    # 检查是否在项目根目录
    if [ ! -f "${PROJECT_DIR}/CMakeLists.txt" ]; then
        print_error "未找到 CMakeLists.txt，请确保在项目根目录运行此脚本"
        exit 1
    fi
    print_success "项目目录正确: ${PROJECT_DIR}"
    
    # 检查示例文件是否存在
    if [ ! -f "$CREDENTIALS_EXAMPLE" ]; then
        print_error "未找到 credentials.local.h.example"
        exit 1
    fi
    print_success "找到配置模板: credentials.local.h.example"
    
    # 检查 .gitignore 是否包含 credentials.local.h
    if grep -q "credentials.local.h" "${PROJECT_DIR}/.gitignore" 2>/dev/null; then
        print_success ".gitignore 已包含 credentials.local.h (安全✓)"
    else
        print_warning "建议将 credentials.local.h 添加到 .gitignore"
    fi
}

# ===========================================
# 📦 备份现有配置
# ===========================================

backup_existing_config() {
    print_step "2/6" "备份现有配置"
    
    if [ -f "$CREDENTIALS_LOCAL" ]; then
        # 检查是否已经是真实配置（非模板）
        if grep -q '"YOUR_' "$CREDENTIALS_LOCAL" 2>/dev/null; then
            print_info "检测到现有配置仍为占位符，跳过备份"
        else
            cp "$CREDENTIALS_LOCAL" "$CREDENTIALS_BACKUP"
            print_success "已备份现有配置到: $(basename "$CREDENTIALS_BACKUP")"
        fi
    else
        print_info "未找到现有配置文件，无需备份"
    fi
}

# ===========================================
# 📝 创建/更新配置文件
# ===========================================

create_credentials_file() {
    print_step "3/6" "创建配置文件"
    
    if [ ! -f "$CREDENTIALS_LOCAL" ]; then
        # 从示例文件复制
        cp "$CREDENTIALS_EXAMPLE" "$CREDENTIALS_LOCAL"
        print_success "已从模板创建: credentials.local.h"
    else
        print_info "配置文件已存在，将进行更新"
    fi
}

# ===========================================
# ⌨️ 交互式配置
# ===========================================

interactive_setup() {
    print_step "4/6" "交互式配置"
    echo ""
    echo -e "${YELLOW}请输入你的真实凭证信息（输入完成后按 Enter）:${NC}"
    echo -e ${YELLOW}"(如果某项留空将保留当前值)"${NC}
    echo ""
    
    # WiFi 配置
    echo -e "${BOLD}🌐 WiFi 配置${NC}"
    echo "-------------------"
    
    read -p "WiFi SSID (网络名称): " wifi_ssid
    if [ -n "$wifi_ssid" ]; then
        sed -i '' "s/LOCAL_WIFI_SSID      \".*\"/LOCAL_WIFI_SSID      \"$wifi_ssid\"/" "$CREDENTIALS_LOCAL"
        print_success "WiFi SSID 已设置"
    fi
    
    read -s -p "WiFi 密码: " wifi_pass
    echo ""
    if [ -n "$wifi_pass" ]; then
        sed -i '' "s/LOCAL_WIFI_PASS      \".*\"/LOCAL_WIFI_PASS      \"$wifi_pass\"/" "$CREDENTIALS_LOCAL"
        print_success "WiFi 密码已设置"
    fi
    
    echo ""
    
    # MQTT 配置
    echo -e "${BOLD}☁️ MQTT 配置 (EMQX Cloud)${NC}"
    echo "---------------------------"
    
    read -p "MQTT Broker URI (例: mqtts://broker.emqxsl.cn:8883): " mqtt_broker
    if [ -n "$mqtt_broker" ]; then
        sed -i '' "s|LOCAL_MQTT_BROKER_URI  \".*\"|LOCAL_MQTT_BROKER_URI  \"$mqtt_broker\"|" "$CREDENTIALS_LOCAL"
        print_success "MQTT Broker URI 已设置"
    fi
    
    read -p "MQTT 用户名: " mqtt_username
    if [ -n "$mqtt_username" ]; then
        sed -i '' "s/LOCAL_MQTT_USERNAME    \".*\"/LOCAL_MQTT_USERNAME    \"$mqtt_username\"/" "$CREDENTIALS_LOCAL"
        print_success "MQTT 用户名已设置"
    fi
    
    read -s -p "MQTT 密码: " mqtt_password
    echo ""
    if [ -n "$mqtt_password" ]; then
        sed -i '' "s/LOCAL_MQTT_PASSWORD    \".*\"/LOCAL_MQTT_PASSWORD    \"$mqtt_password\"/" "$CREDENTIALS_LOCAL"
        print_success "MQTT 密码已设置"
    fi
    
    echo ""
    
    # InfluxDB 配置
    echo -e "${BOLD}📊 InfluxDB Cloud 配置${NC}"
    echo "----------------------"
    
    read -p "InfluxDB URL (例: https://us-east-1-1.aws.cloud2.influxdata.com): " influxdb_url
    if [ -n "$influxdb_url" ]; then
        # 提取主机名用于 INFLUXDB_HOST
        influxdb_host=$(echo "$influxdb_url" | sed 's|https\?://||' | cut -d'/' -f1)
        
        sed -i '' "s|LOCAL_INFLUXDB_URL    \".*\"|LOCAL_INFLUXDB_URL    \"$influxdb_url\"|" "$CREDENTIALS_LOCAL"
        sed -i '' "s|LOCAL_INFLUXDB_HOST   \".*\"|LOCAL_INFLUXDB_HOST   \"$influxdb_host\"|" "$CREDENTIALS_LOCAL"
        print_success "InfluxDB URL 和 Host 已设置"
    fi
    
    read -p "InfluxDB Organization: " influxdb_org
    if [ -n "$influxdb_org" ]; then
        sed -i '' "s/LOCAL_INFLUXDB_ORG    \".*\"/LOCAL_INFLUXDB_ORG    \"$influxdb_org\"/" "$CREDENTIALS_LOCAL"
        print_success "InfluxDB Org 已设置"
    fi
    
    read -p "InfluxDB Bucket (默认: sensor_data): " influxdb_bucket
    if [ -z "$influxdb_bucket" ]; then
        influxdb_bucket="sensor_data"
    fi
    sed -i '' "s/LOCAL_INFLUXDB_BUCKET \".*\"/LOCAL_INFLUXDB_BUCKET \"$influxdb_bucket\"/" "$CREDENTIALS_LOCAL"
    print_success "InfluxDB Bucket 已设置"
    
    echo ""
    echo -e "${YELLOW}InfluxDB API Token 通常是一个很长的字符串${NC}"
    read -p "InfluxDB API Token: " influxdb_token
    if [ -n "$influxdb_token" ]; then
        sed -i '' "s/LOCAL_INFLUXDB_TOKEN  \".*\"/LOCAL_INFLUXDB_TOKEN  \"$influxdb_token\"/" "$CREDENTIALS_LOCAL"
        print_success "InfluxDB Token 已设置"
    fi
    
    echo ""
    
    # 同步小程序配置
    print_info "正在同步微信小程序配置..."
    if [ -n "$influxdb_url" ]; then
        sed -i '' "s|LOCAL_MINIPROGRAM_INFLUXDB_URL    \".*\"|LOCAL_MINIPROGRAM_INFLUXDB_URL    \"$influxdb_url\"|" "$CREDENTIALS_LOCAL"
    fi
    if [ -n "$influxdb_org" ]; then
        sed -i '' "s/LOCAL_MINIPROGRAM_INFLUXDB_ORG    \".*\"/LOCAL_MINIPROGRAM_INFLUXDB_ORG    \"$influxdb_org\"/" "$CREDENTIALS_LOCAL"
    fi
    if [ -n "$influxdb_bucket" ]; then
        sed -i '' "s/LOCAL_MINIPROGRAM_INFLUXDB_BUCKET \".*\"/LOCAL_MINIPROGRAM_INFLUXDB_BUCKET \"$influxdb_bucket\"|" "$CREDENTIALS_LOCAL"
    fi
    if [ -n "$influxdb_token" ]; then
        sed -i '' "s/LOCAL_MINIPROGRAM_INFLUXDB_TOKEN  \".*\"/LOCAL_MINIPROGRAM_INFLUXDB_TOKEN  \"$influxdb_token\"/" "$CREDENTIALS_LOCAL"
    fi
    # 创建或更新小程序配置文件
    MINIPROGRAM_CONFIG="miniprogram/config.local.js"
    
    cat > "$MINIPROGRAM_CONFIG" << EOF
/**
 * 📱 微信小程序 - 本地配置文件（自动生成）
 */

const INFLUXDB_URL = '${influxdb_url:-https://YOUR_INFLUXDB_URL}';
const INFLUXDB_ORG = '${influxdb_org:-YOUR_ORG_NAME}';
const INFLUXDB_TOKEN = '${influxdb_token:-YOUR_INFLUXDB_TOKEN}';

module.exports = {
  INFLUXDB_URL,
  INFLUXDB_ORG,
  INFLUXDB_TOKEN
};
EOF
    
    print_success "小程序配置文件已创建: $MINIPROGRAM_CONFIG"
}

# ===========================================
# ✅ 验证配置
# ===========================================

validate_config() {
    print_step "5/6" "验证配置"
    
    ERRORS=0
    WARNINGS=0
    
    # 检查必要字段
    if grep -q '"YOUR_WIFI_SSID"' "$CREDENTIALS_LOCAL"; then
        print_warning "WiFi SSID 未配置"
        ((WARNINGS++))
    else
        print_success "WiFi SSID ✓"
    fi
    
    if grep -q '"YOUR_WIFI_PASSWORD"' "$CREDENTIALS_LOCAL"; then
        print_warning "WiFi 密码未配置"
        ((WARNINGS++))
    else
        print_success "WiFi 密码 ✓"
    fi
    
    if grep -q '"YOUR_MQTT_BROKER' "$CREDENTIALS_LOCAL"; then
        print_warning "MQTT Broker URI 未配置"
        ((WARNINGS++))
    else
        print_success "MQTT Broker ✓"
    fi
    
    if grep -q '"YOUR_INFLUXDB_URL"' "$CREDENTIALS_LOCAL"; then
        print_warning "InfluxDB URL 未配置"
        ((WARNINGS++))
    else
        print_success "InfluxDB URL ✓"
    fi
    
    if grep -q '"YOUR_INFLUXDB_TOKEN"' "$CREDENTIALS_LOCAL"; then
        print_error "InfluxDB Token 未配置 (必需！)"
        ((ERRORS++))
    else
        print_success "InfluxDB Token ✓"
    fi
    
    echo ""
    
    if [ $ERRORS -gt 0 ]; then
        print_error "验证失败！有 $ERRORS 个错误需要修复"
        return 1
    elif [ $WARNINGS -gt 0 ]; then
        print_warning "验证通过但有 $WARNINGS 个警告（某些功能可能不可用）"
        return 0
    else
        print_success "🎉 所有配置验证通过！"
        return 0
    fi
}

# ===========================================
# 🎉 完成总结
# ===========================================

print_summary() {
    print_step "6/6" "完成总结"
    
    echo ""
    echo -e "${GREEN}========================================${NC}"
    echo -e "${GREEN}  ✅ 凭证配置完成！${NC}"
    echo -e "${GREEN}========================================${NC}"
    echo ""
    echo -e "${BLUE}配置文件位置:${NC}"
    echo "  📄 $CREDENTIALS_LOCAL"
    echo ""
    echo -e "${BLUE}安全状态:${NC}"
    if grep -q "credentials.local.h" "${PROJECT_DIR}/.gitignore" 2>/dev/null; then
        print_success "✓ 此文件已被 .gitignore 保护，不会被提交"
    else
        print_warning "⚠ 建议手动添加到 .gitignore"
    fi
    echo ""
    echo -e "${BLUE}下一步操作:${NC}"
    echo "  1. 编译项目:"
    echo "     cd ${PROJECT_DIR}"
    echo "     idf.py build"
    echo ""
    echo "  2. 烧录并监控:"
    echo "     idf.py -p /dev/cu.usbserial-* flash monitor"
    echo ""
    echo "  3. 如需修改配置:"
    echo "     nano ${CREDENTIALS_LOCAL}"
    echo "     # 或使用任何文本编辑器"
    echo ""
    echo -e "${BLUE}其他命令:${NC}"
    echo "  • 重新运行此向导: bash scripts/setup-credentials.sh"
    echo "  • 安全检查:       bash scripts/security-check.sh"
    echo "  • 查看配置:       cat ${CREDENTIALS_LOCAL}"
    echo ""
    
    if [ -f "$CREDENTIALS_BACKUP" ]; then
        echo -e "${BLUE}备份文件:${NC}"
        echo "  📦 $(basename "$CREDENTIALS_BACKUP")"
        echo ""
    fi
    
    echo -e "${YELLOW}⚠️  重要提醒:${NC}"
    echo "  • 绝对不要将 credentials.local.h 提交到公开仓库"
    echo "  • 不要在日志中打印凭证信息"
    echo "  • 定期更换 Token 和密码"
    echo "  • 使用不同的开发/生产环境凭证"
    echo ""
}

# ===========================================
# 🚀 主程序入口
# ===========================================

main() {
    print_header
    
    check_prerequisites
    backup_existing_config
    create_credentials_file
    interactive_setup
    
    if validate_config; then
        print_summary
        exit 0
    else
        echo ""
        print_error "配置验证未通过，请检查并重新运行此脚本"
        exit 1
    fi
}

# 运行主程序
main "$@"