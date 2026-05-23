#define NOMINMAX
#include "gui.h"
#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"
#include <d3d11.h>
#include <dxgi.h>
#include <vector>
#include <mutex>
#include <chrono>

// Forward declare message handler from imgui_impl_win32.cpp
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

namespace GUI {

    // DirectX state
    static ID3D11Device* g_pd3dDevice = nullptr;
    static ID3D11DeviceContext* g_pd3dDeviceContext = nullptr;
    static IDXGISwapChain* g_pSwapChain = nullptr;
    static ID3D11RenderTargetView* g_mainRenderTargetView = nullptr;

    // App state
    static DumpState g_state;
    static std::vector<LogEntry> g_logs;
    static std::mutex g_logMutex;
    static bool g_scrollToBottom = false;

    // Helpers
    static void CreateRenderTarget() {
        ID3D11Texture2D* pBackBuffer = nullptr;
        g_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
        if (pBackBuffer) {
            g_pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, &g_mainRenderTargetView);
            pBackBuffer->Release();
        }
    }

    static void CleanupRenderTarget() {
        if (g_mainRenderTargetView) {
            g_mainRenderTargetView->Release();
            g_mainRenderTargetView = nullptr;
        }
    }

    static bool CreateDeviceD3D(HWND hWnd) {
        DXGI_SWAP_CHAIN_DESC sd{};
        sd.BufferCount = 2;
        sd.BufferDesc.Width = 0;
        sd.BufferDesc.Height = 0;
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
        const D3D_FEATURE_LEVEL featureLevelArray[] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0 };

        HRESULT hr = D3D11CreateDeviceAndSwapChain(
            nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, createDeviceFlags,
            featureLevelArray, 2, D3D11_SDK_VERSION,
            &sd, &g_pSwapChain, &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext);

        if (hr != S_OK) return false;

        CreateRenderTarget();
        return true;
    }

    static void CleanupDeviceD3D() {
        CleanupRenderTarget();
        if (g_pSwapChain) { g_pSwapChain->Release(); g_pSwapChain = nullptr; }
        if (g_pd3dDeviceContext) { g_pd3dDeviceContext->Release(); g_pd3dDeviceContext = nullptr; }
        if (g_pd3dDevice) { g_pd3dDevice->Release(); g_pd3dDevice = nullptr; }
    }

    static LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
        if (ImGui_ImplWin32_WndProc(hWnd, msg, wParam, lParam))
            return true;

        switch (msg) {
        case WM_SIZE:
            if (g_pd3dDevice != nullptr && wParam != SIZE_MINIMIZED) {
                CleanupRenderTarget();
                g_pSwapChain->ResizeBuffers(0, (UINT)LOWORD(lParam), (UINT)HIWORD(lParam), DXGI_FORMAT_UNKNOWN, 0);
                CreateRenderTarget();
            }
            return 0;
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
        }
        return DefWindowProcW(hWnd, msg, wParam, lParam);
    }

    // ============================================================
    // Public interface
    // ============================================================

    DumpState& GetState() { return g_state; }
    std::vector<LogEntry>& GetLogs() { return g_logs; }
    std::mutex& GetLogMutex() { return g_logMutex; }

    void Log(const std::string& text, int color) {
        std::lock_guard<std::mutex> lock(g_logMutex);
        g_logs.push_back({ text, color });
        g_scrollToBottom = true;
    }

    void LogSuccess(const std::string& text) { Log("[+] " + text, 1); }
    void LogWarning(const std::string& text) { Log("[*] " + text, 2); }
    void LogError(const std::string& text) { Log("[!] " + text, 3); }
    void LogInfo(const std::string& text) { Log("[>] " + text, 4); }

    HWND CreateAppWindow() {
        WNDCLASSEXW wc{};
        wc.cbSize = sizeof(wc);
        wc.style = CS_CLASSDC;
        wc.lpfnWndProc = WndProc;
        wc.hInstance = GetModuleHandleW(nullptr);
        wc.lpszClassName = L"FortniteDumperClass";
        RegisterClassExW(&wc);

        HWND hwnd = CreateWindowExW(
            0, wc.lpszClassName, L"Fortnite SDK Dumper",
            WS_OVERLAPPEDWINDOW,
            100, 100, 900, 620,
            nullptr, nullptr, wc.hInstance, nullptr);

        return hwnd;
    }

    bool Initialize(HWND hwnd) {
        if (!CreateDeviceD3D(hwnd)) return false;

        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

        // Dark theme with custom colors
        ImGui::StyleColorsDark();
        ImGuiStyle& style = ImGui::GetStyle();
        style.WindowRounding = 6.0f;
        style.FrameRounding = 4.0f;
        style.GrabRounding = 4.0f;
        style.TabRounding = 4.0f;
        style.ScrollbarRounding = 4.0f;
        style.WindowPadding = ImVec2(12, 12);
        style.FramePadding = ImVec2(8, 4);
        style.ItemSpacing = ImVec2(8, 6);

        // Custom color palette - dark purple/blue theme
        ImVec4* colors = style.Colors;
        colors[ImGuiCol_WindowBg] = ImVec4(0.06f, 0.06f, 0.10f, 1.00f);
        colors[ImGuiCol_ChildBg] = ImVec4(0.04f, 0.04f, 0.08f, 1.00f);
        colors[ImGuiCol_Border] = ImVec4(0.30f, 0.20f, 0.50f, 0.50f);
        colors[ImGuiCol_FrameBg] = ImVec4(0.10f, 0.08f, 0.15f, 1.00f);
        colors[ImGuiCol_FrameBgHovered] = ImVec4(0.15f, 0.12f, 0.25f, 1.00f);
        colors[ImGuiCol_FrameBgActive] = ImVec4(0.20f, 0.15f, 0.35f, 1.00f);
        colors[ImGuiCol_TitleBg] = ImVec4(0.06f, 0.04f, 0.12f, 1.00f);
        colors[ImGuiCol_TitleBgActive] = ImVec4(0.10f, 0.06f, 0.20f, 1.00f);
        colors[ImGuiCol_Button] = ImVec4(0.40f, 0.20f, 0.80f, 0.80f);
        colors[ImGuiCol_ButtonHovered] = ImVec4(0.50f, 0.30f, 0.90f, 1.00f);
        colors[ImGuiCol_ButtonActive] = ImVec4(0.60f, 0.35f, 1.00f, 1.00f);
        colors[ImGuiCol_Header] = ImVec4(0.30f, 0.15f, 0.60f, 0.60f);
        colors[ImGuiCol_HeaderHovered] = ImVec4(0.40f, 0.20f, 0.70f, 0.80f);
        colors[ImGuiCol_HeaderActive] = ImVec4(0.50f, 0.25f, 0.80f, 1.00f);
        colors[ImGuiCol_Tab] = ImVec4(0.15f, 0.10f, 0.30f, 1.00f);
        colors[ImGuiCol_TabHovered] = ImVec4(0.40f, 0.25f, 0.70f, 1.00f);
        colors[ImGuiCol_TabActive] = ImVec4(0.30f, 0.18f, 0.55f, 1.00f);
        colors[ImGuiCol_PlotHistogram] = ImVec4(0.50f, 0.20f, 1.00f, 1.00f);
        colors[ImGuiCol_PlotHistogramHovered] = ImVec4(0.60f, 0.30f, 1.00f, 1.00f);
        colors[ImGuiCol_ScrollbarBg] = ImVec4(0.04f, 0.04f, 0.08f, 1.00f);
        colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.30f, 0.20f, 0.50f, 1.00f);
        colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.40f, 0.25f, 0.65f, 1.00f);
        colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.50f, 0.30f, 0.80f, 1.00f);
        colors[ImGuiCol_CheckMark] = ImVec4(0.60f, 0.30f, 1.00f, 1.00f);
        colors[ImGuiCol_TextDisabled] = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);

        ImGui_ImplWin32_Init(hwnd);
        ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);

        return true;
    }

    void Shutdown() {
        ImGui_ImplDX11_Shutdown();
        ImGui_ImplWin32_Shutdown();
        ImGui::DestroyContext();
        CleanupDeviceD3D();
    }

    // ============================================================
    // Main render function
    // ============================================================
    void Render() {
        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        // Full-window ImGui panel
        ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(viewport->Pos);
        ImGui::SetNextWindowSize(viewport->Size);

        ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse |
            ImGuiWindowFlags_NoBringToFrontOnFocus;

        ImGui::Begin("##Main", nullptr, flags);

        // Header
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.60f, 0.30f, 1.00f, 1.00f));
        ImGui::SetWindowFontScale(1.3f);
        ImGui::Text("FORTNITE SDK DUMPER");
        ImGui::SetWindowFontScale(1.0f);
        ImGui::PopStyleColor();

        ImGui::SameLine(ImGui::GetWindowWidth() - 200);
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5f, 0.5f, 0.5f, 1.0f));
        ImGui::Text("UE5 | DirectX 11");
        ImGui::PopStyleColor();

        ImGui::Separator();
        ImGui::Spacing();

        // Status panel
        {
            ImGui::BeginChild("##Status", ImVec2(0, 110), true);

            // Progress bar
            ImGui::Text("Progress:");
            ImGui::SameLine();

            float progress = g_state.progress.load();
            char progressText[64];
            if (g_state.finished.load()) {
                snprintf(progressText, sizeof(progressText), "COMPLETE (%.1fs)", g_state.elapsedSeconds);
            } else if (g_state.running.load()) {
                snprintf(progressText, sizeof(progressText), "Step %d/%d - %.0f%%",
                    g_state.currentStep.load(), g_state.totalSteps.load(), progress * 100.0f);
            } else {
                snprintf(progressText, sizeof(progressText), "Ready");
            }

            ImGui::ProgressBar(progress, ImVec2(-1, 22), progressText);

            ImGui::Spacing();

            // Stats row
            ImGui::Columns(4, nullptr, false);

            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.3f, 1.0f, 0.3f, 1.0f));
            ImGui::Text("Offsets");
            ImGui::PopStyleColor();
            ImGui::Text("%d / %d", g_state.offsetsFound.load(), g_state.offsetsTotal.load());
            ImGui::NextColumn();

            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.3f, 0.8f, 1.0f, 1.0f));
            ImGui::Text("Classes");
            ImGui::PopStyleColor();
            ImGui::Text("%d", g_state.classesFound.load());
            ImGui::NextColumn();

            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.8f, 0.3f, 1.0f));
            ImGui::Text("Enums");
            ImGui::PopStyleColor();
            ImGui::Text("%d", g_state.enumsFound.load());
            ImGui::NextColumn();

            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.8f, 0.5f, 1.0f, 1.0f));
            ImGui::Text("Output");
            ImGui::PopStyleColor();
            if (!g_state.outputDir.empty()) {
                ImGui::TextWrapped("%s", g_state.outputDir.c_str());
            } else {
                ImGui::Text("--");
            }
            ImGui::NextColumn();

            ImGui::Columns(1);
            ImGui::EndChild();
        }

        ImGui::Spacing();

        // Console log
        {
            ImGui::Text("Console:");
            ImGui::BeginChild("##Console", ImVec2(0, -40), true);

            std::lock_guard<std::mutex> lock(g_logMutex);
            for (const auto& entry : g_logs) {
                ImVec4 color;
                switch (entry.color) {
                case 1: color = ImVec4(0.3f, 1.0f, 0.3f, 1.0f); break;   // Green
                case 2: color = ImVec4(1.0f, 0.9f, 0.3f, 1.0f); break;   // Yellow
                case 3: color = ImVec4(1.0f, 0.3f, 0.3f, 1.0f); break;   // Red
                case 4: color = ImVec4(0.4f, 0.8f, 1.0f, 1.0f); break;   // Cyan
                default: color = ImVec4(0.85f, 0.85f, 0.85f, 1.0f); break; // White
                }
                ImGui::PushStyleColor(ImGuiCol_Text, color);
                ImGui::TextWrapped("%s", entry.text.c_str());
                ImGui::PopStyleColor();
            }

            if (g_scrollToBottom) {
                ImGui::SetScrollHereY(1.0f);
                g_scrollToBottom = false;
            }

            ImGui::EndChild();
        }

        // Bottom buttons
        {
            bool isRunning = g_state.running.load();
            bool isFinished = g_state.finished.load();

            if (!isRunning && !isFinished) {
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.7f, 0.2f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.85f, 0.3f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.15f, 0.6f, 0.15f, 1.0f));
                if (ImGui::Button("START DUMP", ImVec2(150, 30))) {
                    g_state.running.store(true);
                }
                ImGui::PopStyleColor(3);
            } else if (isFinished) {
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.5f, 0.8f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.6f, 0.9f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.15f, 0.4f, 0.7f, 1.0f));
                if (ImGui::Button("OPEN OUTPUT", ImVec2(150, 30))) {
                    ShellExecuteA(nullptr, "open", g_state.outputDir.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
                }
                ImGui::PopStyleColor(3);
            } else {
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.5f, 0.5f, 0.5f, 0.5f));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.5f, 0.5f, 0.5f, 0.5f));
                ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.5f, 0.5f, 0.5f, 0.5f));
                ImGui::Button("DUMPING...", ImVec2(150, 30));
                ImGui::PopStyleColor(3);
            }

            ImGui::SameLine(ImGui::GetWindowWidth() - 100);
            if (ImGui::Button("EXIT", ImVec2(80, 30))) {
                PostQuitMessage(0);
            }
        }

        ImGui::End();

        // Render
        ImGui::Render();
        const float clear_color[4] = { 0.03f, 0.03f, 0.06f, 1.00f };
        g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, nullptr);
        g_pd3dDeviceContext->ClearRenderTargetView(g_mainRenderTargetView, clear_color);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

        g_pSwapChain->Present(1, 0); // VSync
    }

    int RunGUI() {
        HWND hwnd = CreateAppWindow();
        if (!hwnd) return 1;

        ShowWindow(hwnd, SW_SHOWDEFAULT);
        UpdateWindow(hwnd);

        if (!Initialize(hwnd)) {
            CleanupDeviceD3D();
            DestroyWindow(hwnd);
            return 1;
        }

        // Main message loop
        MSG msg{};
        while (msg.message != WM_QUIT) {
            if (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE)) {
                TranslateMessage(&msg);
                DispatchMessageW(&msg);
                continue;
            }
            Render();
        }

        Shutdown();
        DestroyWindow(hwnd);
        return 0;
    }

} // namespace GUI
