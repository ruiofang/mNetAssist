#!/bin/bash

# mNetAssist 安装脚本
# 功能：将 mNetAssist 安装到 /opt/ 目录并添加系统图标

set -e  # 遇到错误时退出

# 检查是否以root权限运行
if [ "$EUID" -ne 0 ]; then 
    echo "错误：此脚本需要以root权限运行"
    echo "请使用: sudo ./install.sh"
    exit 1
fi

echo "========================================"
echo "         mNetAssist 安装程序"
echo "========================================"

# 设置变量
APP_NAME="mNetAssist"
INSTALL_DIR="/opt/mNetAssist"
EXECUTABLE="./mNetAssist"
ICON_FILE="./mNetAssist.png"
DESKTOP_FILE="/usr/share/applications/mNetAssist.desktop"

# 检查必要文件是否存在
echo "检查必要文件..."
if [ ! -f "$EXECUTABLE" ]; then
    echo "错误：找不到可执行文件 $EXECUTABLE"
    echo "请先编译项目：qmake && make"
    exit 1
fi

if [ ! -f "$ICON_FILE" ]; then
    echo "错误：找不到图标文件 $ICON_FILE"
    exit 1
fi

# 创建安装目录
echo "创建安装目录: $INSTALL_DIR"
mkdir -p "$INSTALL_DIR"

# 复制可执行文件
echo "复制可执行文件..."
cp "$EXECUTABLE" "$INSTALL_DIR/"
chmod +x "$INSTALL_DIR/$APP_NAME"

# 复制图标文件
echo "复制图标文件..."
cp "$ICON_FILE" "$INSTALL_DIR/"

# 复制LICENSE文件（如果存在）
if [ -f "./LICENSE" ]; then
    echo "复制许可证文件..."
    cp "./LICENSE" "$INSTALL_DIR/"
fi

# 创建桌面文件
echo "创建桌面应用程序文件..."
cat > "$DESKTOP_FILE" << EOF
[Desktop Entry]
Version=1.0
Type=Application
Name=mNetAssist
Name[zh_CN]=网络调试助手
Comment=Network debugging assistant
Comment[zh_CN]=网络调试助手工具
Exec=$INSTALL_DIR/$APP_NAME
Icon=$INSTALL_DIR/$APP_NAME.png
Terminal=false
Categories=Development;Network;Utility;
StartupNotify=true
Keywords=network;tcp;udp;socket;debug;assistant;
Keywords[zh_CN]=网络;TCP;UDP;套接字;调试;助手;
EOF

# 设置桌面文件权限
chmod 644 "$DESKTOP_FILE"

# 创建符号链接到 /usr/local/bin（可选，方便命令行调用）
echo "创建符号链接..."
ln -sf "$INSTALL_DIR/$APP_NAME" "/usr/local/bin/$APP_NAME"

# 更新桌面数据库
if command -v update-desktop-database &> /dev/null; then
    echo "更新桌面数据库..."
    update-desktop-database /usr/share/applications/
fi

# 创建卸载脚本
echo "创建卸载脚本..."
cat > "$INSTALL_DIR/uninstall.sh" << 'EOF'
#!/bin/bash

# mNetAssist 卸载脚本

set -e

# 检查是否以root权限运行
if [ "$EUID" -ne 0 ]; then 
    echo "错误：此脚本需要以root权限运行"
    echo "请使用: sudo /opt/mNetAssist/uninstall.sh"
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

echo "mNetAssist 已成功卸载！"
EOF

chmod +x "$INSTALL_DIR/uninstall.sh"

echo ""
echo "========================================"
echo "         安装完成！"
echo "========================================"
echo "安装位置: $INSTALL_DIR"
echo "可执行文件: $INSTALL_DIR/$APP_NAME"
echo "图标文件: $INSTALL_DIR/$APP_NAME.png"
echo "桌面文件: $DESKTOP_FILE"
echo "符号链接: /usr/local/bin/$APP_NAME"
echo ""
echo "使用方法："
echo "1. 从应用程序菜单启动 mNetAssist"
echo "2. 命令行运行: mNetAssist"
echo "3. 直接运行: $INSTALL_DIR/$APP_NAME"
echo ""
echo "卸载方法："
echo "sudo $INSTALL_DIR/uninstall.sh"
echo ""
echo "或者运行项目目录下的卸载脚本："
echo "sudo ./uninstall.sh"
echo "========================================" 