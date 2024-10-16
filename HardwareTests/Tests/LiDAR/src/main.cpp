#include "RealLidar.h"
#include <iostream>

int main() {
    RealLidar lidar;

    if (lidar.initialize()) {
        if (lidar.turnOn()) {
            std::cout << "LiDAR está encendido y funcionando." << std::endl;

            // Simula procesamiento de escaneo
            LaserScan scan;
            if (lidar.doProcessSimple(scan)) {
                std::cout << "Escaneo de LiDAR procesado correctamente." << std::endl;
            }

            lidar.turnOff();
        } else {
            std::cerr << "Error al encender el LiDAR." << std::endl;
        }
    } else {
        std::cerr << "Error al inicializar el LiDAR." << std::endl;
    }

    return 0;
}
