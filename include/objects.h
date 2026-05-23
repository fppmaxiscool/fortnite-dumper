#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include <functional>

namespace Objects {

    // GObjects pointer
    extern uintptr_t GObjects;
    extern int32_t ObjectCount;
    extern int32_t MaxObjects;

    // Chunked TUObjectArray
    constexpr int NumElementsPerChunk = 65536;

    // Initialize GObjects
    bool Initialize();

    // Get object address by index
    uintptr_t GetObjectByIndex(int32_t index);

    // Iterate all objects
    void ForEachObject(const std::function<void(uintptr_t, int32_t)>& callback);

    // Find object by name
    uintptr_t FindObject(const std::string& fullName);

    // Find class addresses for common types
    struct CoreClasses {
        uintptr_t UClass;
        uintptr_t UStruct;
        uintptr_t UEnum;
        uintptr_t UFunction;
        uintptr_t UBlueprintGeneratedClass;
        uintptr_t UScriptStruct;
    };

    extern CoreClasses Classes;
    bool FindCoreClasses();

} // namespace Objects
