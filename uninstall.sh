#!/bin/bash

# mNetAssist 卸载脚本（项目目录版本）
# 功能：卸载已安装到系统的 mNetAssist

set -e

# 检查是否以root权限运行
if [ "$EUID" -ne 0 ]; then 
    echo "错误：此脚本需要以root权限运行"
    echo "请使用: sudo ./uninstall.sh"
    exit 1
fi

echo "========================================"
echo "        mNetAssist 卸载程序"
echo "========================================"

# 设置变量
APP_NAME="mNetAssist"
INSTALL_DIR="/opt/mNetAssist"
DESKTOP_FILE="/usr/share/applications/mNetAssist.desktop"
SYMLINK="/usr/local/bin/$APP_NAME"

# 检查是否已安装
if [ ! -d "$INSTALL_DIR" ] && [ ! -f "$DESKTOP_FILE" ]; then
    echo "mNetAssist 似乎没有安装，或已经被卸载。"
    exit 0
fi

echo "正在卸载 mNetAssist..."

# 删除符号链接
if [ -L "$SYMLINK" ]; then
    echo "删除符号链接: $SYMLINK"
    rm -f "$SYMLINK"
fi

# 删除桌面文件
if [ -f "$DESKTOP_FILE" ]; then
    echo "删除桌面应用程序文件: $DESKTOP_FILE"
    rm -f "$DESKTOP_FILE"
fi

# 删除安装目录
if [ -d "$INSTALL_DIR" ]; then
    echo "删除安装目录: $INSTALL_DIR"
    rm -rf "$INSTALL_DIR"
fi

# 更新桌面数据库
if command -v update-desktop-database &> /dev/null; then
    echo "更新桌面数据库..."
    update-desktop-database /usr/share/applications/
fi

echo ""
echo "========================================"
echo "      mNetAssist 已成功卸载！"
echo "========================================"
echo "已删除的文件和目录："
echo "- $INSTALL_DIR"
echo "- $DESKTOP_FILE"
echo "- $SYMLINK"
echo ""
echo "感谢使用 mNetAssist！"
echo "========================================" 