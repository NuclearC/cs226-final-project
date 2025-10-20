cmake_minimum_required(VERSION 3.10)
project(CS226FinalProject)


find_package(SDL3 REQUIRED)
find_package(Vulkan REQUIRED)

file(GLOB SOURCE_FILES 
    ${PROJECT_SOURCE_DIR}/source/*.c)

add_executable(CS226FinalProject ${SOURCE_FILES})
target_link_libraries(CS226FinalProject SDL3::SDL3 Vulkan::Vulkan)


