cmake_minimum_required(VERSION 3.16)
project(FortniteDumper LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(FortniteDumper
    src/main.cpp
    src/memory.cpp
    src/engine.cpp
    src/names.cpp
    src/objects.cpp
    src/generator.cpp
    src/offsets.cpp
)

target_include_directories(FortniteDumper PRIVATE include)
target_link_libraries(FortniteDumper PRIVATE psapi)

# Embed manifest for auto admin elevation (UAC prompt)
if(MSVC)
    set_target_properties(FortniteDumper PROPERTIES
        LINK_FLAGS "/MANIFEST:EMBED /MANIFESTINPUT:${CMAKE_SOURCE_DIR}/FortniteDumper.manifest"
    )
endif()
