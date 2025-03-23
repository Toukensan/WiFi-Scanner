#include <vector>
#include <windows.h>
#include <wlanapi.h>
#include <iostream>
#include <guiddef.h>
#include <atomic>
#include <algorithm> // 用于std::min

// 接入点信息结构体
struct APInfo {
    int channel;        // 信道（-1表示未知）
    int signal;         // 信号强度（dBm）
    char ssid[33];      // 网络名称，增加1字节给null终止符
    char bssid[18];     // MAC地址
};

class WifiScanner {
public:
    WifiScanner() : m_scanEvent(CreateEvent(nullptr, TRUE, FALSE, nullptr)) {
        // 初始化WLAN客户端
        if (WlanOpenHandle(2, nullptr, &m_version, &m_hClient) != ERROR_SUCCESS) {
            std::cerr << "[Error] Failed to open WLAN handle" << std::endl;
            return;
        }

        // 注册扫描完成通知
        WlanRegisterNotification(
            m_hClient, 
            WLAN_NOTIFICATION_SOURCE_ACM,
            FALSE,                // 不忽略重复通知
            &ScanCompleteCallback, // 回调函数
            this,                 // 上下文参数
            nullptr, 
            nullptr
        );

        // 首次主动扫描
        ForceRefresh();
    }

    ~WifiScanner() {
        if (m_hClient) {
            WlanCloseHandle(m_hClient, nullptr);
        }
        CloseHandle(m_scanEvent);
    }

    // 强制刷新网络列表
    void ForceRefresh() {
        if (!IsValid()) return;

        PWLAN_INTERFACE_INFO_LIST pIfList = nullptr;
        if (WlanEnumInterfaces(m_hClient, nullptr, &pIfList) != ERROR_SUCCESS) {
            std::cerr << "[Error] Failed to enumerate interfaces" << std::endl;
            return;
        }

        // 重置事件状态
        ResetEvent(m_scanEvent);
        m_scanCompleted = false;

        // 在所有接口上触发扫描
        for (DWORD i = 0; i < pIfList->dwNumberOfItems; ++i) {
            const GUID& ifGuid = pIfList->InterfaceInfo[i].InterfaceGuid;
            
            DWORD result = WlanScan(m_hClient, &ifGuid, nullptr, nullptr, nullptr);
            if (result != ERROR_SUCCESS) {
                std::cerr << "[Warning] Scan failed on interface " << i 
                          << " (Error: " << result << ")" << std::endl;
            }
        }

        // 等待扫描完成（最多10秒）
        WaitForSingleObject(m_scanEvent, 10000);
        WlanFreeMemory(pIfList);

        // 获取最新结果
        UpdateAPList();
    }

    // 获取当前AP列表
    const std::vector<APInfo>& GetAPList() const {
        return m_apList;
    }

    bool IsValid() const {
        return m_hClient != nullptr;
    }

private:
    // 扫描完成回调函数
    static void WINAPI ScanCompleteCallback(PWLAN_NOTIFICATION_DATA data, PVOID context) {
        if (data->NotificationCode == wlan_notification_acm_scan_complete) {
            auto scanner = static_cast<WifiScanner*>(context);
            SetEvent(scanner->m_scanEvent);  // 触发事件通知
            scanner->m_scanCompleted = true;
        }
    }

    // 更新AP列表数据
    void UpdateAPList() {
        m_apList.clear();
        PWLAN_INTERFACE_INFO_LIST pIfList = nullptr;

        if (WlanEnumInterfaces(m_hClient, nullptr, &pIfList) != ERROR_SUCCESS) return;

        // 遍历所有无线接口
        for (DWORD i = 0; i < pIfList->dwNumberOfItems; ++i) {
            const GUID& ifGuid = pIfList->InterfaceInfo[i].InterfaceGuid;
            PWLAN_BSS_LIST pBssList = nullptr;

            // 获取扫描结果
            if (WlanGetNetworkBssList(m_hClient, &ifGuid, nullptr,
                dot11_BSS_type_any, true, nullptr, &pBssList) == ERROR_SUCCESS) {
                ParseBssList(pBssList);
                WlanFreeMemory(pBssList);
            }
        }

        WlanFreeMemory(pIfList);
    }

