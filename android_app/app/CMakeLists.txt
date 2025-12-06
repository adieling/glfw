
cmake_minimum_required(VERSION 3.10)
project(android_triangle_app C)

# === Using PREBUILT GLFW libraries ===
# Much faster build: no need to compile GLFW from source

# Get absolute path to project root (glfw_android/)
# From app/CMakeLists.txt, go up 2 levels to reach glfw_android/
get_filename_component(GLFW_PROJECT_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../.." ABSOLUTE)

set(GLFW_PREBUILT "${GLFW_PROJECT_ROOT}/glfw_prebuilt")
set(GLFW_ROOT "${GLFW_PROJECT_ROOT}")  # Still needed for Vulkan deps

message(STATUS "GLFW Project Root: ${GLFW_PROJECT_ROOT}")
message(STATUS "GLFW Prebuilt: ${GLFW_PREBUILT}")

# Determine ABI and set prebuilt lib path
if(CMAKE_ANDROID_ARCH_ABI STREQUAL "arm64-v8a")
    set(GLFW_LIB_PATH "${GLFW_PREBUILT}/lib/arm64-v8a")
    message(STATUS "Using prebuilt GLFW for arm64-v8a at: ${GLFW_LIB_PATH}")
elseif(CMAKE_ANDROID_ARCH_ABI STREQUAL "x86_64")
    set(GLFW_LIB_PATH "${GLFW_PREBUILT}/lib/x86_64")
    message(STATUS "Using prebuilt GLFW for x86_64 at: ${GLFW_LIB_PATH}")
else()
    message(FATAL_ERROR "Unsupported ABI: ${CMAKE_ANDROID_ARCH_ABI}")
endif()

# Our Android shared library using GLFW and Vulkan
# Interactive version: Triangle follows touch input (arrays enabled)
# Source moved to vulkan_demo directory
add_library(android_vulkan_triangle SHARED
    "../../vulkan_demo/android_vulkan_triangle_interactive.c"
)

target_compile_definitions(android_vulkan_triangle PRIVATE GLFW_INCLUDE_NONE)

# Include prebuilt GLFW headers and Vulkan deps from GLFW root
target_include_directories(android_vulkan_triangle PRIVATE
    "${GLFW_PREBUILT}/include"  # GLFW headers (glfw3.h, glfw3native.h)
    "${GLFW_ROOT}/deps"         # Vulkan headers
)

# Link against prebuilt GLFW and Android/Vulkan libs
find_library(log-lib log)
find_library(android-lib android)
find_package(Vulkan REQUIRED)

# Link prebuilt libglfw.a (static library)
target_link_libraries(android_vulkan_triangle
    PRIVATE 
    "${GLFW_LIB_PATH}/libglfw.a"  # Prebuilt static GLFW library
    Vulkan::Vulkan 
    ${android-lib} 
    ${log-lib}
)

set_target_properties(android_vulkan_triangle PROPERTIES OUTPUT_NAME "android_vulkan_triangle")
