#ifndef MOCK_PIGPIO_H
#define MOCK_PIGPIO_H

#include <gmock/gmock.h>

class MockPigpio {
public:
    MOCK_METHOD(int, gpioInitialise, (), ());
    MOCK_METHOD(void, gpioTerminate, (), ());
    MOCK_METHOD(int, gpioSetPWMfrequency, (int pin, int frequency), ());
    MOCK_METHOD(int, gpioPWM, (int pin, int value), ());
    MOCK_METHOD(int, gpioWrite, (int pin, int value), ());
};

extern MockPigpio mock_pigpio;

#endif // MOCK_PIGPIO_H
