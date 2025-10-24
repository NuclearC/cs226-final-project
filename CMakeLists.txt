cmake_minimum_required(VERSION 3.10)
project(CS226FinalProject)

set(CMAKE_C_STANDARD 23)

find_package(SDL3 REQUIRED)
find_package(Vulkan REQUIRED)

file(GLOB SOURCE_FILES 
    ${PROJECT_SOURCE_DIR}/source/*.c)

add_subdirectory(${PROJECT_SOURCE_DIR}/shaders)
add_executable(CS226FinalProject ${SOURCE_FILES}
        source/texture_renderer.c
        source/texture_renderer.h)
add_dependencies(CS226FinalProject Shaders)
target_link_libraries(CS226FinalProject SDL3::SDL3 Vulkan::Vulkan m)


