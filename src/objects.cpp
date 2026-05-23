#define NOMINMAX
#include "objects.h"
#include "memory.h"
#include "engine.h"
#include "names.h"
#include <iostream>

namespace Objects {

    uintptr_t GObjects = 0;
    int32_t ObjectCount = 0;
    int32_t MaxObjects = 0;

    CoreClasses Classes = {};

    bool Initialize() {
        // Pattern for GUObjectArray in Fortnite UE5
        // lea rdx, [rip+xxxxxxxx] ; GUObjectArray
        // Signature: 48 8B 05 ?? ?? ?? ?? 48 8B 0C C8 48 8D 04 D1
        const char* sig = "\x48\x8B\x05\x00\x00\x00\x00\x48\x8B\x0C\xC8\x48\x8D\x04\xD1";
        const char* mask = "xxx????xxxxxxxx";

        uintptr_t result = Memory::PatternScan(sig, mask);
        if (result) {
            GObjects = Memory::ResolveRelative(result, 3, 7);
            std::cout << "[Objects] GObjects found at: 0x" << std::hex << GObjects << std::dec << "\n";
        } else {
            // Fallback pattern
            // 48 8D 0D ?? ?? ?? ?? E8 ?? ?? ?? ?? 48 85 C0 74 ?? 48 8B 00
            const char* sig2 = "\x48\x8D\x0D\x00\x00\x00\x00\xE8\x00\x00\x00\x00\x48\x85\xC0\x74";
            const char* mask2 = "xxx????x????xxxx";

            result = Memory::PatternScan(sig2, mask2);
            if (result) {
                GObjects = Memory::ResolveRelative(result, 3, 7);
                std::cout << "[Objects] GObjects found (fallback) at: 0x" << std::hex << GObjects << std::dec << "\n";
            } else {
                std::cerr << "[Objects] Failed to find GObjects pattern\n";
                return false;
            }
        }

        // FUObjectArray layout (chunked):
        //   +0x00: ObjObjects (FChunkedFixedUObjectArray)
        //       +0x00: Objects** (pointer to array of chunk pointers)
        //       +0x08: PreAllocatedObjects (unused)
        //       +0x10: MaxElements
        //       +0x14: NumElements
        //       +0x18: MaxChunks

        // Read object count
        // The NumElements is at ObjObjects + 0x14
        ObjectCount = Memory::Read<int32_t>(GObjects + 0x14);
        MaxObjects = Memory::Read<int32_t>(GObjects + 0x10);

        std::cout << "[Objects] Object count: " << ObjectCount << "\n";
        std::cout << "[Objects] Max objects: " << MaxObjects << "\n";

        if (ObjectCount <= 0 || ObjectCount > 10000000) {
            std::cerr << "[Objects] Invalid object count\n";
            return false;
        }

        return true;
    }

    uintptr_t GetObjectByIndex(int32_t index) {
        if (index < 0 || index >= ObjectCount) return 0;

        // Chunked array: each chunk holds NumElementsPerChunk items
        int32_t chunkIndex = index / NumElementsPerChunk;
        int32_t withinChunkIndex = index % NumElementsPerChunk;

        // Read chunk pointer from the Objects array
        uintptr_t objectsPtr = Memory::Read<uintptr_t>(GObjects + 0x0);
        if (objectsPtr == 0) return 0;

        uintptr_t chunkPtr = Memory::Read<uintptr_t>(objectsPtr + chunkIndex * sizeof(uintptr_t));
        if (chunkPtr == 0) return 0;

        // Each FUObjectItem is 24 bytes (pointer + flags + clusterIndex + serial)
        constexpr size_t FUObjectItemSize = 24;
        uintptr_t itemAddr = chunkPtr + withinChunkIndex * FUObjectItemSize;

        // First field of FUObjectItem is the UObject pointer
        return Memory::Read<uintptr_t>(itemAddr);
    }

