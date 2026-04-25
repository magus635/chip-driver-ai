#!/bin/bash
# scripts/rules-version.sh - 输出当前 CLAUDE.md 的版本号

CLAUDE_MD="CLAUDE.md"

if [ ! -f "$CLAUDE_MD" ]; then
    echo "v0.0-unknown"
    exit 1
fi

# 从 CLAUDE.md 中提取 "版本: x.x" 这一行
VERSION=$(grep -m 1 "版本:" "$CLAUDE_MD" | awk '{print $2}')

if [ -z "$VERSION" ]; then
    echo "v0.0-unspecified"
else
    echo "v$VERSION"
fi
