#include "engine.h"
#include "memory.h"
#include "names.h"
#include <sstream>

namespace Engine {

    // ============================================================
    // FName
    // ============================================================
    std::string FName::ToString() const {
        std::string name = Names::GetNameFromId(ComparisonIndex);
        if (Number > 0) {
            name += "_" + std::to_string(Number - 1);
        }
        return name;
    }

    // ============================================================
    // UObject
    // ============================================================
    int32_t UObject::GetInternalIndex() const {
        return Memory::Read<int32_t>(address + InternalIndexOffset);
    }

    uintptr_t UObject::GetClass() const {
        return Memory::Read<uintptr_t>(address + ClassOffset);
    }

    FName UObject::GetFName() const {
        FName name{};
        name.ComparisonIndex = Memory::Read<int32_t>(address + NameOffset);
        name.Number = Memory::Read<int32_t>(address + NameOffset + 4);
        return name;
    }

    uintptr_t UObject::GetOuter() const {
        return Memory::Read<uintptr_t>(address + OuterOffset);
    }

    std::string UObject::GetName() const {
        return GetFName().ToString();
    }

    std::string UObject::GetFullName() const {
        std::string name = GetName();

        uintptr_t outer = GetOuter();
        while (outer) {
            UObject outerObj{};
            outerObj.address = outer;
            name = outerObj.GetName() + "." + name;
            outer = outerObj.GetOuter();
        }

        // Prepend class name
        UObject classObj{};
        classObj.address = GetClass();
        std::string className = classObj.GetName();

        return className + " " + name;
    }

    std::string UObject::GetCppName() const {
        std::string name = GetName();

        // Check if this is a class or struct
        UObject classObj{};
        classObj.address = GetClass();
        std::string className = classObj.GetName();

        if (className == "Class" || className == "BlueprintGeneratedClass") {
            return "U" + name;
        } else if (className == "ScriptStruct") {
            return "F" + name;
        }

        return name;
    }

    bool UObject::IsA(uintptr_t classAddress) const {
        uintptr_t currentClass = GetClass();
        while (currentClass) {
            if (currentClass == classAddress) return true;
            // Walk super chain
            currentClass = Memory::Read<uintptr_t>(currentClass + UStruct::SuperStructOffset);
        }
        return false;
    }

    // ============================================================
    // UField
    // ============================================================
    uintptr_t UField::GetNext() const {
        return Memory::Read<uintptr_t>(address + NextOffset);
    }

    // ============================================================
    // UStruct
    // ============================================================
    uintptr_t UStruct::GetSuperStruct() const {
        return Memory::Read<uintptr_t>(address + SuperStructOffset);
    }

    uintptr_t UStruct::GetChildren() const {
        return Memory::Read<uintptr_t>(address + ChildrenOffset);
    }

    uintptr_t UStruct::GetChildProperties() const {
        return Memory::Read<uintptr_t>(address + ChildPropertiesOffset);
    }

    int32_t UStruct::GetPropertiesSize() const {
        return Memory::Read<int32_t>(address + PropertiesSizeOffset);
    }

    // ============================================================
    // UClass
    // ============================================================
    uint64_t UClass::GetCastFlags() const {
        return Memory::Read<uint64_t>(address + CastFlagsOffset);
    }

    uintptr_t UClass::GetDefaultObject() const {
        return Memory::Read<uintptr_t>(address + DefaultObjectOffset);
    }

    // ============================================================
    // UEnum
    // ============================================================
    std::vector<std::pair<std::string, int64_t>> UEnum::GetNames() const {
        std::vector<std::pair<std::string, int64_t>> result;

        // TArray<TPair<FName, int64>> layout:
        // uintptr_t Data, int32 Count, int32 Max
        uintptr_t dataPtr = Memory::Read<uintptr_t>(address + NamesOffset);
        int32_t count = Memory::Read<int32_t>(address + NamesOffset + 8);

        if (count <= 0 || count > 1024 || dataPtr == 0) return result;

        // Each entry is FName (8 bytes) + padding (8 bytes for alignment) + int64 (8 bytes) 
        // Actually in UE5: TPair<FName, int64> = 16 bytes total
        constexpr size_t entrySize = 16;

        for (int32_t i = 0; i < count; i++) {
            uintptr_t entryAddr = dataPtr + (i * entrySize);

            FName name{};
            name.ComparisonIndex = Memory::Read<int32_t>(entryAddr);
            name.Number = Memory::Read<int32_t>(entryAddr + 4);
            int64_t value = Memory::Read<int64_t>(entryAddr + 8);

            std::string nameStr = name.ToString();
            if (!nameStr.empty()) {
                result.push_back({ nameStr, value });
            }
        }

        return result;
    }

