#pragma once
#include <cstdint>
#include <string>
#include <vector>

namespace Offsets {

    struct OffsetEntry {
        std::string Name;
        std::string ClassName;
        int32_t Offset;
    };

    // Initialize and dump offsets
    bool DumpOffsets(const std::string& outputFile = "offsets.h");

    // Find offset of a property in a class by name
    int32_t FindOffset(const std::string& className, const std::string& propertyName);

    // Get all discovered offsets
    const std::vector<OffsetEntry>& GetAllOffsets();

    // Dump formatted offset header in flat namespace style
    void WriteOffsetsHeader(const std::string& filename);

} // namespace Offsets
