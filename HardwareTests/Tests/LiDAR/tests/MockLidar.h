#ifndef MOCKLIDAR_H
#define MOCKLIDAR_H

#include "gmock/gmock.h"
#include "ILidar.h"

class MockLidar : public CYdLidar {
public:
    MOCK_METHOD(bool, initialize, (), (override));  // Asegúrate de que initialize() existe en la clase CYdLidar
    MOCK_METHOD(bool, turnOn, (), (override));      // Asegúrate de que turnOn() existe en la clase CYdLidar
    MOCK_METHOD(bool, doProcessSimple, (LaserScan& scan), (override));  // Asegúrate de que doProcessSimple existe y la firma es correcta
};


#endif // MOCKLIDAR_H
