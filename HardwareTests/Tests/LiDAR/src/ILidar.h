#pragma once
#include "CYdLidar.h" // Incluye el header necesario para LaserScan

class ILidar {
public:
    virtual bool initialize() = 0;
    virtual bool turnOn() = 0;
    virtual void turnOff() = 0;
    virtual bool doProcessSimple(LaserScan &scan) = 0; // Procesa el escaneo de LiDAR
    virtual ~ILidar() = default;  // Destructor virtual
};
