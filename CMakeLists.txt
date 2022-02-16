cmake_minimum_required(VERSION 3.10)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

project(scene-editor)

# generate a custom target to copy files from one location to another
# TARGET_NAME: name of the target to generate
# SRC_PATH_PREFIX: prepended to every filename passed in to generate source filepaths
# DST_PATH_PREFIX: prepended to every filename passed in to generate destination filepaths
function(add_copy_target TARGET_NAME SRC_PATH_PREFIX DST_PATH_PREFIX)
    set(DST_PATHS "")
    foreach(FILENAME ${ARGN})
        set(SRC_PATH ${SRC_PATH_PREFIX}/${FILENAME})
        set(DST_PATH ${DST_PATH_PREFIX}/${FILENAME})
        list(APPEND DST_PATHS ${DST_PATH})
        add_custom_command(
            OUTPUT ${DST_PATH}
            COMMAND ${CMAKE_COMMAND} -E copy ${SRC_PATH} ${DST_PATH}
            DEPENDS ${SRC_PATH})
    endforeach()
    add_custom_target(${TARGET_NAME} DEPENDS ${DST_PATHS})
endfunction()

# find required system libraries
cmake_policy(SET CMP0072 NEW)
find_package(OpenGL REQUIRED)
find_package(GLEW REQUIRED)
find_package(glfw3 3.3 REQUIRED)

# find user libraries. need to set these paths in configure
if(NOT VVM_INCLUDE_DIR)
    message(FATAL_ERROR "Please set VVM_INCLUDE_DIR to a valid directory.")
endif()

if(NOT OGU_INCLUDE_DIR)
    message(FATAL_ERROR "Please set OGU_INCLUDE_DIR to a valid directory.")
endif()

if(NOT OGU_LIBRARY_PATH)
    message(FATAL_ERROR "Please set OGU_LIBRARY_PATH to a valid library path.")
endif()

# create targets
add_executable(scene-editor
    "src/main.cpp"
    "src/mesh_renderer.cpp"
    "src/mesh_vertex_buffer_writer.cpp"
    "src/mesh_io.cpp")

set(SHADERS
    "vertex.glsl"
    "fragment.glsl")

add_copy_target(shaders
    "${CMAKE_HOME_DIRECTORY}/src/shaders"
    "${CMAKE_CURRENT_BINARY_DIR}/shaders"
    ${SHADERS})

add_copy_target(data
    "${CMAKE_HOME_DIRECTORY}/data"
    "${CMAKE_CURRENT_BINARY_DIR}/data"
    "untitled.mbin")

# enable asan on gcc/unix configuration (not available for mingw)
if(UNIX AND CMAKE_COMPILER_IS_GNUCXX)
    target_compile_options(scene-editor PUBLIC $<$<CONFIG:DEBUG>:-fno-omit-frame-pointer -fsanitize=address>)
    target_link_options(scene-editor PUBLIC $<$<CONFIG:DEBUG>:-fno-omit-frame-pointer -fsanitize=address>)
endif()

target_include_directories(scene-editor PUBLIC
    ${CMAKE_HOME_DIRECTORY}/include
    ${VVM_INCLUDE_DIR}
    ${OGU_INCLUDE_DIR})

target_link_libraries(scene-editor PUBLIC
    ${OGU_LIBRARY_PATH}
    OpenGL::GL
    GLEW::GLEW
    glfw)

add_dependencies(scene-editor shaders)
add_dependencies(scene-editor data)