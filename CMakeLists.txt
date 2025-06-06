cmake_minimum_required(VERSION 3.5)
project(nuum LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_POLICY_VERSION_MINIMUM 3.5)
set(CMAKE_DEBUG_POSTFIX "-Debug")

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_SUPPRESS_DEVELOPER_WARNINGS 1 CACHE BOOL "Suppress developer warnings")

include(FetchContent)

# -------------------------------------------------------------------
# Detect platform
# -------------------------------------------------------------------
if(WIN32)
    set(PLATFORM_WINDOWS TRUE)
    if(MSVC)
        foreach(flag_var
    CMAKE_C_FLAGS_RELEASE CMAKE_C_FLAGS_DEBUG
    CMAKE_CXX_FLAGS_RELEASE CMAKE_CXX_FLAGS_DEBUG)
            string (REPLACE "/MD" "/MT" ${flag_var} "${${flag_var}}")
        endforeach()
    endif()
    if(MSVC)
        add_compile_options("/Zc:__cplusplus" "/permissive-" "/Zc:preprocessor" "/EHsc")
        add_compile_definitions("_CRT_SECURE_NO_WARNINGS")
    endif()
elseif(APPLE)
    set(PLATFORM_MACOS TRUE)
    set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
elseif(UNIX)
    set(PLATFORM_LINUX TRUE)
    set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
endif()

# -------------------------------------------------------------------
# SDL2
# -------------------------------------------------------------------

if(PLATFORM_WINDOWS)
    FetchContent_Declare(
        SDL2
        GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
        GIT_TAG release-2.32.4
    )
    FetchContent_MakeAvailable(SDL2)
    include_directories(${SDL2_SOURCE_DIR}/include)
elseif(PLATFORM_LINUX OR PLATFORM_MACOS)
    find_package(SDL2 REQUIRED)
    if(SDL2_FOUND)
        include_directories(${SDL2_INCLUDE_DIRS})
    else()
        message(FATAL_ERROR "SDL2 not found")
    endif()
endif()

# -------------------------------------------------------------------
# GLM
# -------------------------------------------------------------------
FetchContent_Declare(
  glm
  GIT_REPOSITORY https://github.com/g-truc/glm.git
  GIT_TAG 1.0.1
)
FetchContent_MakeAvailable(glm)

# -------------------------------------------------------------------
# BGFX + bx + bimg via bgfx.cmake
# -------------------------------------------------------------------
FetchContent_Declare(
    bgfx-cmake
    GIT_REPOSITORY https://github.com/bkaradzic/bgfx.cmake.git
    GIT_TAG master
)
FetchContent_MakeAvailable(bgfx-cmake)

# -------------------------------------------------------------------
# Compile Shaders
# -------------------------------------------------------------------
set(SHADER_DIR ${CMAKE_SOURCE_DIR}/shaders)
set(BGFX_SHADER_INCLUDE_PATH ${SHADER_DIR}/include)
file(GLOB_RECURSE SHADER_FILES_VS ${SHADER_DIR}/vs_*.sc)
file(GLOB_RECURSE SHADER_FILES_FS ${SHADER_DIR}/fs_*.sc)
file(GLOB_RECURSE SHADER_FILES_CS ${SHADER_DIR}/cs_*.sc)
bgfx_compile_shaders(
    TYPE VERTEX
    SHADERS ${SHADER_FILES_VS}
    VARYING_DEF ${SHADER_DIR}/varying.def.sc
    OUTPUT_DIR ${SHADER_DIR}/include/generated
    AS_HEADERS
)
bgfx_compile_shaders(
    TYPE FRAGMENT
    SHADERS ${SHADER_FILES_FS}
    VARYING_DEF ${SHADER_DIR}/varying.def.sc
    OUTPUT_DIR ${SHADER_DIR}/include/generated
    AS_HEADERS
)
bgfx_compile_shaders(
    TYPE COMPUTE
    SHADERS ${SHADER_FILES_CS}
    VARYING_DEF ${SHADER_DIR}/varying.def.sc
    OUTPUT_DIR ${SHADER_DIR}/include/generated
    AS_HEADERS
)

# -------------------------------------------------------------------
# FreeType
# -------------------------------------------------------------------

if(PLATFORM_WINDOWS)
    FetchContent_Declare(
        freetype
        GIT_REPOSITORY https://gitlab.freedesktop.org/freetype/freetype.git
        GIT_TAG VER-2-13-3
    )
    FetchContent_MakeAvailable(freetype)
elseif(PLATFORM_LINUX OR PLATFORM_MACOS)
    find_package(Freetype REQUIRED)
    if(FREETYPE_FOUND)
        include_directories(${FREETYPE_INCLUDE_DIRS})
    else()
        message(FATAL_ERROR "Freetype not found")
    endif()
endif()

# -------------------------------------------------------------------
# ImGui
# -------------------------------------------------------------------
FetchContent_Declare(
    imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG v1.91.9b-docking
)
FetchContent_MakeAvailable(imgui)

add_library(imgui STATIC
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_demo.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_sdl2.cpp
    ${imgui_SOURCE_DIR}/misc/cpp/imgui_stdlib.cpp
    ${imgui_SOURCE_DIR}/misc/freetype/imgui_freetype.cpp
)
target_include_directories(imgui PUBLIC
    ${imgui_SOURCE_DIR}
    ${imgui_SOURCE_DIR}/backends
    ${imgui_SOURCE_DIR}/misc/cpp
    ${imgui_SOURCE_DIR}/misc/freetype
)
target_compile_definitions(imgui PUBLIC
    IMGUI_ENABLE_FREETYPE
)


# -------------------------------------------------------------------
# Sources
# -------------------------------------------------------------------
file(GLOB_RECURSE SRC_FILES ${CMAKE_SOURCE_DIR}/src/*.cpp)
add_executable(nuum ${SRC_FILES} ${SHADER_FILES_VS} ${SHADER_FILES_FS} ${SHADER_FILES_CS})

# -------------------------------------------------------------------
# Includes
# -------------------------------------------------------------------
target_include_directories(nuum PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/shaders/include/generated
    # include directories for dependencies
    ${SDL2_SOURCE_DIR}/include
    ${imgui_SOURCE_DIR}
    ${GLM_INCLUDE_DIR}/include
    ${bgfx_SOURCE_DIR}/bimg/include
    ${bgfx_SOURCE_DIR}/bgfx/include
    ${bgfx_SOURCE_DIR}/bx/include
    ${CMAKE_SOURCE_DIR}/src/shaders/include
    ${CMAKE_SOURCE_DIR}/src/shaders/include/generated
)

# -------------------------------------------------------------------
# Linking
# -------------------------------------------------------------------
target_link_libraries(nuum PRIVATE
    bgfx
    glm
    imgui
    freetype
)
set_target_properties(nuum PROPERTIES DEBUG_POSTFIX ${CMAKE_DEBUG_POSTFIX})

if(PLATFORM_WINDOWS)
    target_include_directories(nuum PRIVATE ${SDL2_SOURCE_DIR}/include)
    target_link_libraries(nuum PRIVATE SDL2main SDL2)
elseif(PLATFORM_LINUX OR PLATFORM_MACOS)
    target_link_libraries(nuum PRIVATE ${SDL2_LIBRARIES})
    target_link_libraries(nuum PRIVATE m dl pthread)
    if(PLATFORM_MACOS)
        find_library(COCOA_LIBRARY Cocoa REQUIRED)
        target_link_libraries(nuum PRIVATE ${COCOA_LIBRARY} ${SDL2_LIBRARIES})
    endif()
endif()
