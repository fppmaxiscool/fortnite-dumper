cmake_minimum_required(VERSION 3.16)
project(FortniteDumper LANGUAGES CXX RC)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ImGui sources (bundled in libs/imgui/)
set(IMGUI_DIR ${CMAKE_SOURCE_DIR}/libs/imgui)
set(IMGUI_SOURCES
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_demo.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/backends/imgui_impl_win32.cpp
    ${IMGUI_DIR}/backends/imgui_impl_dx11.cpp
)

add_executable(FortniteDumper WIN32
    src/main.cpp
    src/memory.cpp
    src/engine.cpp
    src/names.cpp
    src/objects.cpp
    src/generator.cpp
    src/offsets.cpp
    src/gui.cpp
    ${IMGUI_SOURCES}
    resource.rc
)

target_include_directories(FortniteDumper PRIVATE
    include
    ${IMGUI_DIR}
    ${IMGUI_DIR}/backends
)

target_link_libraries(FortniteDumper PRIVATE
    psapi
    d3d11
    dxgi
    d3dcompiler
)

# Disable CMake's auto-manifest to avoid duplicate with resource.rc
if(MSVC)
    set_target_properties(FortniteDumper PROPERTIES
        LINK_FLAGS "/MANIFEST:NO"
    )
endif()
