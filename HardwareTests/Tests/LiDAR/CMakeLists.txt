cmake_minimum_required(VERSION 2.8)
project(RobotTestCode)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


# Include directories for your source files
include_directories(
     ${CMAKE_SOURCE_DIR}/src   # Incluye tus fuentes
     ${CMAKE_SOURCE_DIR}/tests  # Incluye los tests
     ${CMAKE_SOURCE_DIR}/lib/googletest/googlemock/include # Incluye los headers de gmock
     ${CMAKE_SOURCE_DIR}/lib/googletest/googletest/include # Incluye los headers de gtest
)

############## YDLIDAR SDK START #####################################
# Find ydlidar_sdk package
find_package(ydlidar_sdk REQUIRED)
include_directories(${YDLIDAR_SDK_INCLUDE_DIRS})
link_directories(${YDLIDAR_SDK_LIBRARY_DIRS})
############## YDLIDAR SDK END #####################################

############## SFML START ###########################################
find_package(SFML 2.5 COMPONENTS graphics window system REQUIRED)
include_directories(${SFML_INCLUDE_DIR})
set(SFML_LIBRARIES sfml-graphics sfml-window sfml-system)
############## SFML END #############################################

############## OpenCV START ###########################################
find_package(OpenCV REQUIRED)
include_directories(${OpenCV_INCLUDE_DIRS})
############## OpenCV END #############################################

# Add the source files
add_executable(${PROJECT_NAME} 
    src/RealLidar.cpp
    src/main.cpp
)

# Link libraries
target_link_libraries(${PROJECT_NAME} 
    ${YDLIDAR_SDK_LIBRARIES}
    ${SFML_LIBRARIES} 
    ${OpenCV_LIBS}
)

############## GTEST and GMOCK START ######################################
# Add the GoogleTest library subdirectory
add_subdirectory(lib/googletest)
enable_testing()

# Add test executable
add_executable(lidar_test 
    tests/LidarTest.cpp 
    src/RealLidar.cpp  # Incluye el código necesario para los tests
)

# Link your test executable with gtest, gmock, and other libraries
target_link_libraries(lidar_test
    gmock
    gmock_main
    gtest
    gtest_main
    ${YDLIDAR_SDK_LIBRARIES}
    ${SFML_LIBRARIES}
    ${OpenCV_LIBS}
)


# Register tests
include(GoogleTest)
gtest_discover_tests(lidar_test)
############## GTEST END #############################################