    // 解析BSS条目
    void ParseBssList(PWLAN_BSS_LIST pBssList) {
        for (DWORD i = 0; i < pBssList->dwNumberOfItems; ++i) {
            const WLAN_BSS_ENTRY& entry = pBssList->wlanBssEntries[i];
            APInfo info{};

            // 信道计算
            info.channel = CalculateChannel(entry.ulChCenterFrequency);
            
            // 信号强度
            info.signal = static_cast<int>(entry.lRssi);
            
            // 格式化SSID和BSSID
            FormatSsid(entry.dot11Ssid, info.ssid);
            FormatBssid(entry.dot11Bssid, info.bssid);

            m_apList.push_back(info);
        }
    }

    // 频率转信道计算
    static int CalculateChannel(ULONG freqKHz) noexcept {
        const ULONG freqMHz = freqKHz / 1000;

        // 2.4 GHz频段
        if (freqMHz >= 2412 && freqMHz <= 2484) {
            return freqMHz == 2484 ? 14 : (freqMHz - 2407) / 5;
        }

        // 5 GHz频段
        if (freqMHz >= 5180 && freqMHz <= 5885) {
            if (freqMHz <= 5320) return (freqMHz - 5180) / 5 + 36;
            if (freqMHz <= 5720) return (freqMHz - 5500) / 5 + 100;
            return (freqMHz - 5745) / 5 + 149;
        }

        // 6 GHz频段（WiFi 6E）
        if (freqMHz >= 5955 && freqMHz <= 7115) {
            return (freqMHz - 5955) / 5 + 1;
        }

        return -1;  // 未知频段
    }

    // 安全格式化SSID，支持UTF-8编码
    static void FormatSsid(const DOT11_SSID& dot11Ssid, char* output) noexcept {
        const size_t maxLen = sizeof(APInfo::ssid) - 1; // 留1字节给null终止符
        size_t copyLen = std::min<size_t>(dot11Ssid.uSSIDLength, maxLen);
        
        memcpy(output, dot11Ssid.ucSSID, copyLen);
        output[copyLen] = '\0';

        // 处理不可打印字符，保留UTF-8多字节字符
        for (size_t i = 0; i < copyLen; ++i) {
            if (output[i] < 0) { // 可能是UTF-8多字节字符的开始
                // 判断UTF-8字符长度
                int bytes = 0;
                if ((output[i] & 0xE0) == 0xC0) bytes = 2;      // 2字节字符
                else if ((output[i] & 0xF0) == 0xE0) bytes = 3; // 3字节字符
                else if ((output[i] & 0xF8) == 0xF0) bytes = 4; // 4字节字符
                else {
                    output[i] = '?'; // 无效的UTF-8字节
                    continue;
                }

                // 检查后续字节是否完整
                bool valid = true;
                for (int j = 1; j < bytes; ++j) {
                    if (i + j >= copyLen || (output[i + j] & 0xC0) != 0x80) {
                        valid = false;
                        break;
                    }
                }
                if (valid) {
                    i += bytes - 1; // 跳过整个多字节字符
                } else {
                    output[i] = '?'; // 无效的UTF-8序列，替换为'?'
                }
            } else if (output[i] < 32 || output[i] > 126) {
                output[i] = '?'; // 替换不可打印的ASCII字符
            }
        }
    }

    // 格式化MAC地址
    static void FormatBssid(const DOT11_MAC_ADDRESS& mac, char* output) noexcept {
        snprintf(output, 18, "%02X:%02X:%02X:%02X:%02X:%02X",
                mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    }

    HANDLE m_hClient = nullptr;           // WLAN客户端句柄
    DWORD m_version = 0;                  // API版本
    HANDLE m_scanEvent = nullptr;         // 扫描完成事件
    std::atomic<bool> m_scanCompleted{false};
    std::vector<APInfo> m_apList;         // AP列表缓存
};