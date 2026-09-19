cmake_minimum_required(VERSION 3.20)

project(PacAlgorithmBenchmarks LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Vulkan REQUIRED)

add_library(ComparisonScenario Scenario.cpp)
target_include_directories(ComparisonScenario PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")

add_executable(DijkstraBenchmark
    DijkstraBenchmark.cpp
    ../DijkstraVulkan/src/domain/GridModel.cpp
)
target_include_directories(DijkstraBenchmark PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}/../DijkstraVulkan/src"
)
target_link_libraries(DijkstraBenchmark PRIVATE ComparisonScenario)

add_executable(PhysarumBenchmark PhysarumBenchmark.cpp)
target_include_directories(PhysarumBenchmark PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(PhysarumBenchmark PRIVATE ComparisonScenario Vulkan::Vulkan)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(ComparisonScenario PRIVATE -Wall -Wextra -Wpedantic)
    target_compile_options(DijkstraBenchmark PRIVATE -Wall -Wextra -Wpedantic)
    target_compile_options(PhysarumBenchmark PRIVATE
        -Wall -Wextra -Wpedantic -Wno-missing-field-initializers
    )
endif()
