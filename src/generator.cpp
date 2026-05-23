#include "generator.h"
#include "engine.h"
#include "memory.h"
#include "objects.h"
#include "names.h"
#include <iostream>
#include <filesystem>
#include <algorithm>
#include <set>
#include <map>

namespace Generator {

    // ============================================================
    // Dump a single class/struct with all properties and functions
    // ============================================================
    ClassInfo DumpClass(uintptr_t classAddress) {
        ClassInfo info{};
        if (classAddress == 0) return info;

        Engine::UStruct structObj{};
        structObj.address = classAddress;

        Engine::UObject obj{};
        obj.address = classAddress;

        info.Name = obj.GetCppName();
        info.FullName = obj.GetFullName();
        info.Size = structObj.GetPropertiesSize();

        // Get super class name
        uintptr_t superAddr = structObj.GetSuperStruct();
        if (superAddr) {
            Engine::UObject superObj{};
            superObj.address = superAddr;
            info.SuperName = superObj.GetCppName();
        }

        // Iterate FProperty chain (UE5 FField-based)
        uintptr_t propAddr = structObj.GetChildProperties();
        while (propAddr) {
            Engine::FProperty prop{};
            prop.address = propAddr;

            PropertyInfo propInfo{};
            propInfo.Name = prop.GetName();
            propInfo.Type = prop.GetType();
            propInfo.Offset = prop.GetOffset();
            propInfo.Size = prop.GetElementSize();
            propInfo.ArrayDim = prop.GetArrayDim();
            propInfo.Flags = prop.GetPropertyFlags();

            if (!propInfo.Name.empty()) {
                info.Properties.push_back(propInfo);
            }

            propAddr = prop.GetNext();
        }

        // Iterate UField children for functions (legacy child chain)
        uintptr_t childAddr = structObj.GetChildren();
        while (childAddr) {
            Engine::UField child{};
            child.address = childAddr;

            Engine::UObject childObj{};
            childObj.address = childAddr;

            // Check if this child is a UFunction
            uintptr_t childClass = childObj.GetClass();
            if (childClass == Objects::Classes.UFunction) {
                Engine::UFunction func{};
                func.address = childAddr;

                FunctionInfo funcInfo{};
                funcInfo.Name = childObj.GetName();
                funcInfo.Flags = func.GetFunctionFlags();
                funcInfo.NativeFunc = func.GetNativeFunc();

                // Get function parameters from its property chain
                Engine::UStruct funcStruct{};
                funcStruct.address = childAddr;

                uintptr_t paramAddr = funcStruct.GetChildProperties();
                while (paramAddr) {
                    Engine::FProperty paramProp{};
                    paramProp.address = paramAddr;

                    PropertyInfo paramInfo{};
                    paramInfo.Name = paramProp.GetName();
                    paramInfo.Type = paramProp.GetType();
                    paramInfo.Offset = paramProp.GetOffset();
                    paramInfo.Size = paramProp.GetElementSize();
                    paramInfo.Flags = paramProp.GetPropertyFlags();

                    if (paramInfo.Flags & Engine::CPF_ReturnParm) {
                        funcInfo.ReturnType = paramInfo.Type;
                    } else if (paramInfo.Flags & Engine::CPF_Parm) {
                        funcInfo.Parameters.push_back(paramInfo);
                    }

                    paramAddr = paramProp.GetNext();
                }

                if (funcInfo.ReturnType.empty()) {
                    funcInfo.ReturnType = "void";
                }

                info.Functions.push_back(funcInfo);
            }

            childAddr = child.GetNext();
        }

        return info;
    }

    // ============================================================
    // Dump enum
    // ============================================================
    EnumInfo DumpEnum(uintptr_t enumAddress) {
        EnumInfo info{};
        if (enumAddress == 0) return info;

        Engine::UObject obj{};
        obj.address = enumAddress;

        info.Name = obj.GetName();
        info.FullName = obj.GetFullName();

        Engine::UEnum enumObj{};
        enumObj.address = enumAddress;
        info.Members = enumObj.GetNames();

        return info;
    }

