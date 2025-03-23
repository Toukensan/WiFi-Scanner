# WiFiScanner

**WiFiScanner** 是一个专为 Windows 平台设计的 WiFi 扫描工具，利用 Windows WLAN API 实现无线网络扫描，并通过 ImGui 提供友好的图形用户界面来展示扫描结果。该工具支持数据排序、刷新功能，并能将扫描数据保存为 CSV 文件。此外，项目中的 **WifiScanner.hpp** 文件封装了 WiFi 扫描的核心逻辑，便于在其他项目中复用。

---

## 功能特性

- **WiFi 扫描功能**：
  - 利用 Windows WLAN API 捕获附近的 WiFi 网络信息，包括 SSID、BSSID、信道和信号强度等。
- **数据展示与排序**：
  - 使用 ImGui 实现扫描结果的表格化展示，支持按 SSID、BSSID、信道和信号强度排序。
- **CSV 数据导出**：
  - 根据当前时间戳自动生成文件名，将扫描结果保存至 `WIFI/` 文件夹中，并提供操作反馈。
- **模块化设计**：
  - **WifiScanner.hpp** 封装了所有 WiFi 扫描逻辑，可作为独立组件集成至其他项目中，便于扩展和复用。

---

## 效果预览

（请在此处插入效果展示图片）

---

## 构建指南

1. **克隆仓库**
   
   ```bash
   git clone https://github.com/yourusername/your-repo.git
   cd your-repo
   ```

2. **创建构建目录并编译**
   
   ```bash
   mkdir buildcd buildcmake ..make
   ```

3. **运行程序**：执行生成的可执行文件，启动 WiFi 扫描器。

---

## 核心代码解析

### WiFi 扫描功能（WifiScanner.hpp）

**WifiScanner.hpp** 文件封装了 WiFi 扫描的核心逻辑，主要包括：

- **WLAN 客户端初始化**：
  - 打开 WLAN 句柄，注册扫描完成回调。
- **强制刷新扫描**：
  - 遍历无线接口，发起扫描请求，等待扫描完成后更新 AP 列表。
- **数据解析**：
  - 解析 WLAN API 返回的扫描结果，提取并格式化信道、信号强度、SSID 和 BSSID 信息。
- **模块化设计**：
  - **WifiScanner.hpp** 可作为独立组件使用，只需引入该头文件并链接 `Wlanapi` 库即可实现无线网络扫描功能。

### 独立使用示例

以下示例展示了如何单独使用 **WifiScanner.hpp** 执行 WiFi 扫描并输出扫描结果：

```cpp
#include "WifiScanner.hpp"
#include <iostream>

int main() {
    // 创建 WifiScanner 实例
    WifiScanner scanner;

    // 检查 WLAN 句柄是否初始化成功
    if (!scanner.IsValid()) {
        std::cerr << "WLAN 句柄初始化失败！" << std::endl;
        return -1;
    }

    // 触发扫描并等待完成
    std::cout << "正在扫描 WiFi 接入点..." << std::endl;
    scanner.ForceRefresh();

    // 获取扫描结果
    const auto& apList = scanner.GetAPList();
    std::cout << "共扫描到 " << apList.size() << " 个接入点:" << std::endl;

    // 输出接入点信息
    for (const auto& ap : apList) {
        std::cout << "SSID: " << ap.ssid 
                  << ", BSSID: " << ap.bssid 
                  << ", 信道: " << (ap.channel > 0 ? std::to_string(ap.channel) : "未知")
                  << ", 信号强度: " << ap.signal << " dBm" 
                  << std::endl;
    }

    return 0;
}
```

只需将 **WifiScanner.hpp** 包含进项目，并链接 `Wlanapi` 库，即可在其他项目中复用该功能。

---

## 许可证信息

本项目遵循 MIT 许可证，详细信息请参见 [LICENSE](https://github.com/yourusername/your-repo/blob/main/LICENSE) 文件。

---

## 贡献指南

欢迎提出 Issue 或 Pull Request 以改进项目。如有任何建议或疑问，请随时联系我们。

*基于 ImGui 和 C++ 开发 | 由 Touken 创建*
