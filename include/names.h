#pragma once
#include <cstdint>
#include <string>

namespace Names {

    // GNames pointer
    extern uintptr_t GNames;

    // Initialize GNames
    bool Initialize();

    // Get name from FNameEntryId
    std::string GetNameFromId(int32_t id);

    // FNamePool constants (UE5 chunked name pool)
    constexpr int FNameStride = 2;      // Entry alignment
    constexpr int FNameBlockBits = 16;
    constexpr int FNameBlockOffsetBits = 16;
    constexpr int FNameMaxBlocks = 8192;
    constexpr int FNameBlockSize = 65536;

} // namespace Names