    // ============================================================
    // Generate formatted header for a package
    // ============================================================
    void GeneratePackageHeader(std::ofstream& file, const std::string& packageName,
                               const std::vector<EnumInfo>& enums,
                               const std::vector<ClassInfo>& classes) {
        file << "#pragma once\n\n";
        file << "// ============================================================\n";
        file << "// SDK Dump - Package: " << packageName << "\n";
        file << "// Generated by Fortnite SDK Dumper\n";
        file << "// ============================================================\n\n";

        // Enums
        if (!enums.empty()) {
            file << "// ------- Enums -------\n\n";
            for (const auto& e : enums) {
                file << "// " << e.FullName << "\n";
                file << "enum class " << e.Name << " : uint8_t {\n";
                for (const auto& [name, value] : e.Members) {
                    // Clean up enum member name (remove prefix)
                    std::string memberName = name;
                    size_t colonPos = memberName.rfind("::");
                    if (colonPos != std::string::npos) {
                        memberName = memberName.substr(colonPos + 2);
                    }
                    file << "    " << memberName << " = " << value << ",\n";
                }
                file << "};\n\n";
            }
        }

        // Classes
        if (!classes.empty()) {
            file << "// ------- Classes / Structs -------\n\n";
            for (const auto& c : classes) {
                file << "// " << c.FullName << "\n";
                file << "// Size: 0x" << std::hex << c.Size << std::dec << "\n";
                if (!c.SuperName.empty()) {
                    file << "class " << c.Name << " : public " << c.SuperName << " {\n";
                } else {
                    file << "class " << c.Name << " {\n";
                }
                file << "public:\n";

                // Properties sorted by offset
                auto sortedProps = c.Properties;
                std::sort(sortedProps.begin(), sortedProps.end(),
                    [](const PropertyInfo& a, const PropertyInfo& b) {
                        return a.Offset < b.Offset;
                    });

                int32_t lastOffset = -1;
                for (const auto& p : sortedProps) {
                    if (p.Offset == lastOffset) continue; // Skip duplicates
                    lastOffset = p.Offset;

                    file << "    " << p.Type;
                    if (p.ArrayDim > 1) {
                        file << " " << p.Name << "[" << p.ArrayDim << "]";
                    } else {
                        file << " " << p.Name;
                    }
                    file << "; // 0x" << std::hex << p.Offset << std::dec;
                    file << " (Size: 0x" << std::hex << p.Size << std::dec << ")";

                    // Property flags
                    if (p.Flags & Engine::CPF_Edit) file << " [Edit]";
                    if (p.Flags & Engine::CPF_BlueprintVisible) file << " [BlueprintVisible]";
                    if (p.Flags & Engine::CPF_Net) file << " [Net]";
                    if (p.Flags & Engine::CPF_RepNotify) file << " [RepNotify]";

                    file << "\n";
                }

                // Functions
                if (!c.Functions.empty()) {
                    file << "\n    // Functions\n";
                    for (const auto& f : c.Functions) {
                        file << "    ";

                        // Function flags as comment
                        if (f.Flags & Engine::FUNC_Static) file << "static ";
                        if (f.Flags & Engine::FUNC_Native) file << "/* Native */ ";

                        file << f.ReturnType << " " << f.Name << "(";

                        for (size_t i = 0; i < f.Parameters.size(); i++) {
                            if (i > 0) file << ", ";
                            const auto& param = f.Parameters[i];
                            if (param.Flags & Engine::CPF_OutParm) file << "/* out */ ";
                            if (param.Flags & Engine::CPF_ConstParm) file << "const ";
                            file << param.Type << " " << param.Name;
                        }
                        file << ");";

                        if (f.NativeFunc) {
                            file << " // Native: 0x" << std::hex << (f.NativeFunc - Memory::baseAddress) << std::dec;
                        }
                        file << "\n";
                    }
                }

                file << "};\n\n";
            }
        }
    }

