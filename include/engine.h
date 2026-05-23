#pragma once
#include <cstdint>
#include <string>
#include <vector>

// Unreal Engine structure definitions for Fortnite (UE5)
// These offsets correspond to recent Fortnite builds

namespace Engine {

    // Forward declarations
    struct FName;
    struct UObject;
    struct UField;
    struct UStruct;
    struct UClass;
    struct UEnum;
    struct UFunction;
    struct FProperty;

    // ============================================================
    // FName - Engine name system
    // ============================================================
    struct FNameEntryId {
        uint32_t Value;
    };

    struct FName {
        int32_t ComparisonIndex;
        int32_t Number;

        std::string ToString() const;
    };

    // ============================================================
    // TUObjectArray - Global objects array (chunked)
    // ============================================================
    struct FUObjectItem {
        uintptr_t Object;   // UObject*
        int32_t Flags;
        int32_t ClusterRootIndex;
        int32_t SerialNumber;
    };

    // ============================================================
    // UObject - Base object
    // ============================================================
    struct UObject {
        static constexpr int VTableOffset = 0x0;
        static constexpr int ObjectFlagsOffset = 0x8;
        static constexpr int InternalIndexOffset = 0xC;
        static constexpr int ClassOffset = 0x10;
        static constexpr int NameOffset = 0x18;
        static constexpr int OuterOffset = 0x20;

        uintptr_t address;

        int32_t GetInternalIndex() const;
        uintptr_t GetClass() const;
        FName GetFName() const;
        uintptr_t GetOuter() const;
        std::string GetName() const;
        std::string GetFullName() const;
        std::string GetCppName() const;
        bool IsA(uintptr_t classAddress) const;
    };

    // ============================================================
    // UField
    // ============================================================
    struct UField : UObject {
        static constexpr int NextOffset = 0x28;

        uintptr_t GetNext() const;
    };

    // ============================================================
    // UStruct
    // ============================================================
    struct UStruct : UField {
        static constexpr int SuperStructOffset = 0x40;
        static constexpr int ChildrenOffset = 0x48;
        static constexpr int ChildPropertiesOffset = 0x50;
        static constexpr int PropertiesSizeOffset = 0x58;

        uintptr_t GetSuperStruct() const;
        uintptr_t GetChildren() const;
        uintptr_t GetChildProperties() const;
        int32_t GetPropertiesSize() const;
    };

    // ============================================================
    // UClass
    // ============================================================
    struct UClass : UStruct {
        static constexpr int CastFlagsOffset = 0xE0;
        static constexpr int DefaultObjectOffset = 0x110;

        uint64_t GetCastFlags() const;
        uintptr_t GetDefaultObject() const;
    };

    // ============================================================
    // UEnum
    // ============================================================
    struct UEnum : UField {
        static constexpr int NamesOffset = 0x40;

        struct EnumEntry {
            FName Name;
            int64_t Value;
        };

        std::vector<std::pair<std::string, int64_t>> GetNames() const;
    };

    // ============================================================
    // UFunction
    // ============================================================
    struct UFunction : UStruct {
        static constexpr int FunctionFlagsOffset = 0xB0;
        static constexpr int NativeFuncOffset = 0xB8;

        uint32_t GetFunctionFlags() const;
        uintptr_t GetNativeFunc() const;
    };

    // ============================================================
    // FProperty (UE5 uses FField-based property system)
    // ============================================================
    struct FField {
        static constexpr int NameOffset = 0x28;
        static constexpr int NextOffset = 0x20;

        uintptr_t address;

        FName GetFName() const;
        std::string GetName() const;
        uintptr_t GetNext() const;
    };

    struct FProperty : FField {
        static constexpr int ArrayDimOffset = 0x38;
        static constexpr int ElementSizeOffset = 0x3C;
        static constexpr int PropertyFlagsOffset = 0x40;
        static constexpr int OffsetInternalOffset = 0x4C;

        int32_t GetArrayDim() const;
        int32_t GetElementSize() const;
        uint64_t GetPropertyFlags() const;
        int32_t GetOffset() const;
        std::string GetType() const;
    };

    // ============================================================
    // Property flag enums
    // ============================================================
    enum EPropertyFlags : uint64_t {
        CPF_Edit = 0x0000000000000001,
        CPF_ConstParm = 0x0000000000000002,
        CPF_BlueprintVisible = 0x0000000000000004,
        CPF_ExportObject = 0x0000000000000008,
        CPF_BlueprintReadOnly = 0x0000000000000010,
        CPF_Net = 0x0000000000000020,
        CPF_EditFixedSize = 0x0000000000000040,
        CPF_Parm = 0x0000000000000080,
        CPF_OutParm = 0x0000000000000100,
        CPF_ZeroConstructor = 0x0000000000000200,
        CPF_ReturnParm = 0x0000000000000400,
        CPF_DisableEditOnTemplate = 0x0000000000000800,
        CPF_Transient = 0x0000000000002000,
        CPF_Config = 0x0000000000004000,
        CPF_RepNotify = 0x0000000100000000,
    };

    enum EFunctionFlags : uint32_t {
        FUNC_Final = 0x00000001,
        FUNC_RequiredAPI = 0x00000002,
        FUNC_BlueprintAuthorityOnly = 0x00000004,
        FUNC_BlueprintCosmetic = 0x00000008,
        FUNC_Net = 0x00000040,
        FUNC_NetReliable = 0x00000080,
        FUNC_NetRequest = 0x00000100,
        FUNC_Exec = 0x00000200,
        FUNC_Native = 0x00000400,
        FUNC_Event = 0x00000800,
        FUNC_NetResponse = 0x00001000,
        FUNC_Static = 0x00002000,
        FUNC_NetMulticast = 0x00004000,
        FUNC_BlueprintCallable = 0x04000000,
        FUNC_BlueprintEvent = 0x08000000,
        FUNC_BlueprintPure = 0x10000000,
    };

} // namespace Engine
