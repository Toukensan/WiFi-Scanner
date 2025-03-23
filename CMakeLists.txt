cmake_minimum_required(VERSION 3.16)
project(WiFiScanner)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# -------------------------------
# Windows/Mingw 专属静态链接优化
# -------------------------------
if(MINGW)
    # 完全静态链接
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static")
    
    # 对标准库及其他可选组件进行静态化
    add_link_options(
        -static-libgcc
        -static-libstdc++
        -Wl,-Bstatic -lstdc++ -lpthread
    )
endif()
# -------------------------------
# 收集主程序源文件
# -------------------------------
file(GLOB SRC_FILES
    src/*.cpp
)

# -------------------------------
# 收集 ImGui 核心和 DirectX11 后端
# -------------------------------
file(GLOB IMGUI_SRC
    libs/imgui/*.cpp
    libs/imgui/backends/imgui_impl_dx11.cpp
    libs/imgui/backends/imgui_impl_win32.cpp
)

# -------------------------------
# 添加可执行文件
# -------------------------------
add_executable(${PROJECT_NAME}
    WIN32
    ${SRC_FILES}
    ${IMGUI_SRC}
    resources/resources.rc
)

# -------------------------------
# 包含头文件目录
# -------------------------------
target_include_directories(${PROJECT_NAME} PRIVATE
    libs/imgui
    libs/imgui/backends
)

# -------------------------------
# 链接 Windows / DirectX 库
# -------------------------------
target_link_libraries(${PROJECT_NAME}
    d3d11
    D3DCompiler
    dxgi      # 通常用于交换链、设备相关
    dxguid    # 如果需要 CLSID, IID 之类接口
    Dwmapi    # 如果使用 Windows DWM 特效
    user32    # GUI交互，窗口系统
    gdi32     # 基本图形界面
    Wlanapi
)

