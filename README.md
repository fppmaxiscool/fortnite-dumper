# Fortnite SDK Dumper

A C++ tool that dumps Unreal Engine 5 SDK structures and offsets from Fortnite's memory at runtime.

## Features

- **Pattern Scanning** - Automatically finds GObjects and GNames via signature scanning
- **Full SDK Dump** - Dumps all classes, structs, enums, properties, and functions
- **Offset Dump** - Extracts 60+ commonly used game offsets into a ready-to-use header
- **UE5 Compatible** - Supports the modern FField/FProperty system used in UE5
- **Chunked Object Array** - Handles the chunked TUObjectArray format
- **FNamePool** - Reads from the UE5 block-based name pool
- **Per-Package Output** - Organizes SDK dump into separate headers per package

## Building

### Requirements

- Windows 10/11
- CMake 3.16+
- Visual Studio 2022 (or any C++20 compatible MSVC compiler)

### Build Steps

```bash
mkdir build
cd build
cmake .. -G "Visual Studio 17 2022" -A x64
cmake --build . --config Release
```

Or with command line:

```bash
mkdir build && cd build
cmake ..
cmake --build . --config Release
```

## Usage

1. Launch Fortnite and wait until you're in the lobby or a match
2. Run `FortniteDumper.exe` as Administrator
3. Wait for the dump to complete

### Command Line Options

```
FortniteDumper.exe [options]

Options:
  --offsets-only    Only dump offsets (much faster)
  --sdk-only        Only dump full SDK structures
  --output <dir>    Output directory (default: SDK_Dump)
  --help            Show help
```

### Output

The tool generates:

- `SDK_Dump/offsets.h` - All game offsets in a ready-to-use C++ header
- `SDK_Dump/<PackageName>.h` - Full SDK dump organized by package

## Project Structure

```
fortnite-dumper/
├── CMakeLists.txt
├── include/
│   ├── engine.h        # UE5 structure definitions
│   ├── memory.h        # Process memory reading utilities
│   ├── names.h         # GNames / FNamePool interface
│   ├── objects.h       # GObjects / TUObjectArray interface
│   ├── generator.h     # SDK generation logic
│   └── offsets.h       # Offset dumping interface
└── src/
    ├── main.cpp        # Entry point and CLI
    ├── memory.cpp      # Memory attach, RPM, pattern scan
    ├── engine.cpp      # UObject/UStruct/UClass/FProperty methods
    ├── names.cpp       # FNamePool resolution with caching
    ├── objects.cpp     # Chunked object array traversal
    ├── generator.cpp   # Full SDK dump generation
    └── offsets.cpp     # Common offset extraction
```

## Notes

- **Run as Administrator** - Required for ReadProcessMemory access
- **Pattern Updates** - If patterns break after a game update, update the signatures in `names.cpp` and `objects.cpp`
- **Offset Accuracy** - Offsets are read directly from the engine's reflection system, so they're always accurate for the running version
- **Performance** - Full SDK dump can take 1-5 minutes depending on object count. Use `--offsets-only` for quick offset extraction.

## Disclaimer

This tool is for educational and research purposes only. Use at your own risk.
