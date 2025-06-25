# mNetAssist 安装指南

## 简介
mNetAssist 是一个功能强大的网络调试助手工具，基于Qt开发，支持UDP、TCP客户端、TCP服务器三种网络协议，提供智能历史记录、专业日志系统等高级功能。

## 系统要求

### 运行环境
- **操作系统**: Linux (已在Ubuntu 20.04+测试)
- **Qt运行库**: Qt 5.15+ 
- **显示服务器**: X11 或 Wayland
- **内存**: 最低128MB可用内存
- **存储**: 约10MB磁盘空间

### 开发环境（编译需要）
- **Qt开发包**: qtbase5-dev, qttools5-dev
- **编译器**: g++ 支持C++11标准
- **构建工具**: make, qmake

## 依赖安装

### Ubuntu/Debian 系统
```bash
# 更新软件包列表
sudo apt update

# 安装Qt开发环境（如需编译）
sudo apt install qtbase5-dev qttools5-dev build-essential

# 或者仅安装运行时依赖
sudo apt install qt5-default libqt5widgets5 libqt5network5 libqt5gui5
```

### CentOS/RHEL 系统
```bash
# 安装Qt开发环境
sudo yum install qt5-qtbase-devel qt5-qttools-devel gcc-c++ make

# 或者仅安装运行时依赖
sudo yum install qt5-qtbase qt5-qtbase-gui
```

## 安装方法

### 方法1：自动安装（推荐）

1. **编译项目**
   ```bash
   cd mNetAssist
   qmake mNetAssist.pro
   make
   ```

2. **执行安装脚本**
   ```bash
   chmod +x install.sh
   sudo ./install.sh
   ```

安装完成后，mNetAssist将被安装到以下位置：
- **程序文件**: `/opt/mNetAssist/mNetAssist`
- **配置目录**: `/opt/mNetAssist/`（程序运行时会在此创建配置文件）
- **图标文件**: `/opt/mNetAssist/mNetAssist.png`
- **桌面文件**: `/usr/share/applications/mNetAssist.desktop`
- **符号链接**: `/usr/local/bin/mNetAssist`
- **卸载脚本**: `/opt/mNetAssist/uninstall.sh`

### 方法2：手动安装

1. **编译程序**
   ```bash
   qmake && make
   ```

2. **创建安装目录**
   ```bash
   sudo mkdir -p /opt/mNetAssist
   ```

3. **复制文件**
   ```bash
   sudo cp mNetAssist /opt/mNetAssist/
   sudo cp mNetAssist.png /opt/mNetAssist/
   sudo chmod +x /opt/mNetAssist/mNetAssist
   ```

4. **创建桌面快捷方式**
   ```bash
   sudo cp desktop/mNetAssist.desktop /usr/share/applications/
   ```

## 使用方法

### 启动程序
安装完成后，您可以通过以下方式启动程序：

1. **从应用程序菜单启动**：
   - 在系统菜单中找到"开发" → "网络调试助手"
   - 或搜索"mNetAssist"

2. **命令行启动**：
   ```bash
   mNetAssist
   ```

3. **直接运行**：
   ```bash
   /opt/mNetAssist/mNetAssist
   ```

### 快速上手

1. **选择协议类型**：
   - UDP：用于无连接通信
   - TCP服务器：创建服务器等待客户端连接
   - TCP客户端：连接到指定的TCP服务器

2. **配置网络参数**：
   - 本地IP和端口（UDP/TCP服务器模式）
   - 服务器IP和端口（TCP客户端模式）
   - 程序会自动加载上次使用的配置

3. **建立连接**：
   - 点击"连接网络"按钮
   - 连接成功后状态栏会显示相应信息

4. **数据通信**：
   - 在发送框中输入数据
   - 支持文本和十六进制格式
   - 点击"发送数据"按钮发送

## 高级功能

### 智能历史记录
- **发送历史**：自动保存最近20条发送数据，支持快速重用
- **连接历史**：保存IP和端口历史（各15条），一键重连
- **协议记忆**：自动记住最后使用的协议类型
- **跨会话持久化**：所有配置保存在`mNetAssist_history.ini`文件中

### 专业日志系统
- **彩色显示**：发送数据绿色，接收数据蓝色
- **时间戳**：精确到毫秒的时间记录
- **灵活控制**：可选择显示/隐藏不同类型的数据
- **源地址显示**：清楚标识数据来源

### 数据传输功能
- **循环发送**：设置间隔时间自动重复发送
- **文件传输**：支持发送和接收文件
- **数据统计**：实时显示收发数据量统计
- **多客户端**：TCP服务器模式支持多客户端管理

## 配置文件说明

程序运行时会在安装目录下创建`mNetAssist_history.ini`配置文件，包含：

```ini
[ConnectionHistory]
IpHistory\1\ip=192.168.1.100
PortHistory\1\port=8080
ServerIpHistory\1\serverIp=192.168.1.100
LastProtocolType=0

[SendHistory]
SendHistory\1\data=Hello World
```

配置文件可以备份和迁移，支持跨设备使用。

## 卸载方法

### 方法1：使用项目目录下的卸载脚本
```bash
cd mNetAssist
sudo ./uninstall.sh
```

### 方法2：使用安装目录下的卸载脚本
```bash
sudo /opt/mNetAssist/uninstall.sh
```

### 方法3：手动卸载
```bash
sudo rm -rf /opt/mNetAssist
sudo rm -f /usr/share/applications/mNetAssist.desktop
sudo rm -f /usr/local/bin/mNetAssist
```

## 故障排除

### 常见问题及解决方案

1. **编译错误**
   ```bash
   # 检查Qt安装
   qmake --version
   
   # 重新安装Qt开发包
   sudo apt install --reinstall qtbase5-dev
   ```

2. **Qt平台插件警告**
   ```
   qt.qpa.plugin: Could not find the Qt platform plugin "wayland"
   ```
   这是正常警告，不影响程序运行。如需消除警告：
   ```bash
   export QT_QPA_PLATFORM=xcb
   ```

3. **权限问题**
   - 确保安装脚本使用sudo权限运行
   - 某些端口（如80、443）可能需要root权限

4. **网络连接失败**
   - 检查防火墙设置
   - 确认目标IP和端口可访问
   - 检查网络配置

5. **界面显示异常**
   - 检查显示服务器状态
   - 尝试重启X11服务
   - 检查Qt主题设置

### 调试模式

启用详细日志输出：
```bash
QT_LOGGING_RULES="*.debug=true" mNetAssist
```

## 更新日志

### 当前版本特性
- ✅ 智能窗口大小调整（屏幕80%）和自动居中
- ✅ 发送数据历史记录（最多20条）
- ✅ 连接配置历史记录（IP/端口各15条）
- ✅ 协议类型自动记忆功能
- ✅ TCP客户端服务器IP独立历史管理
- ✅ 配置文件程序同目录保存
- ✅ 彩色日志显示和精确时间戳
- ✅ 实时数据保存，避免配置丢失
- ✅ 专业的网络通信日志系统
- ✅ 优化的用户界面和交互体验

## 许可证
本项目遵循MIT许可证，详见LICENSE文件。

## 技术支持

如果您在安装或使用过程中遇到问题：

1. **检查系统日志**：`journalctl -f` 查看系统日志
2. **查看程序输出**：在终端中运行程序查看错误信息
3. **检查依赖**：确认Qt库已正确安装
4. **网络测试**：使用系统网络工具验证网络连通性

---

**注意**：首次运行时程序会自动检测和配置最优的网络参数，建议在安装完成后先进行简单的本地回环测试以验证功能正常。 