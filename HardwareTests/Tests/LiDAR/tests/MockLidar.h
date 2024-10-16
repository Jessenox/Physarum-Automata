#ifndef MOCKLIDAR_H
#define MOCKLIDAR_H

#include "gmock/gmock.h"
#include "ILidar.h"

class MockLidar : public ILidar {
public:
    MOCK_METHOD(bool, initialize, (), (override));
    MOCK_METHOD(bool, turnOn, (), (override));
    MOCK_METHOD(bool, doProcessSimple, (LaserScan &scan), (override));
    MOCK_METHOD(void, turnOff, (), (override));
};

#endif // MOCKLIDAR_H
