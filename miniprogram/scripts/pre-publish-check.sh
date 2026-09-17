#!/bin/bash

# ============================================
# 🚀 小程序发布前自动检查脚本
# ============================================
# 用途：检查代码是否符合微信审核要求
# 使用：bash scripts/pre-publish-check.sh
# ============================================

echo "=========================================="
echo "🔍 小程序发布前检查"
echo "=========================================="
echo ""

ERRORS=0
WARNINGS=0

# 1. 检查 console.log 是否存在（应该移除或注释）
echo "📋 1. 检查调试日志..."
LOG_COUNT=$(grep -r "console.log" --include="*.js" miniprogram/ | grep -v "node_modules" | wc -l)
if [ $LOG_COUNT -gt 50 ]; then
    echo "   ❌ 发现 $LOG_COUNT 处 console.log，建议减少到 50 个以下"
    ((ERRORS++))
elif [ $LOG_COUNT -gt 20 ]; then
    echo "   ⚠️  发现 $LOG_COUNT 处 console.log，建议进一步清理"
    ((WARNINGS++))
else
    echo "   ✅ 调试日志数量合理 ($LOG_COUNT)"
fi
echo ""

# 2. 检查隐私协议调用
echo "📋 2. 检查隐私协议..."
if grep -q "requirePrivacyAuthorize" miniprogram/app.js; then
    echo "   ✅ 已添加隐私协议调用"
else
    echo "   ❌ 未找到隐私协议调用（审核必填）"
    ((ERRORS++))
fi
echo ""

# 3. 检查敏感词
echo "📋 3. 检查敏感词汇..."
SENSITIVE_WORDS=("医疗" "诊断" "治疗" "官方" "认证" "权威" "第一" "最佳")
for word in "${SENSITIVE_WORDS[@]}"; do
    COUNT=$(grep -r "$word" --include="*.js" --include="*.wxml" --include="*.json" miniprogram/ 2>/dev/null | wc -l)
    if [ $COUNT -gt 0 ]; then
        echo "   ⚠️  发现敏感词: '$word' ($COUNT 处)"
        ((WARNINGS++))
    fi
done
if [ $WARNINGS -eq 0 ]; then
    echo "   ✅ 未发现明显敏感词"
fi
echo ""

# 4. 检查必要文件
echo "📋 4. 检查必要文件..."
REQUIRED_FILES=(
    "miniprogram/app.js"
    "miniprogram/app.json"
    "miniprogram/app.wxss"
    "miniprogram/project.config.json"
    "miniprogram/sitemap.json"
)

for file in "${REQUIRED_FILES[@]}"; do
    if [ -f "$file" ]; then
        echo "   ✅ $file"
    else
        echo "   ❌ 缺少 $file"
        ((ERRORS++))
    fi
done
echo ""

# 5. 检查配置文件
echo "📋 5. 检查配置..."
if [ -f "miniprogram/app.json" ]; then
    # 检查是否有页面配置
    PAGE_COUNT=$(grep -o '"pages"' miniprogram/app.json | wc -l)
    if [ $PAGE_COUNT -gt 0 ]; then
        echo "   ✅ 页面配置存在"
    else
        echo "   ❌ 缺少页面配置"
        ((ERRORS++))
    fi

    # 检查是否有 tabBar 配置
    TABBAR_COUNT=$(grep -o '"tabBar"' miniprogram/app.json | wc -l)
    if [ $TABBAR_COUNT -gt 0 ]; then
        echo "   ✅ tabBar 配置存在"
    else
        echo "   ⚠️  未配置 tabBar（可选）"
    fi
fi
echo ""

# 6. 检查云函数
echo "📋 6. 检查云函数..."
if [ -d "miniprogram/cloudfunctions/queryInfluxDB" ]; then
    if [ -f "miniprogram/cloudfunctions/queryInfluxDB/index.js" ]; then
        echo "   ✅ 云函数文件存在"
    else
        echo "   ❌ 云函数 index.js 缺失"
        ((ERRORS++))
    fi
    if [ -f "miniprogram/cloudfunctions/queryInfluxDB/package.json" ]; then
        echo "   ✅ 云函数 package.json 存在"
    else
        echo "   ⚠️  云函数 package.json 缺失"
    fi
else
    echo "   ⚠️  云函数目录不存在（可能使用直连模式）"
fi
echo ""

# 7. 检查 config.local.js 是否在 gitignore 中
echo "📋 7. 检查安全配置..."
if [ -f ".gitignore" ]; then
    if grep -q "config.local.js" .gitignore; then
        echo "   ✅ config.local.js 已在 .gitignore 中"
    else
        echo "   ⚠️  建议: 将 config.local.js 添加到 .gitignore"
        ((WARNINGS++))
    fi
fi

# 检查是否有硬编码的 Token
TOKEN_COUNT=$(grep -r "INFLUXDB_TOKEN\s*=" --include="*.js" miniprogram/ | grep -v "config.local.js" | grep -v "require(" | wc -l)
if [ $TOKEN_COUNT -gt 0 ]; then
    echo "   ❌ 发现硬编码的 Token（安全风险）"
    ((ERRORS++))
else
    echo "   ✅ 未发现硬编码凭证"
fi
echo ""

# 8. 检查代码大小
echo "📋 8. 检查代码大小..."
TOTAL_SIZE=$(du -sk miniprogram/ | cut -f1)
if [ $TOTAL_SIZE -gt 2048 ]; then
    echo "   ⚠️  代码总大小: ${TOTAL_SIZE}KB (接近 2MB 限制)"
    ((WARNINGS++))
else
    echo "   ✅ 代码大小合理: ${TOTAL_SIZE}KB"
fi
echo ""

# ============================================
# 输出总结
# ============================================
echo "=========================================="
echo "📊 检查结果汇总"
echo "=========================================="
echo ""
echo "❌ 错误: $ERRORS"
echo "⚠️  警告: $WARNINGS"
echo ""

if [ $ERRORS -gt 0 ]; then
    echo "🔴 请修复以上错误后再提交审核！"
    exit 1
elif [ $WARNINGS -gt 0 ]; then
    echo "🟡 建议处理警告项，但可以提交审核"
    exit 0
else
    echo "🟢 所有检查通过，可以提交审核！"
    exit 0
fi