    void ForEachObject(const std::function<void(uintptr_t, int32_t)>& callback) {
        uintptr_t objectsPtr = Memory::Read<uintptr_t>(GObjects + 0x0);
        if (objectsPtr == 0) return;

        int32_t numChunks = (ObjectCount + NumElementsPerChunk - 1) / NumElementsPerChunk;
        constexpr size_t FUObjectItemSize = 24;

        for (int32_t chunkIdx = 0; chunkIdx < numChunks; chunkIdx++) {
            uintptr_t chunkPtr = Memory::Read<uintptr_t>(objectsPtr + chunkIdx * sizeof(uintptr_t));
            if (chunkPtr == 0) continue;

            int32_t chunkStart = chunkIdx * NumElementsPerChunk;
            int32_t chunkEnd = std::min(chunkStart + NumElementsPerChunk, ObjectCount);

            // Read chunk in bulk for performance
            int32_t itemsInChunk = chunkEnd - chunkStart;
            std::vector<uint8_t> chunkData(itemsInChunk * FUObjectItemSize);

            if (!Memory::ReadBuffer(chunkPtr, chunkData.data(), chunkData.size())) {
                // Fall back to individual reads
                for (int32_t i = chunkStart; i < chunkEnd; i++) {
                    uintptr_t objPtr = Memory::Read<uintptr_t>(chunkPtr + (i - chunkStart) * FUObjectItemSize);
                    if (objPtr != 0) {
                        callback(objPtr, i);
                    }
                }
                continue;
            }

            for (int32_t i = 0; i < itemsInChunk; i++) {
                uintptr_t objPtr = *reinterpret_cast<uintptr_t*>(chunkData.data() + i * FUObjectItemSize);
                if (objPtr != 0) {
                    callback(objPtr, chunkStart + i);
                }
            }
        }
    }

    uintptr_t FindObject(const std::string& fullName) {
        uintptr_t found = 0;

        ForEachObject([&](uintptr_t address, int32_t index) {
            if (found != 0) return; // Already found

            Engine::UObject obj{};
            obj.address = address;
            if (obj.GetFullName() == fullName) {
                found = address;
            }
        });

        return found;
    }

    bool FindCoreClasses() {
        std::cout << "[Objects] Finding core engine classes...\n";

        ForEachObject([&](uintptr_t address, int32_t index) {
            Engine::UObject obj{};
            obj.address = address;

            // Check if this object's class is itself (metaclass pattern)
            uintptr_t classAddr = obj.GetClass();
            std::string name = obj.GetName();

            if (name == "Class" && classAddr == address) {
                Classes.UClass = address;
            } else if (name == "ScriptStruct" && Classes.UScriptStruct == 0) {
                // ScriptStruct's class should be Class
                if (classAddr == Classes.UClass || Classes.UClass == 0) {
                    Classes.UScriptStruct = address;
                }
            }

            // These are found by their full names
            std::string fullName = obj.GetFullName();
            if (fullName == "Class /Script/CoreUObject.Struct") {
                Classes.UStruct = address;
            } else if (fullName == "Class /Script/CoreUObject.Enum") {
                Classes.UEnum = address;
            } else if (fullName == "Class /Script/CoreUObject.Function") {
                Classes.UFunction = address;
            } else if (fullName == "Class /Script/Engine.BlueprintGeneratedClass") {
                Classes.UBlueprintGeneratedClass = address;
            }
        });

        bool success = true;
        if (Classes.UClass == 0) { std::cerr << "  [!] UClass not found\n"; success = false; }
        else std::cout << "  UClass: 0x" << std::hex << Classes.UClass << "\n";

        if (Classes.UStruct == 0) { std::cerr << "  [!] UStruct not found\n"; }
        else std::cout << "  UStruct: 0x" << std::hex << Classes.UStruct << "\n";

        if (Classes.UEnum == 0) { std::cerr << "  [!] UEnum not found\n"; }
        else std::cout << "  UEnum: 0x" << std::hex << Classes.UEnum << "\n";

        if (Classes.UFunction == 0) { std::cerr << "  [!] UFunction not found\n"; }
        else std::cout << "  UFunction: 0x" << std::hex << Classes.UFunction << "\n";

        std::cout << std::dec;
        return success;
    }

} // namespace Objects
