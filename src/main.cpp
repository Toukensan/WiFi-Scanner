#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"
#include <d3d11.h>
#include <dwmapi.h>
#include <tchar.h>
#include "Application.hpp"

struct D3DResources {
    ID3D11Device* device = nullptr;
    ID3D11DeviceContext* context = nullptr;
    IDXGISwapChain* swapChain = nullptr;
    ID3D11RenderTargetView* mainRenderTargetView = nullptr;
    UINT resizeWidth = 0;
    UINT resizeHeight = 0;
    bool swapChainOccluded = false;

    bool CreateDevice(HWND hWnd);
    void CleanupDevice();
    void CreateRenderTarget();
    void CleanupRenderTarget();
};

bool D3DResources::CreateDevice(HWND hWnd) {
    DXGI_SWAP_CHAIN_DESC sd = {};
    sd.BufferCount = 2;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = hWnd;
    sd.SampleDesc.Count = 1;
    sd.SampleDesc.Quality = 0;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    UINT createDeviceFlags = 0;
    D3D_FEATURE_LEVEL featureLevel;
    const D3D_FEATURE_LEVEL featureLevels[] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0 };
    HRESULT hr = D3D11CreateDeviceAndSwapChain(
        nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, createDeviceFlags,
        featureLevels, 2, D3D11_SDK_VERSION, &sd, &swapChain,
        &device, &featureLevel, &context
    );

    if (hr == DXGI_ERROR_UNSUPPORTED)
        hr = D3D11CreateDeviceAndSwapChain(
            nullptr, D3D_DRIVER_TYPE_WARP, nullptr, createDeviceFlags,
            featureLevels, 2, D3D11_SDK_VERSION, &sd, &swapChain,
            &device, &featureLevel, &context
        );

    if (FAILED(hr)) return false;
    CreateRenderTarget();
    return true;
}

void D3DResources::CleanupDevice() {
    CleanupRenderTarget();
    if (swapChain) { swapChain->Release(); swapChain = nullptr; }
    if (context) { context->Release(); context = nullptr; }
    if (device) { device->Release(); device = nullptr; }
}

void D3DResources::CreateRenderTarget() {
    ID3D11Texture2D* backBuffer;
    swapChain->GetBuffer(0, IID_PPV_ARGS(&backBuffer));
    device->CreateRenderTargetView(backBuffer, nullptr, &mainRenderTargetView);
    backBuffer->Release();
}

void D3DResources::CleanupRenderTarget() {
    if (mainRenderTargetView) { 
        mainRenderTargetView->Release(); 
        mainRenderTargetView = nullptr; 
    }
}

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND, UINT, WPARAM, LPARAM);
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return 1;

    D3DResources* pD3D = reinterpret_cast<D3DResources*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));

    switch (msg) {
    case WM_SIZE:
        if (wParam == SIZE_MINIMIZED) return 0;
        if (pD3D) {
            pD3D->resizeWidth = LOWORD(lParam);
            pD3D->resizeHeight = HIWORD(lParam);
        }
        return 0;
    case WM_SYSCOMMAND:
        if ((wParam & 0xfff0) == SC_KEYMENU) return 0;
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(hWnd, msg, wParam, lParam);
}

int main() {
    WNDCLASSEXW wc = {
        sizeof(wc), CS_CLASSDC, WndProc, 0L, 0L,
        GetModuleHandle(nullptr), LoadIcon(GetModuleHandle(nullptr), "MYICON"), nullptr, nullptr, nullptr,
        L"ImGuiApp", (HICON)LoadImage(GetModuleHandle(nullptr), "MYICON", IMAGE_ICON, 16, 16, LR_SHARED)
    };
    RegisterClassExW(&wc);

    HWND hwnd = CreateWindowW(wc.lpszClassName, L"WiFi Scanner",
        WS_OVERLAPPEDWINDOW, 100, 100, 600, 400, nullptr, nullptr, wc.hInstance, nullptr);

    D3DResources d3d;
    SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)&d3d);

    BOOL darkMode = TRUE;
    DwmSetWindowAttribute(hwnd, 20/*DWMWA_USE_IMMERSIVE_DARK_MODE*/, &darkMode, sizeof(darkMode));

    if (!d3d.CreateDevice(hwnd)) {
        d3d.CleanupDevice();
        UnregisterClassW(wc.lpszClassName, wc.hInstance);
        return 1;
    }

    ShowWindow(hwnd, SW_SHOWDEFAULT);
    UpdateWindow(hwnd);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.IniFilename = nullptr;
    ImGui::StyleColorsDark();

    ImGui_ImplWin32_Init(hwnd);
    ImGui_ImplDX11_Init(d3d.device, d3d.context);

    bool done = false;
    while (!done) {
        MSG msg;
        while (PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
            if (msg.message == WM_QUIT)
                done = true;
        }
        if (done) break;

        if (d3d.resizeWidth && d3d.resizeHeight) {
            d3d.CleanupRenderTarget();
            d3d.swapChain->ResizeBuffers(0, d3d.resizeWidth, d3d.resizeHeight, DXGI_FORMAT_UNKNOWN, 0);
            d3d.resizeWidth = d3d.resizeHeight = 0;
            d3d.CreateRenderTarget();
        }

        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        RenderUI();

        ImGui::Render();
        const float clearColor[] = { 0.45f, 0.55f, 0.60f, 1.00f };
        d3d.context->OMSetRenderTargets(1, &d3d.mainRenderTargetView, nullptr);
        d3d.context->ClearRenderTargetView(d3d.mainRenderTargetView, clearColor);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

        HRESULT hr = d3d.swapChain->Present(1, 0);
        d3d.swapChainOccluded = (hr == DXGI_STATUS_OCCLUDED);
    }

    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    d3d.CleanupDevice();
    DestroyWindow(hwnd);
    UnregisterClassW(wc.lpszClassName, wc.hInstance);

    return 0;
}