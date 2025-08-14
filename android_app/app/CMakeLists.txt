cmake_minimum_required(VERSION 3.10)
project(android_triangle_app C)

# Path to GLFW repo root (two levels up from this app folder)
set(GLFW_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../..")

# Ensure examples/tests/docs are not built from GLFW root
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)

# Add GLFW root; this will build the glfw target for Android
add_subdirectory("${GLFW_ROOT}" "${CMAKE_BINARY_DIR}/glfw_root_build")

# Our Android shared library using GLFW and GLES2
add_library(android_triangle SHARED
    "${GLFW_ROOT}/examples/android_triangle.c"
)

target_compile_definitions(android_triangle PRIVATE GLFW_INCLUDE_NONE)

# Include GLFW headers and deps
target_include_directories(android_triangle PRIVATE
    "${GLFW_ROOT}/include"
    "${GLFW_ROOT}/deps"
)

# Link against glfw and Android/GLES libs
find_library(log-lib log)
find_library(android-lib android)
find_library(glesv2-lib GLESv2)

target_link_libraries(android_triangle
    PRIVATE glfw ${glesv2-lib} ${android-lib} ${log-lib}
)

set_target_properties(android_triangle PROPERTIES OUTPUT_NAME "android_triangle")
