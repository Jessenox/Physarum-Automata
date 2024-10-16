cmake_minimum_required(VERSION 2.8)
project(utils)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
add_definitions(-std=c++11) # Use C++11

# Include directories
include_directories(
     ${CMAKE_SOURCE_DIR}
)

############## YDLIDAR SDK START #####################################
# Find ydlidar_sdk package
find_package(ydlidar_sdk REQUIRED)
# Include directories
include_directories(
  ${YDLIDAR_SDK_INCLUDE_DIRS}
)

# Link library directories
link_directories(${YDLIDAR_SDK_LIBRARY_DIRS})
############## YDLIDAR SDK END #####################################

############## SFML START ###########################################
# Find SFML package
find_package(SFML 2.5 COMPONENTS graphics window system REQUIRED)

# Include directories for SFML
include_directories(${SFML_INCLUDE_DIR})

# Add the SFML libraries
set(SFML_LIBRARIES sfml-graphics sfml-window sfml-system)
############## SFML END #############################################

############## OpenCV START ###########################################
find_package(OpenCV REQUIRED)
include_directories(${OpenCV_INCLUDE_DIRS})
############## OpenCV END #############################################

############## LIBUSB START ###########################################
find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBUSB REQUIRED libusb-1.0)
include_directories(${LIBUSB_INCLUDE_DIRS})
link_directories(${LIBUSB_LIBRARY_DIRS})
############## LIBUSB END #############################################

############## LIBFREENECT START ######################################
find_package(libfreenect REQUIRED)
include_directories(${libfreenect_INCLUDE_DIRS})
link_directories(${libfreenect_LIBRARY_DIRS})
############## LIBFREENECT END ########################################


find_package(Boost REQUIRED COMPONENTS system)
include_directories(${Boost_INCLUDE_DIRS})


# Add executable
add_library(${PROJECT_NAME} STATIC utils.cpp)

# Link your project to ydlidar_sdk, SFML, OpenCV, libusb, and libfreenect libraries
target_link_libraries(${PROJECT_NAME} ${YDLIDAR_SDK_LIBRARIES} ${SFML_LIBRARIES} ${OpenCV_LIBS} ${LIBUSB_LIBRARIES} freenect)
# Añadir Google Test
include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/release-1.12.1.zip
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(googletest)

enable_testing()

# Añadir el ejecutable de pruebas
add_executable(tests tests.cpp utils.cpp)

# Vincular las bibliotecas necesarias
target_link_libraries(tests
    gtest
    gtest_main
    ${OpenCV_LIBS}
    ${SFML_LIBRARIES}
    ${YDLIDAR_SDK_LIBRARIES}  # Añadir la biblioteca YDLIDAR
    ${LIBUSB_LIBRARIES}       # Añadir libusb
    freenect                  # Añadir la biblioteca Freenect
)

# Registrar las pruebas
include(GoogleTest)
gtest_discover_tests(tests)
