#!/bin/bash

# ===========================================
# 🔐 ESP32 Environment Monitor - 安全检查脚本
# 在 Git 提交前运行，检测是否包含敏感信息
# ===========================================

set -e  # 遇到错误立即退出

# 颜色定义
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# 计数器
ERRORS=0
WARNINGS=0

echo -e "${BLUE}🔍 开始安全检查...${NC}"
echo "================================"

# -------------------------------------------
# 1. 检查 .gitignore 是否存在
# -------------------------------------------
echo -e "\n${BLUE}📋 检查 .gitignore 配置...${NC}"

if [ ! -f ".gitignore" ]; then
    echo -e "${RED}❌ 错误: 未找到 .gitignore 文件${NC}"
    ((ERRORS++))
else
    echo -e "${GREEN}✅ .gitignore 存在${NC}"
    
    # 检查关键规则是否存在
    CRITICAL_PATTERNS=(".env" "*.key" "*.secret" "credentials")
    for pattern in "${CRITICAL_PATTERNS[@]}"; do
        if grep -q "$pattern" .gitignore; then
            echo -e "${GREEN}   ✅ 包含规则: $pattern${NC}"
        else
            echo -e "${YELLOW}   ⚠️  建议添加规则: $pattern${NC}"
            ((WARNINGS++))
        fi
    done
fi

# -------------------------------------------
# 2. 检查是否有未忽略的敏感文件
# -------------------------------------------
echo -e "\n${BLUE}🔎 扫描可能泄露的文件...${NC}"

SENSITIVE_FILES=(
    ".env"
    ".env.local"
    "credentials.json"
    "secrets.yaml"
    "config.local.*"
    "*_real.*"
)

for pattern in "${SENSITIVE_FILES[@]}"; do
    if ls $pattern 1>/dev/null 2>&1; then
        echo -e "${RED}❌ 发现敏感文件: $pattern (应被 .gitignore 忽略)${NC}"
        ((ERRORS++))
    fi
done

if [ $ERRORS -eq 0 ]; then
    echo -e "${GREEN}✅ 未发现未忽略的敏感文件${NC}"
fi

# -------------------------------------------
# 3. 检查代码中的真实凭证 (硬编码检测)
# -------------------------------------------
echo -e "\n${BLUE}🔍 检测代码中的硬编码凭证...${NC}"

# 常见的真实凭证模式（示例值）
REAL_CREDENTIALS=(
    # WiFi 示例
    "HUAWEI-5FEC"
    "97395269"
    
    # MQTT 示例  
    "f4319339.ala.cn-hangzhou.emqxsl.cn"
    "jerrysun"
    "renyi1004"
    
    # InfluxDB 示例
    "us-east-1-1.aws.cloud2.influxdata.com"
    "Fellowes"
    "doR-H4Eoxc"
    
    # 通用模式
    "password\s*=\s*\"[^\"]+\""
    "token\s*=\s*\"[^\"]{20,}\""
    "api_key\s*=\s*\"[^\"]+\""
    "secret\s*=\s*\"[^\"]+\""
)

FOUND_REAL_CREDS=false

for cred in "${REAL_CREDENTIALS[@]}"; do
    # 在源代码中搜索（排除以下目录和文件）：
    # - node_modules: 依赖包
    # - build: 编译产物
    # - .git: Git 内部文件
    # - credentials.local.h: 本地配置文件（允许包含真实凭证）
    # - *.local.h: 所有本地配置文件
    if grep -r "$cred" --include="*.c" --include="*.h" --include="*.js" --include="*.json" \
           --exclude-dir=node_modules --exclude-dir=build --exclude-dir=.git \
           --exclude="credentials.local.h" --exclude="*.local.h" 2>/dev/null; then
        echo -e "${RED}❌ 发现真实凭证: $cred${NC}"
        ((ERRORS++))
        FOUND_REAL_CREDS=true
    fi
done

# 特别说明：credentials.local.h 中的凭证是预期行为
if [ -f "credentials.local.h" ]; then
    echo -e "${BLUE}ℹ️  注意: credentials.local.h 包含真实凭证（这是正常的）${NC}"
    echo -e "${BLUE}   此文件已被 .gitignore 保护，不会被提交到 Git${NC}"
fi

