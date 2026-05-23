#define NOMINMAX
#include <Windows.h>
#include <shellapi.h>
#include <iostream>
#include <string>
#include <chrono>
#include <thread>
#include <filesystem>

#include "memory.h"
#include "names.h"
#include "objects.h"
#include "engine.h"
#include "generator.h"
#include "offsets.h"
#include "gui.h"

// ============================================================
// Dump worker thread - runs the actual dump logic
// ============================================================
void DumpWorkerThread() {
    auto& state = GUI::GetState();
    auto startTime = std::chrono::high_resolution_clock::now();

    // Get exe directory for output
    char exePathBuf[MAX_PATH];
    GetModuleFileNameA(NULL, exePathBuf, MAX_PATH);
    std::filesystem::path exePath = std::filesystem::path(exePathBuf).parent_path();
    std::filesystem::path offsetsDir = exePath / "offsets";
    std::filesystem::create_directories(offsetsDir);
    state.outputDir = offsetsDir.string();

    GUI::LogInfo("Output directory: " + state.outputDir);
    GUI::Log("");

    // ============================================================
    // Step 1: Attach to Fortnite
    // ============================================================
    state.currentStep.store(1);
    state.progress.store(0.05f);
    GUI::LogInfo("Step 1/6: Attaching to Fortnite...");

    if (!Memory::Attach()) {
        GUI::LogError("Could not attach to Fortnite!");
        GUI::LogError("Make sure Fortnite is running.");
        state.failed.store(true);
        state.running.store(false);
        return;
    }

    char baseBuf[64];
    snprintf(baseBuf, sizeof(baseBuf), "0x%llX", (unsigned long long)Memory::baseAddress);
    GUI::LogSuccess("Attached! Base: " + std::string(baseBuf) +
        " | Size: " + std::to_string(Memory::moduleSize / 1024 / 1024) + " MB");

    // ============================================================
    // Step 2: Resolve GNames
    // ============================================================
    state.currentStep.store(2);
    state.progress.store(0.15f);
    GUI::Log("");
    GUI::LogInfo("Step 2/6: Resolving GNames...");

    if (!Names::Initialize()) {
        GUI::LogError("Failed to find GNames. Game version may be unsupported.");
        state.failed.store(true);
        state.running.store(false);
        Memory::Detach();
        return;
    }

    std::string testName = Names::GetNameFromId(0);
    if (!testName.empty()) {
        GUI::LogSuccess("GNames verified. Name[0] = \"" + testName + "\"");
    } else {
        GUI::LogWarning("GNames read test returned empty. Continuing...");
    }

    // ============================================================
    // Step 3: Resolve GObjects
    // ============================================================
    state.currentStep.store(3);
    state.progress.store(0.25f);
    GUI::Log("");
    GUI::LogInfo("Step 3/6: Resolving GObjects...");

    if (!Objects::Initialize()) {
        GUI::LogError("Failed to find GObjects.");
        state.failed.store(true);
        state.running.store(false);
        Memory::Detach();
        return;
    }

    GUI::LogSuccess("Object count: " + std::to_string(Objects::ObjectCount));

    // ============================================================
    // Step 4: Find core classes
    // ============================================================
    state.currentStep.store(4);
    state.progress.store(0.35f);
    GUI::Log("");
    GUI::LogInfo("Step 4/6: Finding core engine classes...");

    if (!Objects::FindCoreClasses()) {
        GUI::LogWarning("Some core classes not found. Dump may be incomplete.");
    } else {
        GUI::LogSuccess("All core classes located!");
    }

    // ============================================================
    // Step 5: Dump offsets
    // ============================================================
    state.currentStep.store(5);
    state.progress.store(0.50f);
    GUI::Log("");
    GUI::LogInfo("Step 5/6: Dumping offsets...");

    std::string offsetsFile = state.outputDir + "\\offsets.h";
    if (Offsets::DumpOffsets(offsetsFile)) {
        const auto& offsets = Offsets::GetAllOffsets();
        int found = 0;
        for (const auto& o : offsets) {
            if (o.Offset >= 0) found++;
        }
        state.offsetsFound.store(found);
        state.offsetsTotal.store(static_cast<int>(offsets.size()));

        // Log each found offset
        for (const auto& o : offsets) {
            if (o.Offset >= 0) {
                char buf[128];
                snprintf(buf, sizeof(buf), "  %s = 0x%X", o.Name.c_str(), o.Offset);
                GUI::LogSuccess(buf);
            } else {
                GUI::Log("  " + o.Name + " = NOT FOUND", 2);
            }
        }

        GUI::LogSuccess("Offsets saved to: " + offsetsFile);
    } else {
        GUI::LogError("No offsets found.");
    }

    // ============================================================
    // Step 6: Generate full SDK
    // ============================================================
    state.currentStep.store(6);
    state.progress.store(0.65f);
    GUI::Log("");
    GUI::LogInfo("Step 6/6: Generating full SDK dump...");
    GUI::LogWarning("This may take a few minutes...");

    Generator::GenerateSDK(state.outputDir);

    state.progress.store(1.0f);

    // ============================================================
    // Done
    // ============================================================
    auto endTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
    state.elapsedSeconds = duration.count() / 1000.0f;

    GUI::Log("");
    GUI::Log("========================================", 4);
    GUI::LogSuccess("DUMP COMPLETE!");
    char timeBuf[64];
    snprintf(timeBuf, sizeof(timeBuf), "Time elapsed: %.1f seconds", state.elapsedSeconds);
    GUI::LogSuccess(timeBuf);
    GUI::LogSuccess("Output: " + state.outputDir);
    GUI::Log("========================================", 4);

    Memory::Detach();
    state.running.store(false);
    state.finished.store(true);
}

// ============================================================
// WinMain entry point (GUI app - no console window)
// ============================================================
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    (void)hInstance;
    (void)hPrevInstance;
    (void)lpCmdLine;
    (void)nCmdShow;

    // Create GUI window
    HWND hwnd = GUI::CreateAppWindow();
    if (!hwnd) return 1;

    ShowWindow(hwnd, SW_SHOWDEFAULT);
    UpdateWindow(hwnd);

    if (!GUI::Initialize(hwnd)) {
        DestroyWindow(hwnd);
        return 1;
    }

    GUI::Log("Fortnite SDK Dumper initialized.", 4);
    GUI::Log("Click START DUMP with Fortnite running.", 0);
    GUI::Log("");

    // Worker thread handle
    std::thread workerThread;

    // Main message loop
    MSG msg{};
    while (true) {
        if (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT) break;
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
            continue;
        }

        // Check if user clicked Start
        auto& state = GUI::GetState();
        if (state.running.load() && !workerThread.joinable()) {
            workerThread = std::thread(DumpWorkerThread);
        }

        GUI::Render();
    }

    // Cleanup
    if (workerThread.joinable()) {
        workerThread.join();
    }

    GUI::Shutdown();
    DestroyWindow(hwnd);

    return 0;
}
