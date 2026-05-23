#include "names.h"
#include "memory.h"
#include <iostream>
#include <unordered_map>

namespace Names {

    uintptr_t GNames = 0;

    // Cache for resolved names
    static std::unordered_map<int32_t, std::string> nameCache;

    bool Initialize() {
        // Pattern for GNames (FNamePool) in Fortnite UE5
        // This pattern finds the reference to the global FNamePool
        // lea rcx, [rip+xxxxxxxx] ; FNamePool
        // Signature: 48 8D 0D ?? ?? ?? ?? E8 ?? ?? ?? ?? C6 05 ?? ?? ?? ?? 01
        const char* sig = "\x48\x8D\x0D\x00\x00\x00\x00\xE8\x00\x00\x00\x00\xC6\x05\x00\x00\x00\x00\x01";
        const char* mask = "xxx????x????xx????x";

        uintptr_t result = Memory::PatternScan(sig, mask);
        if (result) {
            GNames = Memory::ResolveRelative(result, 3, 7);
            std::cout << "[Names] GNames found at: 0x" << std::hex << GNames << std::dec << "\n";
            return true;
        }

        // Fallback pattern
        // 48 8D 05 ?? ?? ?? ?? EB 16
        const char* sig2 = "\x48\x8D\x05\x00\x00\x00\x00\xEB\x16";
        const char* mask2 = "xxx????xx";

        result = Memory::PatternScan(sig2, mask2);
        if (result) {
            GNames = Memory::ResolveRelative(result, 3, 7);
            std::cout << "[Names] GNames found (fallback) at: 0x" << std::hex << GNames << std::dec << "\n";
            return true;
        }

        std::cerr << "[Names] Failed to find GNames pattern\n";
        return false;
    }

    std::string GetNameFromId(int32_t id) {
        if (id < 0) return "";

        // Check cache first
        auto it = nameCache.find(id);
        if (it != nameCache.end()) {
            return it->second;
        }

        if (GNames == 0) return "";

        // UE5 FNamePool structure:
        // FNamePool contains an array of blocks (FNameEntry**)
        // Each block contains FNameEntry entries
        //
        // Layout:
        //   +0x0: Lock (FRWLock)
        //   +0x8: CurrentBlock
        //   +0xC: CurrentByteCursor  
        //   +0x10: Blocks[FNameMaxBlocks] (array of pointers)

        int32_t blockIndex = id >> FNameBlockOffsetBits;
        int32_t blockOffset = id & (FNameBlockSize - 1);

        // Read block pointer from Blocks array
        // Blocks start at offset 0x10 in FNamePool
        uintptr_t blockPtr = Memory::Read<uintptr_t>(GNames + 0x10 + blockIndex * sizeof(uintptr_t));
        if (blockPtr == 0) {
            return "";
        }

        // Each FNameEntry in the block:
        // The offset within the block is blockOffset * FNameStride
        uintptr_t entryAddr = blockPtr + static_cast<uintptr_t>(blockOffset) * FNameStride;

        // FNameEntry layout:
        //   +0x0: FNameEntryHeader (uint16_t)
        //         Bits [0..9]: Length
        //         Bit [10]: bIsWide
        //   +0x2: char Data[] (ANSI) or wchar_t Data[] (wide)

        uint16_t header = Memory::Read<uint16_t>(entryAddr);
        int32_t nameLen = header >> 6;  // Upper bits store length
        bool bIsWide = (header & 1) != 0;

        if (nameLen <= 0 || nameLen > 256) {
            return "";
        }

        std::string name;
        if (!bIsWide) {
            name = Memory::ReadString(entryAddr + 2, nameLen);
        } else {
            // Wide string - convert to narrow
            std::wstring wname = Memory::ReadWString(entryAddr + 2, nameLen);
            name.assign(wname.begin(), wname.end());
        }

        // Cache the result
        nameCache[id] = name;
        return name;
    }

} // namespace Names