    // ============================================================
    // UFunction
    // ============================================================
    uint32_t UFunction::GetFunctionFlags() const {
        return Memory::Read<uint32_t>(address + FunctionFlagsOffset);
    }

    uintptr_t UFunction::GetNativeFunc() const {
        return Memory::Read<uintptr_t>(address + NativeFuncOffset);
    }

    // ============================================================
    // FField (UE5 property system)
    // ============================================================
    FName FField::GetFName() const {
        FName name{};
        name.ComparisonIndex = Memory::Read<int32_t>(address + NameOffset);
        name.Number = Memory::Read<int32_t>(address + NameOffset + 4);
        return name;
    }

    std::string FField::GetName() const {
        return GetFName().ToString();
    }

    uintptr_t FField::GetNext() const {
        return Memory::Read<uintptr_t>(address + NextOffset);
    }

    // ============================================================
    // FProperty
    // ============================================================
    int32_t FProperty::GetArrayDim() const {
        return Memory::Read<int32_t>(address + ArrayDimOffset);
    }

    int32_t FProperty::GetElementSize() const {
        return Memory::Read<int32_t>(address + ElementSizeOffset);
    }

    uint64_t FProperty::GetPropertyFlags() const {
        return Memory::Read<uint64_t>(address + PropertyFlagsOffset);
    }

    int32_t FProperty::GetOffset() const {
        return Memory::Read<int32_t>(address + OffsetInternalOffset);
    }

    std::string FProperty::GetType() const {
        // Read the FFieldClass pointer to determine property type
        // FField has a ClassPrivate at offset 0x8
        uintptr_t fieldClass = Memory::Read<uintptr_t>(address + 0x8);
        if (fieldClass == 0) return "Unknown";

        // FFieldClass has FName at offset 0x0
        FName className{};
        className.ComparisonIndex = Memory::Read<int32_t>(fieldClass + 0x0);
        className.Number = Memory::Read<int32_t>(fieldClass + 0x4);

        std::string typeName = className.ToString();

        // Map internal property type names to C++ types
        if (typeName == "BoolProperty") return "bool";
        if (typeName == "Int8Property") return "int8_t";
        if (typeName == "Int16Property") return "int16_t";
        if (typeName == "IntProperty") return "int32_t";
        if (typeName == "Int64Property") return "int64_t";
        if (typeName == "ByteProperty") return "uint8_t";
        if (typeName == "UInt16Property") return "uint16_t";
        if (typeName == "UInt32Property") return "uint32_t";
        if (typeName == "UInt64Property") return "uint64_t";
        if (typeName == "FloatProperty") return "float";
        if (typeName == "DoubleProperty") return "double";
        if (typeName == "NameProperty") return "FName";
        if (typeName == "StrProperty") return "FString";
        if (typeName == "TextProperty") return "FText";

        if (typeName == "ObjectProperty" || typeName == "ObjectPtrProperty") {
            // Could read the PropertyClass to get specific type
            return "UObject*";
        }
        if (typeName == "ClassProperty" || typeName == "ClassPtrProperty") {
            return "UClass*";
        }
        if (typeName == "WeakObjectProperty") return "TWeakObjectPtr<UObject>";
        if (typeName == "SoftObjectProperty") return "TSoftObjectPtr<UObject>";
        if (typeName == "LazyObjectProperty") return "TLazyObjectPtr<UObject>";
        if (typeName == "InterfaceProperty") return "TScriptInterface<IInterface>";

        if (typeName == "StructProperty") {
            // Read inner struct to get actual type name
            uintptr_t innerStruct = Memory::Read<uintptr_t>(address + 0x78);
            if (innerStruct) {
                UObject structObj{};
                structObj.address = innerStruct;
                return "F" + structObj.GetName();
            }
            return "FUnknownStruct";
        }

        if (typeName == "ArrayProperty") return "TArray<>";
        if (typeName == "MapProperty") return "TMap<>";
        if (typeName == "SetProperty") return "TSet<>";
        if (typeName == "EnumProperty") return "TEnumAsByte<>";
        if (typeName == "DelegateProperty") return "FDelegate";
        if (typeName == "MulticastDelegateProperty") return "FMulticastDelegate";
        if (typeName == "MulticastInlineDelegateProperty") return "FMulticastInlineDelegate";
        if (typeName == "MulticastSparseDelegateProperty") return "FMulticastSparseDelegate";

        return typeName;
    }

} // namespace Engine
