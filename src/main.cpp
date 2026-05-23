#define NOMINMAX
#include <Windows.h>
#include <iostream>
#include <string>
#include <chrono>
#include <filesystem>
#include <thread>

#include "memory.h"
#include "names.h"
#include "objects.h"
#include "engine.h"
#include "generator.h"
#include "offsets.h"

// ============================================================
// Console colors
// ============================================================
enum Color {
    WHITE = 7,
    GREEN = 10,
    CYAN = 11,
    RED = 12,
    YELLOW = 14,
    BRIGHT_WHITE = 15
};

void SetColor(int color) {
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), color);
}

void PrintColored(const std::string& text, int color) {
    SetColor(color);
    std::cout << text;
    SetColor(WHITE);
}

// ============================================================
// UI
// ============================================================
void PrintBanner() {
    SetColor(CYAN);
    std::cout << R"(
   ______           __       _ __          ____                                 
  / ____/___  _____/ /_____ (_) /____     / __ \__  ______ ___  ____  ___  _____
 / /_  / __ \/ ___/ __/ __ \/ / __/ _ \  / / / / / / / __ `__ \/ __ \/ _ \/ ___/
/ __/ / /_/ / /  / /_/ / / / / /_/  __/ / /_/ / /_/ / / / / / / /_/ /  __/ /    
\/_/  \____/_/   \__/_/ /_/_/\__/\___/ /_____/\__,_/_/ /_/ /_/ .___/\___/_/     
                                                             /_/                 
)";
    SetColor(BRIGHT_WHITE);
    std::cout << "  ===============================================================\n";
    SetColor(YELLOW);
    std::cout << "    UE5 SDK & Offset Dumper | Fortnite\n";
    SetColor(BRIGHT_WHITE);
    std::cout << "  ===============================================================\n\n";
    SetColor(WHITE);
}

void PrintStep(int step, int total, const std::string& msg) {
    SetColor(BRIGHT_WHITE);
    std::cout << "  [";
    SetColor(CYAN);
    std::cout << step << "/" << total;
    SetColor(BRIGHT_WHITE);
    std::cout << "] ";
    SetColor(WHITE);
    std::cout << msg << "\n";
}

void PrintSuccess(const std::string& msg) {
    std::cout << "      ";
    SetColor(GREEN);
    std::cout << "[+] ";
    SetColor(WHITE);
    std::cout << msg << "\n";
}

void PrintError(const std::string& msg) {
    std::cout << "      ";
    SetColor(RED);
    std::cout << "[!] ";
    SetColor(WHITE);
    std::cout << msg << "\n";
}

void PrintInfo(const std::string& msg) {
    std::cout << "      ";
    SetColor(YELLOW);
    std::cout << "[*] ";
    SetColor(WHITE);
    std::cout << msg << "\n";
}

void PrintSeparator() {
    SetColor(BRIGHT_WHITE);
    std::cout << "  ---------------------------------------------------------------\n";
    SetColor(WHITE);
}

int main(int argc, char* argv[]) {
    // Set console title
    SetConsoleTitleA("Fortnite SDK Dumper");

    // Enable virtual terminal for better rendering
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD dwMode = 0;
    GetConsoleMode(hOut, &dwMode);
    SetConsoleMode(hOut, dwMode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);

    PrintBanner();

    // Get the directory where the exe is located
    char exePathBuf[MAX_PATH];
    GetModuleFileNameA(NULL, exePathBuf, MAX_PATH);
    std::filesystem::path exePath = std::filesystem::path(exePathBuf).parent_path();

    // Create "offsets" folder beside the exe
    std::filesystem::path offsetsDir = exePath / "offsets";
    std::filesystem::create_directories(offsetsDir);
    std::string outputDir = offsetsDir.string();

    // Parse arguments
    bool offsetsOnly = false;
    bool sdkOnly = false;

    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "--offsets-only") offsetsOnly = true;
        else if (arg == "--sdk-only") sdkOnly = true;
        else if (arg == "--output" && i + 1 < argc) outputDir = argv[++i];
    }

    auto startTime = std::chrono::high_resolution_clock::now();
    int totalSteps = 6;

    // ============================================================
    // Step 1: Attach to Fortnite process
    // ============================================================
    PrintStep(1, totalSteps, "Attaching to Fortnite...");
    if (!Memory::Attach()) {
        PrintError("Could not attach to Fortnite!");
        PrintError("Make sure Fortnite is running.");
        std::cout << "\n";
        PrintSeparator();
        SetColor(RED);
        std::cout << "  Press Enter to exit...\n";
        SetColor(WHITE);
        std::cin.get();
        return 1;
    }
    PrintSuccess("Attached! Base: 0x" + ([&]() {
        char buf[32]; snprintf(buf, sizeof(buf), "%llX", (unsigned long long)Memory::baseAddress); return std::string(buf);
    })() + " | Size: " + std::to_string(Memory::moduleSize / 1024 / 1024) + " MB");

    // ============================================================
    // Step 2: Resolve GNames
    // ============================================================
    std::cout << "\n";
    PrintStep(2, totalSteps, "Resolving GNames...");
    if (!Names::Initialize()) {
        PrintError("Failed to find GNames. Game version may be unsupported.");
        Memory::Detach();
        std::cin.get();
        return 1;
    }

    std::string testName = Names::GetNameFromId(0);
    if (!testName.empty()) {
        PrintSuccess("GNames verified. Name[0] = \"" + testName + "\"");
    } else {
        PrintInfo("GNames read test returned empty. Continuing...");
    }

    // ============================================================
    // Step 3: Resolve GObjects
    // ============================================================
    std::cout << "\n";
    PrintStep(3, totalSteps, "Resolving GObjects...");
    if (!Objects::Initialize()) {
        PrintError("Failed to find GObjects.");
        Memory::Detach();
        std::cin.get();
        return 1;
    }
    PrintSuccess("Object count: " + std::to_string(Objects::ObjectCount));

    // ============================================================
    // Step 4: Find core engine classes
    // ============================================================
    std::cout << "\n";
    PrintStep(4, totalSteps, "Finding core engine classes...");
    if (!Objects::FindCoreClasses()) {
        PrintInfo("Some core classes not found. Dump may be incomplete.");
    } else {
        PrintSuccess("All core classes found!");
    }

    // ============================================================
    // Step 5: Dump offsets
    // ============================================================
    if (!sdkOnly) {
        std::cout << "\n";
        PrintStep(5, totalSteps, "Dumping offsets...");
        std::string offsetsFile = outputDir + "\\offsets.h";
        if (Offsets::DumpOffsets(offsetsFile)) {
            PrintSuccess("Offsets saved to: " + offsetsFile);
        } else {
            PrintError("No offsets were found.");
        }
    }

    // ============================================================
    // Step 6: Generate full SDK
    // ============================================================
    if (!offsetsOnly) {
        std::cout << "\n";
        PrintStep(6, totalSteps, "Generating full SDK dump...");
        PrintInfo("This may take a few minutes...");
        Generator::GenerateSDK(outputDir);
        PrintSuccess("SDK dump complete!");
    }

    // ============================================================
    // Done
    // ============================================================
    auto endTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::seconds>(endTime - startTime);

    std::cout << "\n";
    PrintSeparator();
    SetColor(GREEN);
    std::cout << R"(
      ____  _   _ __  __ ____    ____ ___  __  __ ____  _     _____ _____ _____ 
     |  _ \| | | |  \/  |  _ \  / ___/ _ \|  \/  |  _ \| |   | ____|_   _| ____|
     | | | | | | | |\/| | |_) || |  | | | | |\/| | |_) | |   |  _|   | | |  _|  
     | |_| | |_| | |  | |  __/ | |__| |_| | |  | |  __/| |___| |___  | | | |___ 
     |____/ \___/|_|  |_|_|     \____\___/|_|  |_|_|   |_____|_____| |_| |_____|
)";
    SetColor(WHITE);
    std::cout << "\n";
    PrintSeparator();
    PrintSuccess("Time: " + std::to_string(duration.count()) + " seconds");
    PrintSuccess("Output: " + outputDir + "\\");
    PrintSeparator();

    std::cout << "\n";
    SetColor(BRIGHT_WHITE);
    std::cout << "  Press Enter to exit...\n";
    SetColor(WHITE);

    Memory::Detach();
    std::cin.get();

    return 0;
}
