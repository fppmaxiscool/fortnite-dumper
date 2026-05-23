#pragma once
#include <Windows.h>
#include <TlHelp32.h>
#include <psapi.h>
#include <cstdint>
#include <string>
#include <vector>

namespace Memory {

    // Process handle and base address
    extern HANDLE hProcess;
    extern uintptr_t baseAddress;
    extern size_t moduleSize;

    // Initialize - attach to Fortnite process
    bool Attach(const wchar_t* processName = L"FortniteClient-Win64-Shipping.exe");
    void Detach();

    // Read process memory
    template<typename T>
    T Read(uintptr_t address) {
        T value{};
        ReadProcessMemory(hProcess, reinterpret_cast<LPCVOID>(address), &value, sizeof(T), nullptr);
        return value;
    }

    // Read buffer
    bool ReadBuffer(uintptr_t address, void* buffer, size_t size);

    // Read string
    std::string ReadString(uintptr_t address, size_t maxLen = 256);
    std::wstring ReadWString(uintptr_t address, size_t maxLen = 256);

    // Pattern scanning
    uintptr_t PatternScan(const char* signature, const char* mask);
    uintptr_t PatternScanModule(const char* signature, const char* mask);

    // Resolve relative address (for RIP-relative instructions)
    uintptr_t ResolveRelative(uintptr_t address, int offset = 3, int instructionSize = 7);

} // namespace Memory
