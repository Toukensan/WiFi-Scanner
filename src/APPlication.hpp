#include <imgui.h>
#include "WifiScanner.hpp"
#include <algorithm>
#include <cstring>
#include <chrono>       // 用于生成时间戳
#include <ctime>        // 用于格式化时间
#include <iomanip>      // 用于时间格式控制
#include <fstream>      // 用于文件操作
#include <filesystem>   // 用于目录创建

WifiScanner g_scanner;

// 根据列索引和排序方向进行排序
bool CompareAPInfo(const APInfo& a, const APInfo& b, int columnIndex, ImGuiSortDirection direction) {
    switch (columnIndex) {
        case 0: return direction == ImGuiSortDirection_Ascending ? strcmp(a.ssid, b.ssid) < 0 : strcmp(a.ssid, b.ssid) > 0;
        case 1: return direction == ImGuiSortDirection_Ascending ? strcmp(a.bssid, b.bssid) < 0 : strcmp(a.bssid, b.bssid) > 0;
        case 2: return direction == ImGuiSortDirection_Ascending ? a.channel < b.channel : a.channel > b.channel;
        case 3: return direction == ImGuiSortDirection_Ascending ? a.signal < b.signal : a.signal > b.signal;
        default: return false;
    }
}

// 处理排序逻辑
void SortAPList(std::vector<APInfo>& apList, ImGuiTableSortSpecs* sortSpecs) {
    if (sortSpecs && sortSpecs->SpecsCount > 0) {
        const auto& spec = sortSpecs->Specs[0]; // 仅按第一列排序
        std::sort(apList.begin(), apList.end(), [&](const APInfo& a, const APInfo& b) {
            return CompareAPInfo(a, b, spec.ColumnIndex, spec.SortDirection);
        });
    }
}

// 根据信号强度设置颜色
void SetSignalStrengthColor(int signal) {
    ImU32 color = (signal >= -50) ? IM_COL32(0, 255, 0, 255) :
                  (signal >= -70) ? IM_COL32(255, 255, 0, 255) :
                                    IM_COL32(255, 0, 0, 255);
    ImGui::PushStyleColor(ImGuiCol_Text, color);
}

// 获取当前时间戳
std::string GetTimestamp() {
    auto now = std::chrono::system_clock::now();
    std::time_t now_time = std::chrono::system_clock::to_time_t(now);
    std::tm* now_tm = std::localtime(&now_time);
    std::ostringstream oss;
    oss << std::put_time(now_tm, "%Y%m%d_%H%M%S");
    return oss.str();
}

// 保存WiFi扫描结果到CSV文件
void SaveToCSV(const std::vector<APInfo>& apList) {
    std::string timestamp = GetTimestamp();
    std::string filename = "WIFI/WiFi_" + timestamp + ".csv";

    // 确保WIFI目录存在
    std::filesystem::create_directory("WIFI");

    std::ofstream file(filename);
    if (!file.is_open()) {
        // 文件打开失败，弹出错误提示
        ImGui::OpenPopup("Save Error");
        return;
    }

    // 写入表头
    file << "SSID,BSSID,Channel,Signal (dBm)\n";

    // 写入每个AP的信息
    for (const auto& ap : apList) {
        file << ap.ssid << "," << ap.bssid << "," << ap.channel << "," << ap.signal << "\n";
    }

    file.close();

    // 保存成功，弹出成功提示
    ImGui::OpenPopup("Save Success");
}

void RenderUI() {
    ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize, ImGuiCond_Always);

    if (ImGui::Begin("Made By Touken", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse |
                                              ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove)) {
        if (ImGui::Button("Refresh")) g_scanner.ForceRefresh();
        ImGui::SameLine();
        if (ImGui::Button("Save")) {
            SaveToCSV(g_scanner.GetAPList());
        }
        ImGui::SameLine();
        ImGui::Text("AP Count: %d", static_cast<int>(g_scanner.GetAPList().size()));

        static constexpr ImGuiTableFlags flags = ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg |
                                                 ImGuiTableFlags_ScrollY | ImGuiTableFlags_SizingFixedFit |
                                                 ImGuiTableFlags_Sortable;

        if (ImGui::BeginTable("##APTable", 4, flags)) {
            ImGui::TableSetupColumn("SSID", ImGuiTableColumnFlags_DefaultSort | ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableSetupColumn("BSSID", ImGuiTableColumnFlags_DefaultSort, 150.0f);
            ImGui::TableSetupColumn("Channel", ImGuiTableColumnFlags_DefaultSort, 80.0f);
            ImGui::TableSetupColumn("Signal (dBm)", ImGuiTableColumnFlags_DefaultSort, 100.0f);
            ImGui::TableHeadersRow();

            std::vector<APInfo> apList = g_scanner.GetAPList();
            SortAPList(apList, ImGui::TableGetSortSpecs());

            for (const auto& ap : apList) {
                ImGui::TableNextRow();
                
                ImGui::TableSetColumnIndex(0);
                ImGui::Text("%s", ap.ssid);

                ImGui::TableSetColumnIndex(1);
                ImGui::Text("%s", ap.bssid);

                ImGui::TableSetColumnIndex(2);
                ImGui::Text("%d", ap.channel > 0 ? ap.channel : -1);

                ImGui::TableSetColumnIndex(3);
                SetSignalStrengthColor(ap.signal);
                ImGui::Text("%d dBm", ap.signal);
                ImGui::PopStyleColor();
            }
            ImGui::EndTable();
        }

        // 保存成功的弹窗
        if (ImGui::BeginPopupModal("Save Success", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::Text("WiFi data saved successfully!");
            if (ImGui::Button("OK")) ImGui::CloseCurrentPopup();
            ImGui::EndPopup();
        }

        // 保存失败的弹窗
        if (ImGui::BeginPopupModal("Save Error", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::Text("Failed to save WiFi data.");
            if (ImGui::Button("OK")) ImGui::CloseCurrentPopup();
            ImGui::EndPopup();
        }
    }
    ImGui::End();
}