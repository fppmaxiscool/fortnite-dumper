#pragma once
#define NOMINMAX
#include <Windows.h>
#include <d3d11.h>
#include <string>
#include <vector>
#include <mutex>
#include <atomic>

namespace GUI {

    // Log entry for the console
    struct LogEntry {
        std::string text;
        int color; // 0=white, 1=green, 2=yellow, 3=red, 4=cyan
    };

    // Dump state
    struct DumpState {
        std::atomic<bool> running{false};
        std::atomic<bool> finished{false};
        std::atomic<bool> failed{false};
        std::atomic<int> currentStep{0};
        std::atomic<int> totalSteps{6};
        std::atomic<int> offsetsFound{0};
        std::atomic<int> offsetsTotal{0};
        std::atomic<int> classesFound{0};
        std::atomic<int> enumsFound{0};
        std::atomic<float> progress{0.0f};
        std::string errorMsg;
        std::string outputDir;
        float elapsedSeconds{0.0f};
    };

    // Initialize DirectX11 + ImGui
    bool Initialize(HWND hwnd);
    void Shutdown();

    // Render frame
    void Render();

    // Window creation
    HWND CreateAppWindow();

    // Main loop
    int RunGUI();

    // Logging
    void Log(const std::string& text, int color = 0);
    void LogSuccess(const std::string& text);
    void LogWarning(const std::string& text);
    void LogError(const std::string& text);
    void LogInfo(const std::string& text);

    // Access state
    DumpState& GetState();
    std::vector<LogEntry>& GetLogs();
    std::mutex& GetLogMutex();

} // namespace GUI
