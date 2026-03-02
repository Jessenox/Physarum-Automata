cmake_minimum_required(VERSION 3.20)

project(PhysarumVulkan LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Vulkan REQUIRED)
find_package(glfw3 QUIET)
find_package(OpenCV QUIET COMPONENTS core imgcodecs imgproc)

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
    "${CMAKE_CURRENT_SOURCE_DIR}/shaders/attractor.comp"
    "${CMAKE_CURRENT_SOURCE_DIR}/shaders/physarum.comp"
    "${CMAKE_CURRENT_SOURCE_DIR}/shaders/quad.vert"
    "${CMAKE_CURRENT_SOURCE_DIR}/shaders/quad.frag"
    "${CMAKE_CURRENT_SOURCE_DIR}/shaders/rect.vert"
    "${CMAKE_CURRENT_SOURCE_DIR}/shaders/rect.frag"
)
file(MAKE_DIRECTORY "${SHADER_OUTPUT_DIR}")

set(SPIRV_OUTPUTS)
foreach(SHADER_SOURCE IN LISTS SHADER_SOURCES)
    get_filename_component(SHADER_NAME "${SHADER_SOURCE}" NAME)
    set(SPIRV_OUTPUT "${SHADER_OUTPUT_DIR}/${SHADER_NAME}.spv")

    add_custom_command(
        OUTPUT "${SPIRV_OUTPUT}"
        COMMAND "${CMAKE_COMMAND}" -E make_directory "${SHADER_OUTPUT_DIR}"
        COMMAND "${GLSLANG_VALIDATOR}" -V "${SHADER_SOURCE}" -o "${SPIRV_OUTPUT}"
        DEPENDS "${SHADER_SOURCE}"
        COMMENT "Compiling ${SHADER_NAME} -> ${SPIRV_OUTPUT}"
        VERBATIM
    )

    list(APPEND SPIRV_OUTPUTS "${SPIRV_OUTPUT}")
endforeach()

add_custom_target(PhysarumVulkanShaders ALL DEPENDS ${SPIRV_OUTPUTS})

add_executable(PhysarumVulkan
    src/application/AppController.h
    src/application/AppController.cpp
    src/AppModel.h
    src/AppModel.cpp
    src/AppViewModel.h
    src/AppViewModel.cpp
    src/AttractorCompute.h
    src/AttractorCompute.cpp
    src/AttractorGenerator.h
    src/AttractorGenerator.cpp
    src/FileDialog.h
    src/FileDialog.cpp
    src/infrastructure/AttractorGraphExporter.h
    src/infrastructure/AttractorGraphExporter.cpp
    src/main.cpp
    src/PhysarumSim.h
    src/PhysarumSim.cpp
    src/VulkanApp.h
    src/VulkanApp.cpp
    src/VulkanHelpers.h
)

add_dependencies(PhysarumVulkan PhysarumVulkanShaders)
target_include_directories(PhysarumVulkan PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src")
target_link_libraries(PhysarumVulkan PRIVATE Vulkan::Vulkan "${GLFW_TARGET}")

if(OpenCV_FOUND)
    target_include_directories(PhysarumVulkan PRIVATE ${OpenCV_INCLUDE_DIRS})
    target_link_libraries(PhysarumVulkan PRIVATE ${OpenCV_LIBS})
    target_compile_definitions(PhysarumVulkan PRIVATE PHYSARUM_VULKAN_HAS_OPENCV=1)
else()
    target_compile_definitions(PhysarumVulkan PRIVATE PHYSARUM_VULKAN_HAS_OPENCV=0)
endif()

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(PhysarumVulkan PRIVATE -Wall -Wextra -Wpedantic)
endif()

set_target_properties(PhysarumVulkan PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
)
