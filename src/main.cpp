#include <iostream>
#include <string>
#include <chrono>

#include "memory.h"
#include "names.h"
#include "objects.h"
#include "engine.h"
#include "generator.h"
#include "offsets.h"

void PrintBanner() {
    std::cout << R"(
  _____ ___  ____ _____ _   _ ___ _____ _____   ____  _   _ __  __ ____  _____ ____
 |  ___/ _ \|  _ \_   _| \ | |_ _|_   _| ____| |  _ \| | | |  \/  |  _ \| ____|  _ \
 | |_ | | | | |_) || | |  \| || |  | | |  _|   | | | | | | | |\/| | |_) |  _| | |_) |
 |  _|| |_| |  _ < | | | |\  || |  | | | |___  | |_| | |_| | |  | |  __/| |___|  _ <
 |_|   \___/|_| \_\|_| |_| \_|___| |_| |_____| |____/ \___/|_|  |_|_|   |_____|_| \_\

    Fortnite SDK Dumper - UE5 Offset & Structure Dumper
    ===================================================
)" << std::endl;
}

void PrintUsage() {
    std::cout << "Usage:\n";
    std::cout << "  FortniteDumper.exe [options]\n\n";
    std::cout << "Options:\n";
    std::cout << "  --offsets-only    Only dump offsets (faster)\n";
    std::cout << "  --sdk-only        Only dump full SDK\n";
    std::cout << "  --output <dir>    Output directory (default: SDK_Dump)\n";
    std::cout << "  --help            Show this help\n\n";
    std::cout << "Make sure Fortnite is running before starting the dumper.\n";
}

int main(int argc, char* argv[]) {
    PrintBanner();

    // Parse arguments
    bool offsetsOnly = false;
    bool sdkOnly = false;
    std::string outputDir = "SDK_Dump";

    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "--offsets-only") {
            offsetsOnly = true;
        } else if (arg == "--sdk-only") {
            sdkOnly = true;
        } else if (arg == "--output" && i + 1 < argc) {
            outputDir = argv[++i];
        } else if (arg == "--help" || arg == "-h") {
            PrintUsage();
            return 0;
        }
    }

    auto startTime = std::chrono::high_resolution_clock::now();

    // ============================================================
    // Step 1: Attach to Fortnite process
    // ============================================================
    std::cout << "[*] Step 1: Attaching to Fortnite...\n";
    if (!Memory::Attach()) {
        std::cerr << "\n[!] ERROR: Could not attach to Fortnite.\n";
        std::cerr << "    Make sure Fortnite is running and you have admin privileges.\n";
        std::cerr << "    Press Enter to exit...\n";
        std::cin.get();
        return 1;
    }

    // ============================================================
    // Step 2: Resolve GNames
    // ============================================================
    std::cout << "\n[*] Step 2: Resolving GNames...\n";
    if (!Names::Initialize()) {
        std::cerr << "\n[!] ERROR: Failed to find GNames.\n";
        std::cerr << "    The game version may not be supported.\n";
        std::cerr << "    Try updating the patterns.\n";
        Memory::Detach();
        std::cin.get();
        return 1;
    }

    // Verify GNames is working
    std::string testName = Names::GetNameFromId(0);
    if (testName.empty()) {
        std::cerr << "[!] WARNING: GNames read test failed. Results may be incorrect.\n";
    } else {
        std::cout << "[+] GNames verified. Name[0] = \"" << testName << "\"\n";
    }

    // ============================================================
    // Step 3: Resolve GObjects
    // ============================================================
    std::cout << "\n[*] Step 3: Resolving GObjects...\n";
    if (!Objects::Initialize()) {
        std::cerr << "\n[!] ERROR: Failed to find GObjects.\n";
        Memory::Detach();
        std::cin.get();
        return 1;
    }

    // ============================================================
    // Step 4: Find core engine classes
    // ============================================================
    std::cout << "\n[*] Step 4: Finding core engine classes...\n";
    if (!Objects::FindCoreClasses()) {
        std::cerr << "\n[!] ERROR: Failed to find core classes.\n";
        std::cerr << "    The dump may be incomplete.\n";
    }

    // ============================================================
    // Step 5: Dump offsets
    // ============================================================
    if (!sdkOnly) {
        std::cout << "\n[*] Step 5: Dumping offsets...\n";
        std::string offsetsFile = outputDir + "/offsets.h";
        Offsets::DumpOffsets(offsetsFile);
    }

    // ============================================================
    // Step 6: Generate full SDK
    // ============================================================
    if (!offsetsOnly) {
        std::cout << "\n[*] Step 6: Generating full SDK dump...\n";
        std::cout << "    This may take a few minutes depending on object count...\n\n";
        Generator::GenerateSDK(outputDir);
    }

    // ============================================================
    // Done
    // ============================================================
    auto endTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::seconds>(endTime - startTime);

    std::cout << "\n============================================================\n";
    std::cout << "[+] DUMP COMPLETE!\n";
    std::cout << "    Time elapsed: " << duration.count() << " seconds\n";
    std::cout << "    Output directory: " << outputDir << "/\n";
    std::cout << "============================================================\n";

    Memory::Detach();

    std::cout << "\nPress Enter to exit...";
    std::cin.get();

    return 0;
}
