#pragma once
#include "ILidar.h"
#include "CYdLidar.h"  // Para incluir las funciones necesarias como initialize, turnOn, turnOff

class RealLidar : public ILidar {
public:
    // Métodos específicos del LiDAR
    bool initialize() {
        // Inicializa el LiDAR
        return laser_.initialize();
    }

    bool turnOn() {
        // Enciende el escaneo del LiDAR
        return laser_.turnOn();
    }

    void turnOff() {
        // Apaga el LiDAR
        laser_.turnOff();
    }

    bool doProcessSimple(LaserScan &scan) override {
        // Implementa la función que procesa el escaneo de LiDAR
        return laser_.doProcessSimple(scan);
    }

private:
    CYdLidar laser_; // Objeto del LiDAR real desde el SDK
};
