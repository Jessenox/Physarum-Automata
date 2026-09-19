cmake_minimum_required(VERSION 3.20)

project(NetworkACOVulkan LANGUAGES CXX)

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
    shaders/ui.vert
    shaders/ui.frag
    shaders/aco_init.comp
    shaders/aco_begin.comp
    shaders/aco_construct.comp
    shaders/aco_accumulate.comp
    shaders/aco_advance.comp
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
add_custom_target(NetworkACOVulkanShaders ALL DEPENDS ${SPIRV_OUTPUTS})

add_library(NetworkACODomain
    ../comparison/Scenario.cpp
    src/domain/Graph.cpp
    src/domain/NetworkACO.cpp
)
target_include_directories(NetworkACODomain PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/src"
    "${CMAKE_CURRENT_SOURCE_DIR}/../comparison"
)

add_executable(NetworkACOVulkan
    src/main.cpp
    src/presentation/VulkanApp.cpp
)
add_dependencies(NetworkACOVulkan NetworkACOVulkanShaders)
target_include_directories(NetworkACOVulkan PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src")
target_compile_definitions(NetworkACOVulkan PRIVATE
    NETWORK_ACO_SHADER_DIRECTORY="${SHADER_OUTPUT_DIR}"
)
target_link_libraries(NetworkACOVulkan PRIVATE NetworkACODomain Vulkan::Vulkan "${GLFW_TARGET}")

include(CTest)
if(BUILD_TESTING)
    add_executable(NetworkACOTests tests/NetworkACOTests.cpp)
    target_link_libraries(NetworkACOTests PRIVATE NetworkACODomain)
    if(MSVC)
        target_compile_options(NetworkACOTests PRIVATE /UNDEBUG)
    else()
        target_compile_options(NetworkACOTests PRIVATE -UNDEBUG)
    endif()
    add_test(NAME NetworkACOTests COMMAND NetworkACOTests)
endif()

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(NetworkACODomain PRIVATE -Wall -Wextra -Wpedantic)
    target_compile_options(NetworkACOVulkan PRIVATE -Wall -Wextra -Wpedantic -Wno-missing-field-initializers)
endif()
