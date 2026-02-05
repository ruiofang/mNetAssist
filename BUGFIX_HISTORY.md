# 历史记录功能修复说明

## 问题描述
工程在历史保存和选择历史时会出现不对应的问题，主要表现为：
1. 发送历史下拉框显示的内容与实际选择的内容不匹配
2. 连接历史（IP和端口）在添加时可能触发不必要的事件导致UI混乱

## 问题原因

### 1. 发送历史问题
- **索引计算错误**：在 `on_cBoxSendHistory_currentTextChanged` 函数中，使用 `currentIndex() - 1` 来访问 `sendHistory` 列表，但当下拉框项目发生变化时（添加/删除），索引可能不同步
- **截断文本匹配问题**：下拉框中显示的是截断的文本（超过50字符显示为"..."），但选择时按索引匹配，导致找不到完整的历史数据
- **信号干扰**：在 `addToSendHistory` 中修改下拉框时会触发 `currentTextChanged` 信号，导致不必要的UI更新

### 2. 连接历史问题
- **信号干扰**：在添加连接历史和加载历史时，UI操作会触发 `currentTextChanged` 信号，导致输入框内容被意外修改
- **下拉框未初始化**：在某些情况下，下拉框可能没有提示文本，导致后续添加项目时索引计算错误

## 修复方案

### 1. 发送历史修复（mNetAssistWidget.cpp）

#### addToSendHistory 函数
- 添加 `blockSignals(true)` 来阻止 UI 操作触发信号
- 确保所有 UI 修改操作（removeItem、insertItem、setCurrentIndex）都在信号阻塞期间完成
- 操作完成后恢复信号：`blockSignals(false)`

#### on_cBoxSendHistory_currentTextChanged 函数
- **改用文本匹配而非索引计算**：不再使用 `currentIndex() - 1` 来访问数组
- **支持截断文本匹配**：检测文本是否以"..."结尾，如果是则进行前缀匹配
- **精确匹配**：对于完整文本，直接在 `sendHistory` 中查找

```cpp
// 旧方法（有问题）
int index = ui->cBoxSendHistory->currentIndex() - 1;
if (index >= 0 && index < sendHistory.size()) {
    QString selectedData = sendHistory.at(index);
}

// 新方法（修复后）
if (text.endsWith("...")) {
    QString prefix = text.left(text.length() - 3);
    for (const QString &historyItem : sendHistory) {
        if (historyItem.startsWith(prefix)) {
            selectedData = historyItem;
            break;
        }
    }
} else {
    if (sendHistory.contains(text)) {
        selectedData = text;
    }
}
```

### 2. 连接历史修复（mNetAssistWidget.cpp）

#### addToConnectionHistory 函数
- 在所有 UI 操作前添加信号阻塞
- 操作完成后恢复信号
- 保持原有的索引恢复逻辑，确保用户当前选择不受影响

#### loadConnectionHistory 函数
- 在加载开始前阻塞 IP 和端口下拉框的信号
- 确保下拉框初始化时有提示文本（"▼"）
- 加载完成后恢复信号
- 最后再设置协议类型和IP/端口，此时信号已恢复，可以正常响应

## 修改的文件
- `/home/ruio/mNetAssist_v1.2/mNetAssistWidget.cpp`

## 修改的函数
1. `addToSendHistory` - 添加信号阻塞
2. `on_cBoxSendHistory_currentTextChanged` - 改用文本匹配
3. `addToConnectionHistory` - 添加信号阻塞
4. `loadConnectionHistory` - 添加信号阻塞和初始化检查

## 测试建议
1. **发送历史测试**：
   - 发送多条不同长度的数据（包括超过50字符的）
   - 从历史下拉框中选择，验证是否正确显示完整内容
   - 发送重复内容，验证历史列表是否正确去重并置顶

2. **连接历史测试**：
   - 在不同协议模式（UDP、TCP服务器、TCP客户端）下建立连接
   - 验证IP和端口历史是否正确保存
   - 切换协议模式，验证历史是否正确显示
   - 从历史下拉框中选择，验证输入框是否正确填充

3. **跨会话测试**：
   - 关闭程序后重新打开
   - 验证历史记录是否正确加载
   - 验证最后使用的协议类型和IP/端口是否正确恢复

## 编译结果
修复后的代码已成功编译，只有一些Qt版本相关的废弃API警告（QString::split），不影响功能使用。

编译命令：
```bash
make clean
make
```

## 版本信息
- 修复日期：2026-02-05
- 修复版本：v1.2
- Qt版本：Qt5
