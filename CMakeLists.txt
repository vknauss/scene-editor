cmake_minimum_required(VERSION 3.10)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

project(scene-editor)

macro(msys_to_cmake_path MsysPath ResultingPath)
	if(WIN32)
		string(REGEX REPLACE "^/([a-zA-Z])/" "\\1:/" ${ResultingPath} "${MsysPath}")
	else()
		set(${ResultingPath} "${MsysPath}")
	endif()
endmacro()

set(SHADERS_SRC_PATH_PREFIX "${CMAKE_HOME_DIRECTORY}/src/shaders")
set(SHADERS_DST_PATH_PREFIX "${CMAKE_CURRENT_BINARY_DIR}/shaders")
function(add_shaders_target TARGET)
    set(OUT_PATHS "")
    foreach(SHADER ${ARGN})
        set(SHADER_SRC_PATH ${SHADERS_SRC_PATH_PREFIX}/${SHADER})
        set(SHADER_DST_PATH ${SHADERS_DST_PATH_PREFIX}/${SHADER})
        list(APPEND OUT_PATHS ${SHADER_DST_PATH})
        add_custom_command(
            OUTPUT ${SHADER_DST_PATH}
            COMMAND ${CMAKE_COMMAND} -E copy ${SHADER_SRC_PATH} ${SHADER_DST_PATH}
            DEPENDS ${SHADER_SRC_PATH})
    endforeach()
    add_custom_target(${TARGET} DEPENDS ${OUT_PATHS})
endfunction()


find_package(OpenGL REQUIRED)
find_package(GLEW REQUIRED)
find_package(glfw3 3.3 REQUIRED)

if(NOT VVM_INCLUDE_DIR)
    message(FATAL_ERROR "Please set VVM_INCLUDE_DIR to a valid directory.")
endif()

if(NOT OGU_INCLUDE_DIR)
    message(FATAL_ERROR "Please set OGU_INCLUDE_DIR to a valid directory.")
endif()

if(NOT OGU_LIBRARY_PATH)
    message(FATAL_ERROR "Please set OGU_LIBRARY_PATH to a valid library path.")
endif()

add_executable(scene-editor
    "src/main.cpp"
    "src/mesh_renderer.cpp"
    "src/mesh_vertex_buffer_writer.cpp"
    "src/mesh_io.cpp")

set(SHADERS
    "vertex.glsl"
    "fragment.glsl")

# foreach(SHADER ${SHADERS})
#     message(STATUS "${SHADER}")
#     set(_SHADER_SRC_PATH ${SHADERS_SRC_PATH_PREFIX}/${SHADER})
#     set(_SHADER_DST_PATH ${SHADERS_DST_PATH_PREFIX}/${SHADER})
#     message(STATUS "Executing: ${CMAKE_COMMAND} -E copy ${_SHADER_SRC_PATH} ${_SHADER_DST_PATH}")
#     add_custom_command(
#         TARGET scene-editor POST_BUILD
#         # OUTPUT ${_SHADER_DST_PATH}
#         COMMAND ${CMAKE_COMMAND} -E copy ${_SHADER_SRC_PATH} ${_SHADER_DST_PATH}
#         MAIN_DEPENDENCY ${_SHADER_SRC_PATH})
# endforeach()

add_shaders_target(shaders ${SHADERS})

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