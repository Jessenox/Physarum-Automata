cmake_minimum_required(VERSION 3.20)

project(DijkstraVulkan LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Vulkan REQUIRED)
find_package(glfw3 QUIET)
if(TARGET glfw)
    set(GLFW_TARGET glfw)
elseif(TARGET glfw3::glfw)
    set(GLFW_TARGET glfw3::glfw)
else()
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(GLFW3 REQUIRED IMPORTED_TARGET glfw3)
    set(GLFW_TARGET PkgConfig::GLFW3)
endif()

find_program(GLSLANG_VALIDATOR NAMES glslangValidator)
if(NOT GLSLANG_VALIDATOR)
    message(FATAL_ERROR "glslangValidator not found. Install glslang-tools or the Vulkan SDK.")
endif()

set(SHADER_OUTPUT_DIR "${CMAKE_BINARY_DIR}/shaders")
set(SHADER_SOURCES
    shaders/grid.vert
    shaders/grid.frag
    shaders/ui.vert
    shaders/ui.frag
    shaders/dijkstra_reset.comp
    shaders/dijkstra_clear.comp
    shaders/dijkstra_find_min.comp
    shaders/dijkstra_choose_min.comp
    shaders/dijkstra_relax.comp
)
set(SPIRV_OUTPUTS)
foreach(SHADER_SOURCE IN LISTS SHADER_SOURCES)
    get_filename_component(SHADER_NAME "${SHADER_SOURCE}" NAME)
    set(SPIRV_OUTPUT "${SHADER_OUTPUT_DIR}/${SHADER_NAME}.spv")
    add_custom_command(
        OUTPUT "${SPIRV_OUTPUT}"
        COMMAND "${CMAKE_COMMAND}" -E make_directory "${SHADER_OUTPUT_DIR}"
        COMMAND "${GLSLANG_VALIDATOR}" -V "${CMAKE_CURRENT_SOURCE_DIR}/${SHADER_SOURCE}" -o "${SPIRV_OUTPUT}"
        DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/${SHADER_SOURCE}"
        COMMENT "Compiling ${SHADER_NAME}"
        VERBATIM
    )
    list(APPEND SPIRV_OUTPUTS "${SPIRV_OUTPUT}")
endforeach()
add_custom_target(DijkstraVulkanShaders ALL DEPENDS ${SPIRV_OUTPUTS})

add_library(DijkstraDomain src/domain/GridModel.cpp)
target_include_directories(DijkstraDomain PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/src")

add_executable(DijkstraVulkan
    src/main.cpp
    src/presentation/VulkanApp.cpp
)
add_dependencies(DijkstraVulkan DijkstraVulkanShaders)
target_include_directories(DijkstraVulkan PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src")
target_link_libraries(DijkstraVulkan PRIVATE DijkstraDomain Vulkan::Vulkan "${GLFW_TARGET}")

include(CTest)
if(BUILD_TESTING)
    add_executable(DijkstraDomainTests tests/GridModelTests.cpp)
    target_link_libraries(DijkstraDomainTests PRIVATE DijkstraDomain)
    add_test(NAME DijkstraDomainTests COMMAND DijkstraDomainTests)
endif()

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(DijkstraDomain PRIVATE -Wall -Wextra -Wpedantic)
    target_compile_options(DijkstraVulkan PRIVATE -Wall -Wextra -Wpedantic)
endif()
