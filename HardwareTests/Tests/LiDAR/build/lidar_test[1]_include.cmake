if(EXISTS "/home/eduardo-hernadez-vergara/Documentos/TrabajoTerminal/Physarum-Automata/HardwareTests/Tests/LiDAR/build/lidar_test[1]_tests.cmake")
  include("/home/eduardo-hernadez-vergara/Documentos/TrabajoTerminal/Physarum-Automata/HardwareTests/Tests/LiDAR/build/lidar_test[1]_tests.cmake")
else()
  add_test(lidar_test_NOT_BUILT lidar_test_NOT_BUILT)
endif()