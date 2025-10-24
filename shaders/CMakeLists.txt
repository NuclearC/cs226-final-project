
set(SHADERS_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR})

file(GLOB SHADERS_VERTEX_FILES ${SHADERS_SOURCE_DIR}/*_vs.glsl)
file(GLOB SHADERS_FRAGMENT_FILES ${SHADERS_SOURCE_DIR}/*_fs.glsl)
file(GLOB NC_SHADERS_COMPUTE_FILES ${SHADERS_SOURCE_DIR}/*_c.glsl)

set(NC_SHADERS_SRC_FILES 
    ${NC_SHADERS_VERTEX_FILES}
    ${NC_SHADERS_FRAGMENT_FILES}
    ${NC_SHADERS_COMPUTE_FILES}
)

if (WIN32)
    set(GLSLC_EXECUTABLE $ENV{VK_SDK_PATH}/bin/glslc.exe)
else()
    set(GLSLC_EXECUTABLE glslc)
endif()

foreach(_file ${NC_SHADERS_SRC_FILES})
    get_filename_component(OUTPUT_FILE ${_file} NAME)
    string(REPLACE ".glsl" ".spv" NEW_OUTPUT_FILE ${OUTPUT_FILE})
    list(APPEND NC_OUTPUT_FILES ${NEW_OUTPUT_FILE})
endforeach()

add_custom_command(
  OUTPUT ${NC_OUTPUT_FILES}
  # COMMAND ${GLSLC_EXECUTABLE} -c -fshader-stage=fragment ${NC_SHADERS_FRAGMENT_FILES}
  # COMMAND ${GLSLC_EXECUTABLE} -c -fshader-stage=vertex ${NC_SHADERS_VERTEX_FILES}
  COMMAND ${GLSLC_EXECUTABLE} -c -fshader-stage=compute ${NC_SHADERS_COMPUTE_FILES}
  DEPENDS ${NC_SHADERS_SRC_FILES}
  COMMENT "SPIR-V: Compiling shaders...")

message("List of output shaders: " ${NC_OUTPUT_FILES})
add_custom_target(Shaders ALL
    DEPENDS ${NC_OUTPUT_FILES}
    SOURCES ${NC_SHADERS_SRC_FILES})