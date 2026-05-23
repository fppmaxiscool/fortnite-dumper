#pragma once
#include <string>
#include <vector>
#include <fstream>
#include <unordered_map>

namespace Generator {

    struct PropertyInfo {
        std::string Name;
        std::string Type;
        int32_t Offset;
        int32_t Size;
        int32_t ArrayDim;
        uint64_t Flags;
    };

    struct FunctionInfo {
        std::string Name;
        uint32_t Flags;
        uintptr_t NativeFunc;
        std::vector<PropertyInfo> Parameters;
        std::string ReturnType;
    };

    struct ClassInfo {
        std::string Name;
        std::string FullName;
        std::string SuperName;
        int32_t Size;
        std::vector<PropertyInfo> Properties;
        std::vector<FunctionInfo> Functions;
    };

    struct EnumInfo {
        std::string Name;
        std::string FullName;
        std::vector<std::pair<std::string, int64_t>> Members;
    };

    // Generate SDK dump
    bool GenerateSDK(const std::string& outputDir = "SDK_Dump");

    // Dump a single class/struct
    ClassInfo DumpClass(uintptr_t classAddress);

    // Dump a single enum
    EnumInfo DumpEnum(uintptr_t enumAddress);

    // Generate header file for a package
    void GeneratePackageHeader(std::ofstream& file, const std::string& packageName,
                               const std::vector<EnumInfo>& enums,
                               const std::vector<ClassInfo>& classes);

} // namespace Generator