if [ "$FOUND_REAL_CREDS" = false ]; then
    echo -e "${GREEN}✅ 未发现硬编码的真实凭证${NC}"
fi

# -------------------------------------------
# 4. 检查占位符是否正确使用
# -------------------------------------------
echo -e "\n${BLUE}✅ 验证占位符使用...${NC}"

PLACEHOLDERS=(
    "YOUR_WIFI_SSID"
    "YOUR_WIFI_PASSWORD"
    "YOUR_MQTT_BROKER"
    "YOUR_MQTT_USERNAME"
    "YOUR_MQTT_PASSWORD"
    "YOUR_INFLUXDB_URL"
    "YOUR_ORG_NAME"
    "YOUR_INFLUXDB_TOKEN"
)

for placeholder in "${PLACEHOLDERS[@]}"; do
    if grep -r "$placeholder" --include="*.c" --include="*.js" 2>/dev/null | head -1 > /dev/null; then
        echo -e "${GREEN}   ✅ 使用占位符: $placeholder${NC}"
    else
        echo -e "${YELLOW}   ⚠️  未找到占位符: $placeholder (可能已被替换或不存在)${NC}"
        ((WARNINGS++))
    fi
done

# -------------------------------------------
# 5. 检查 Git 状态
# -------------------------------------------
echo -e "\n${BLUE}📊 检查 Git 状态...${NC}"

if git rev-parse --is-inside-work-tree > /dev/null 2>&1; then
    # 检查是否有未跟踪的敏感文件
    UNTRACKED_SENSITIVE=$(git ls-files --others --exclude-standard | grep -E "(credential|secret|\.env|\.key)" || true)
    
    if [ -n "$UNTRACKED_SENSITIVE" ]; then
        echo -e "${RED}❌ 发现未跟踪的敏感文件:${NC}"
        echo "$UNTRACKED_SENSITIVE" | while read file; do
            echo -e "   ${RED}   - $file${NC}"
        done
        ((ERRORS++))
    else
        echo -e "${GREEN}✅ 无未跟踪的敏感文件${NC}"
    fi
    
    # 检查暂存区
    STAGED_FILES=$(git diff --cached --name-only)
    if [ -n "$STAGED_FILES" ]; then
        echo -e "${YELLOW}⚠️  有 ${#STAGED_FILES} 个文件已暂存待提交${NC}"
        
        # 检查暂存区是否包含敏感信息
        SENSITIVE_IN_STAGING=$(git diff --cached | grep -E "(password|token|secret|api_key).*\"[^\"]+\"" | head -5 || true)
        if [ -n "$SENSITIVE_IN_STAGING" ]; then
            echo -e "${RED}❌ 暂存区可能包含敏感信息!${NC}"
            echo "$SENSITIVE_IN_STAGING" | head -3
            ((ERRORS++))
        fi
    fi
else
    echo -e "${YELLOW}⚠️  当前目录不是 Git 仓库${NC}"
fi

# -------------------------------------------
# 6. 输出结果摘要
# -------------------------------------------
echo ""
echo "================================"
echo -e "${BLUE}📋 安全检查报告${NC}"
echo "================================"

if [ $ERRORS -gt 0 ]; then
    echo -e "${RED}❌ 发现 $ERRORS 个错误${NC}"
    echo ""
    echo -e "${RED}🚨 请修复以上错误后再提交代码！${NC}"
    echo -e "${RED}   运行: git reset HEAD 取消暂存${NC}"
    exit 1
elif [ $WARNINGS -gt 0 ]; then
    echo -e "${YELLOW}⚠️  发现 $WARNINGS 个警告${NC}"
    echo ""
    echo -e "${YELLOW}💡 建议处理警告，但可以继续提交${NC}"
    exit 0
else
    echo -e "${GREEN}✅ 所有检查通过！代码安全，可以提交${NC}"
    echo ""
    echo -e "${GREEN}🎉 准备提交...${NC}"
    exit 0
fi

# -------------------------------------------
# 使用说明
# -------------------------------------------
# 
# 1. 直接运行:
#    chmod +x scripts/security-check.sh
#    ./scripts/security-check.sh
#
# 2. 作为 Git pre-commit hook:
#    cp scripts/security-check.sh .git/hooks/pre-commit
#    chmod +x .git/hooks/pre-commit
#
# 3. 手动触发:
#    bash scripts/security-check.sh
#
# ===========================================