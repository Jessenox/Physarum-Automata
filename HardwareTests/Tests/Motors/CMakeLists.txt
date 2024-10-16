cmake_minimum_required(VERSION 2.8)
project(RobotTestCode)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Include directories for your source files and tests
include_directories(
     ${CMAKE_SOURCE_DIR}/src
     ${CMAKE_SOURCE_DIR}/tests
     ${CMAKE_SOURCE_DIR}/lib/googletest/googlemock/include
     ${CMAKE_SOURCE_DIR}/lib/googletest/googletest/include
)

# Mock Pigpio Header
include_directories(${CMAKE_SOURCE_DIR}/mock)

# GoogleTest
add_subdirectory(lib/googletest)
enable_testing()

# Motor control executable for tests
# Agrega mock_pigpio.cpp a los archivos de origen para motor_test
add_executable(motor_test 
    tests/motor_test.cpp
    src/motor_control.cpp
    #src/mock_pigpio.cpp  # Asegúrate de incluir mock_pigpio.cpp
)


# Link gtest and gmock
target_link_libraries(motor_test
    gmock
    gmock_main
    gtest
    gtest_main
)

# Register tests
include(GoogleTest)
gtest_discover_tests(motor_test)
