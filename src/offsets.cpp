#include "offsets.h"
#include "engine.h"
#include "memory.h"
#include "objects.h"
#include "names.h"
#include <iostream>
#include <fstream>
#include <iomanip>
#include <map>

namespace Offsets {

    static std::vector<OffsetEntry> allOffsets;

    // ============================================================
    // Find offset of a property within a class hierarchy
    // ============================================================
    int32_t FindOffset(const std::string& className, const std::string& propertyName) {
        // Find the class object
        uintptr_t classAddr = 0;

        Objects::ForEachObject([&](uintptr_t address, int32_t index) {
            if (classAddr != 0) return;

            Engine::UObject obj{};
            obj.address = address;

            uintptr_t objClass = obj.GetClass();
            if (objClass != Objects::Classes.UClass && objClass != Objects::Classes.UScriptStruct)
                return;

            if (obj.GetName() == className) {
                classAddr = address;
            }
        });

        if (classAddr == 0) return -1;

        // Walk the class hierarchy to find the property
        uintptr_t currentClass = classAddr;
        while (currentClass) {
            Engine::UStruct structObj{};
            structObj.address = currentClass;

            // Iterate FProperty chain
            uintptr_t propAddr = structObj.GetChildProperties();
            while (propAddr) {
                Engine::FProperty prop{};
                prop.address = propAddr;

                if (prop.GetName() == propertyName) {
                    return prop.GetOffset();
                }

                propAddr = prop.GetNext();
            }

            currentClass = structObj.GetSuperStruct();
        }

        return -1;
    }

    const std::vector<OffsetEntry>& GetAllOffsets() {
        return allOffsets;
    }

    // ============================================================
    // Dump all Fortnite offsets in the user's desired format
    // ============================================================
    bool DumpOffsets(const std::string& outputFile) {
        std::cout << "\n[Offsets] Dumping game offsets...\n";
        allOffsets.clear();

        // Define all offsets to search for - matches the user's desired output format
        struct OffsetSearch {
            std::string className;
            std::string propertyName;
            std::string outputName; // Name as it appears in the output header
        };

        std::vector<OffsetSearch> searches = {
            // UWorld / Game globals
            {"World", "PersistentLevel", "UWorld"},  // GWorld offset found via pattern
            {"GameInstance", "OwningGameInstance", "OwningGameInstance"},  // Will be searched
            {"World", "GameState", "GameState"},
            {"GameStateBase", "PlayerArray", "PlayerArray"},
            {"GameStateBase", "ServerWorldTimeSecondsDelta", "ServerWorldTimeSecondsDelta"},

            // Targeting / Habanero
            {"FortPawn", "TargetedFortPawn", "TargetedFortPawn"},
            {"FortPawn", "HabaneroComponent", "HabaneroComponent"},

            // Actor
            {"Actor", "RootComponent", "RootComponent"},

            // Level
            {"World", "Levels", "Levels"},
            // AActor array in Level is at Level.Actors
            {"Level", "Actors", "AActor"},

            // SceneComponent
            {"SceneComponent", "RelativeLocation", "RelativeLocation"},

            // PlayerState names
            {"PlayerState", "PlayerNamePrivate", "PlayerName"},
            {"FortPlayerState", "Platform", "Platform"},

            // Weapons
            {"FortPawn", "CurrentWeapon", "CurrentWeapon"},
            {"FortWeapon", "WeaponData", "WeaponData"},
            {"FortItemDefinition", "DisplayName", "ItemName"},
            {"FortItemDefinition", "Tier", "Tier"},

            // Local player chain
            {"GameInstance", "LocalPlayers", "LocalPlayers"},
            {"LocalPlayer", "PlayerController", "PlayerController"},
            {"PlayerController", "AcknowledgedPawn", "AcknowledgedPawn"},
            {"FortPlayerStateAthena", "TeamIndex", "TeamID"},

            // PlayerState / Pawn
            {"PlayerState", "PawnPrivate", "PawnPrivate"},
            {"Pawn", "PlayerState", "PlayerState"},
            {"FortPlayerState", "TeamIndex", "TeamIndex"},

            // Mesh / Bones
            {"Character", "Mesh", "Mesh"},
            {"SkinnedMeshComponent", "BoneArray", "BoneArray"},
            {"SkinnedMeshComponent", "CachedComponentSpaceTransforms", "BoneCache"},
            {"SceneComponent", "ComponentToWorld", "ComponentToWorld"},
            {"FortPawn", "bIsDying", "bIsDying"},

            // Rendering
            {"PrimitiveComponent", "bRecentlyRendered", "bRecentlyRendered"},
            {"PrimitiveComponent", "LastRenderTime", "LastRenderTime"},

            // Camera - these are found via PlayerCameraManager struct
            {"PlayerCameraManager", "CameraCachePrivate", "CameraLocation"},

            // FOV
            {"PlayerController", "FOV", "Fov"},
        };

        int found = 0;
        int total = static_cast<int>(searches.size());

        for (const auto& search : searches) {
            int32_t offset = FindOffset(search.className, search.propertyName);

            OffsetEntry entry;
            entry.ClassName = search.className;
            entry.Name = search.outputName;
            entry.Offset = offset;
            allOffsets.push_back(entry);

            if (offset >= 0) {
                found++;
                std::cout << "  [+] " << search.outputName << " = 0x"
                          << std::hex << offset << std::dec << "\n";
            } else {
                std::cout << "  [-] " << search.outputName << " (" << search.className
                          << "." << search.propertyName << ") = NOT FOUND\n";
            }
        }

        std::cout << "[Offsets] Found " << found << "/" << total << " offsets\n";

        // Write output in the user's desired format
        WriteOffsetsHeader(outputFile);

        return found > 0;
    }

    // ============================================================
    // Write formatted C++ header with offsets in flat namespace style
    // ============================================================
    void WriteOffsetsHeader(const std::string& filename) {
        std::ofstream file(filename);
        if (!file.is_open()) {
            std::cerr << "[Offsets] Failed to write: " << filename << "\n";
            return;
        }

        file << "#pragma once\n";
        file << "#include <cstdint>\n\n";

        file << "namespace offsets\n";
        file << "{\n";
        file << "    constexpr uint64_t\n";

        for (size_t i = 0; i < allOffsets.size(); i++) {
            const auto& entry = allOffsets[i];

            file << "        " << entry.Name << " = ";

            if (entry.Offset >= 0) {
                file << "0x" << std::hex << entry.Offset << std::dec;
            } else {
                file << "0x0 /* NOT FOUND */";
            }

            file << ",\n";
        }

        // Computed offsets inline (CameraRotation, Seconds)
        file << "        CameraRotation = CameraLocation + 0xC,\n";
        file << "        Seconds = CameraRotation + 0xC;\n";

        file << "}\n";
        file.close();

        std::cout << "[Offsets] Written to: " << filename << "\n";
    }

} // namespace Offsets
