#include "memory.h"
#include <iostream>
#include <algorithm>

namespace Memory {

    HANDLE hProcess = nullptr;
    uintptr_t baseAddress = 0;
    size_t moduleSize = 0;

    bool Attach(const wchar_t* processName) {
        // Find process by name
        HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        if (snapshot == INVALID_HANDLE_VALUE) {
            std::cerr << "[Memory] Failed to create process snapshot\n";
            return false;
        }

        PROCESSENTRY32W entry{};
        entry.dwSize = sizeof(entry);

        DWORD pid = 0;
        if (Process32FirstW(snapshot, &entry)) {
            do {
                if (_wcsicmp(entry.szExeFile, processName) == 0) {
                    pid = entry.th32ProcessID;
                    break;
                }
            } while (Process32NextW(snapshot, &entry));
        }
        CloseHandle(snapshot);

        if (pid == 0) {
            std::cerr << "[Memory] Process not found: ";
            std::wcerr << processName << L"\n";
            return false;
        }

        // Open process with read access
        hProcess = OpenProcess(PROCESS_VM_READ | PROCESS_QUERY_INFORMATION, FALSE, pid);
        if (!hProcess) {
            std::cerr << "[Memory] Failed to open process. Error: " << GetLastError() << "\n";
            return false;
        }

        // Get module base address
        HANDLE moduleSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, pid);
        if (moduleSnapshot == INVALID_HANDLE_VALUE) {
            std::cerr << "[Memory] Failed to create module snapshot\n";
            Detach();
            return false;
        }

        MODULEENTRY32W moduleEntry{};
        moduleEntry.dwSize = sizeof(moduleEntry);

        if (Module32FirstW(moduleSnapshot, &moduleEntry)) {
            do {
                if (_wcsicmp(moduleEntry.szModule, processName) == 0) {
                    baseAddress = reinterpret_cast<uintptr_t>(moduleEntry.modBaseAddr);
                    moduleSize = moduleEntry.modBaseSize;
                    break;
                }
            } while (Module32NextW(moduleSnapshot, &moduleEntry));
        }
        CloseHandle(moduleSnapshot);

        if (baseAddress == 0) {
            std::cerr << "[Memory] Failed to find module base\n";
            Detach();
            return false;
        }

        std::cout << "[Memory] Attached to process (PID: " << pid << ")\n";
        std::cout << "[Memory] Base: 0x" << std::hex << baseAddress << std::dec << "\n";
        std::cout << "[Memory] Size: " << (moduleSize / 1024 / 1024) << " MB\n";

        return true;
    }

    void Detach() {
        if (hProcess) {
            CloseHandle(hProcess);
            hProcess = nullptr;
        }
        baseAddress = 0;
        moduleSize = 0;
    }

    bool ReadBuffer(uintptr_t address, void* buffer, size_t size) {
        SIZE_T bytesRead = 0;
        return ReadProcessMemory(hProcess, reinterpret_cast<LPCVOID>(address),
                                 buffer, size, &bytesRead) && bytesRead == size;
    }

    std::string ReadString(uintptr_t address, size_t maxLen) {
        std::string result;
        result.resize(maxLen);
        SIZE_T bytesRead = 0;
        if (ReadProcessMemory(hProcess, reinterpret_cast<LPCVOID>(address),
                              result.data(), maxLen, &bytesRead)) {
            // Find null terminator
            size_t len = 0;
            for (size_t i = 0; i < bytesRead; i++) {
                if (result[i] == '\0') break;
                len++;
            }
            result.resize(len);
            return result;
        }
        return "";
    }

    std::wstring ReadWString(uintptr_t address, size_t maxLen) {
        std::wstring result;
        result.resize(maxLen);
        SIZE_T bytesRead = 0;
        if (ReadProcessMemory(hProcess, reinterpret_cast<LPCVOID>(address),
                              result.data(), maxLen * sizeof(wchar_t), &bytesRead)) {
            size_t len = 0;
            for (size_t i = 0; i < bytesRead / sizeof(wchar_t); i++) {
                if (result[i] == L'\0') break;
                len++;
            }
            result.resize(len);
            return result;
        }
        return L"";
    }

    uintptr_t PatternScan(const char* signature, const char* mask) {
        size_t patternLen = strlen(mask);

        // Read module memory in chunks
        constexpr size_t CHUNK_SIZE = 0x100000; // 1MB chunks
        std::vector<uint8_t> buffer(CHUNK_SIZE + patternLen);

        for (size_t offset = 0; offset < moduleSize; offset += CHUNK_SIZE) {
            size_t readSize = std::min(CHUNK_SIZE + patternLen, moduleSize - offset);
            SIZE_T bytesRead = 0;

            if (!ReadProcessMemory(hProcess,
                                   reinterpret_cast<LPCVOID>(baseAddress + offset),
                                   buffer.data(), readSize, &bytesRead)) {
                continue;
            }

            // Scan chunk for pattern
            for (size_t i = 0; i < bytesRead - patternLen; i++) {
                bool found = true;
                for (size_t j = 0; j < patternLen; j++) {
                    if (mask[j] == '?') continue;
                    if (buffer[i + j] != static_cast<uint8_t>(signature[j])) {
                        found = false;
                        break;
                    }
                }
                if (found) {
                    return baseAddress + offset + i;
                }
            }
        }

        return 0;
    }

    uintptr_t PatternScanModule(const char* signature, const char* mask) {
        return PatternScan(signature, mask);
    }

    uintptr_t ResolveRelative(uintptr_t address, int offset, int instructionSize) {
        int32_t relativeOffset = Read<int32_t>(address + offset);
        return address + instructionSize + relativeOffset;
    }

} // namespace Memory
