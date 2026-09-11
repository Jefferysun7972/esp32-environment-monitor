#!/bin/bash
# ===========================================
# 微信小程序配置切换脚本
# ===========================================
# 用法:
#   bash scripts/mp-config.sh apply     注入真实凭证（编译前）
#   bash scripts/mp-config.sh restore   还原为占位符（提交前）
# ===========================================

set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"
APP_JS="$PROJECT_DIR/miniprogram/app.js"
CONFIG_LOCAL="$PROJECT_DIR/miniprogram/config.local.js"
BACKUP="$PROJECT_DIR/miniprogram/app.js.placeholder"

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[0;33m'
NC='\033[0m'

# 从 config.local.js 提取值
extract_value() {
    local key="$1"
    grep "const ${key} = " "$CONFIG_LOCAL" | sed "s/.*= '\(.*\)'.*/\1/"
}

apply_config() {
    if [ ! -f "$CONFIG_LOCAL" ]; then
        echo -e "${RED}❌ 未找到 $CONFIG_LOCAL${NC}"
        echo "   请先运行: bash scripts/setup-credentials.sh"
        exit 1
    fi

    # 先备份当前 app.js（占位符版本）
    if [ ! -f "$BACKUP" ]; then
        cp "$APP_JS" "$BACKUP"
        echo -e "${GREEN}📦 已备份占位符版本到 app.js.placeholder${NC}"
    fi

    # 读取真实值
    REAL_URL=$(extract_value "INFLUXDB_URL")
    REAL_ORG=$(extract_value "INFLUXDB_ORG")
    REAL_TOKEN=$(extract_value "INFLUXDB_TOKEN")

    if [ -z "$REAL_URL" ] || [ -z "$REAL_ORG" ] || [ -z "$REAL_TOKEN" ]; then
        echo -e "${RED}❌ 无法从 config.local.js 读取配置${NC}"
        exit 1
    fi

    # 替换
    sed -i '' \
        -e "s|https://YOUR_INFLUXDB_URL|${REAL_URL}|" \
        -e "s|'YOUR_ORG_NAME'|'${REAL_ORG}'|" \
        -e "s|'YOUR_INFLUXDB_TOKEN'|'${REAL_TOKEN}'|" \
        "$APP_JS"

    echo -e "${GREEN}✅ 已注入真实凭证到 app.js${NC}"
    echo -e "   URL:   ${REAL_URL}"
    echo -e "   Org:   ${REAL_ORG}"
    echo -e "${YELLOW}   ⚠️  编译测试完后记得运行: bash scripts/mp-config.sh restore${NC}"
}

restore_config() {
    if [ ! -f "$BACKUP" ]; then
        echo -e "${YELLOW}⚠️  未找到备份文件，无法还原${NC}"
        echo "   占位符版本应在此: $BACKUP"
        exit 1
    fi

    cp "$BACKUP" "$APP_JS"
    rm "$BACKUP"
    echo -e "${GREEN}✅ 已还原为占位符版本${NC}"
    echo -e "   现在可以安全提交到 Git 了"
}

case "${1:-}" in
    apply)
        apply_config
        ;;
    restore)
        restore_config
        ;;
    *)
        echo "用法: bash scripts/mp-config.sh [apply|restore]"
        echo ""
        echo "  apply     注入真实凭证到 app.js（微信开发者工具编译前运行）"
        echo "  restore   还原为占位符（Git 提交前运行）"
        exit 1
        ;;
esac