    // ============================================================
    // Generate full SDK dump
    // ============================================================
    bool GenerateSDK(const std::string& outputDir) {
        std::cout << "\n[Generator] Starting SDK dump...\n";

        // Create output directory
        std::filesystem::create_directories(outputDir);

        // Group objects by package (outer-most object)
        std::map<std::string, std::vector<uintptr_t>> packageClasses;
        std::map<std::string, std::vector<uintptr_t>> packageEnums;

        int classCount = 0;
        int enumCount = 0;

        Objects::ForEachObject([&](uintptr_t address, int32_t index) {
            Engine::UObject obj{};
            obj.address = address;
            uintptr_t classAddr = obj.GetClass();

            // Determine package name from outermost object
            std::string packageName;
            uintptr_t outer = obj.GetOuter();
            uintptr_t lastOuter = 0;
            while (outer) {
                lastOuter = outer;
                Engine::UObject outerObj{};
                outerObj.address = outer;
                outer = outerObj.GetOuter();
            }
            if (lastOuter) {
                Engine::UObject pkgObj{};
                pkgObj.address = lastOuter;
                packageName = pkgObj.GetName();
            } else {
                packageName = "Default";
            }

            // Check if it's a class/struct or enum
            if (classAddr == Objects::Classes.UClass ||
                classAddr == Objects::Classes.UScriptStruct ||
                classAddr == Objects::Classes.UBlueprintGeneratedClass) {
                packageClasses[packageName].push_back(address);
                classCount++;
            } else if (classAddr == Objects::Classes.UEnum) {
                packageEnums[packageName].push_back(address);
                enumCount++;
            }
        });

        std::cout << "[Generator] Found " << classCount << " classes/structs\n";
        std::cout << "[Generator] Found " << enumCount << " enums\n";

        // Generate a header file per package
        std::set<std::string> allPackages;
        for (auto& [pkg, _] : packageClasses) allPackages.insert(pkg);
        for (auto& [pkg, _] : packageEnums) allPackages.insert(pkg);

        int packageNum = 0;
        int totalPackages = static_cast<int>(allPackages.size());

        for (const auto& packageName : allPackages) {
            packageNum++;

            // Sanitize filename
            std::string filename = packageName;
            std::replace(filename.begin(), filename.end(), '/', '_');
            std::replace(filename.begin(), filename.end(), '\\', '_');

            std::string filepath = outputDir + "/" + filename + ".h";
            std::ofstream file(filepath);
            if (!file.is_open()) {
                std::cerr << "[Generator] Failed to create: " << filepath << "\n";
                continue;
            }

            // Dump enums for this package
            std::vector<EnumInfo> enums;
            if (packageEnums.count(packageName)) {
                for (uintptr_t addr : packageEnums[packageName]) {
                    auto info = DumpEnum(addr);
                    if (!info.Name.empty() && !info.Members.empty()) {
                        enums.push_back(info);
                    }
                }
            }

            // Dump classes for this package
            std::vector<ClassInfo> classes;
            if (packageClasses.count(packageName)) {
                for (uintptr_t addr : packageClasses[packageName]) {
                    auto info = DumpClass(addr);
                    if (!info.Name.empty()) {
                        classes.push_back(info);
                    }
                }
            }

            GeneratePackageHeader(file, packageName, enums, classes);
            file.close();

            // Progress
            if (packageNum % 50 == 0 || packageNum == totalPackages) {
                std::cout << "[Generator] Progress: " << packageNum << "/" << totalPackages << " packages\n";
            }
        }

        std::cout << "[Generator] SDK dump complete! Output: " << outputDir << "/\n";
        std::cout << "[Generator] Total packages: " << totalPackages << "\n";

        return true;
    }

} // namespace Generator
