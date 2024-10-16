#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "motor_control.h" // Archivo donde tienes las funciones de los motores

// Mock para GPIO
class MockGPIO {
public:
    MOCK_METHOD(int, gpioSetPWMfrequency, (int pin, int frequency), ());
    MOCK_METHOD(int, gpioWrite, (int pin, int value), ());
    MOCK_METHOD(int, gpioPWM, (int pin, int dutyCycle), ());
};

// Instancia global del mock
MockGPIO* mockGPIO = nullptr;

// Sobrescribir las funciones GPIO con el mock
int gpioSetPWMfrequency(int pin, int frequency) {
    return mockGPIO->gpioSetPWMfrequency(pin, frequency);
}

int gpioWrite(int pin, int value) {
    return mockGPIO->gpioWrite(pin, value);
}

int gpioPWM(int pin, int dutyCycle) {
    return mockGPIO->gpioPWM(pin, dutyCycle);
}

// Clase de prueba
class MotorControlTest : public ::testing::Test {
protected:
    MockGPIO gpioMock;

    void SetUp() override {
        mockGPIO = &gpioMock;
    }

    void TearDown() override {
        mockGPIO = nullptr;
    }
};

// Prueba para setMotorSpeed
TEST_F(MotorControlTest, SetMotorSpeed_ValidMotor) {
    EXPECT_CALL(gpioMock, gpioSetPWMfrequency(13, 400)).Times(1);
    setMotorSpeed(0, 400);
}

TEST_F(MotorControlTest, SetMotorSpeed_InvalidMotor) {
    // No debe llamar a gpioSetPWMfrequency si el motor no es válido
    EXPECT_CALL(gpioMock, gpioSetPWMfrequency(::testing::_, ::testing::_)).Times(0);
    setMotorSpeed(5, 400);  // Motor fuera del rango válido
}

// Prueba para setMotorDirection
TEST_F(MotorControlTest, SetMotorDirection_ValidMotor) {
    EXPECT_CALL(gpioMock, gpioWrite(5, 1)).Times(1);
    setMotorDirection(0, 1);
}

TEST_F(MotorControlTest, SetMotorDirection_InvalidMotor) {
    // No debe llamar a gpioWrite si el motor no es válido
    EXPECT_CALL(gpioMock, gpioWrite(::testing::_, ::testing::_)).Times(0);
    setMotorDirection(5, 1);  // Motor fuera del rango válido
}

// Prueba para moveForward
TEST_F(MotorControlTest, MoveForward_CorrectCalls) {
    // Esperar llamadas a gpioPWM y gpioWrite para todos los motores
    EXPECT_CALL(gpioMock, gpioPWM(::testing::_, 128)).Times(4);
    EXPECT_CALL(gpioMock, gpioWrite(0, 0)).Times(1); // Motor 1 dirección
    EXPECT_CALL(gpioMock, gpioWrite(1, 1)).Times(1); // Motor 2 dirección
    EXPECT_CALL(gpioMock, gpioWrite(2, 0)).Times(1); // Motor 3 dirección
    EXPECT_CALL(gpioMock, gpioWrite(3, 1)).Times(1); // Motor 4 dirección

    moveForward();
}

// Prueba para moveBackward
TEST_F(MotorControlTest, MoveBackward_CorrectCalls) {
    // Esperar llamadas a gpioPWM y gpioWrite para todos los motores
    EXPECT_CALL(gpioMock, gpioPWM(::testing::_, 128)).Times(4);
    EXPECT_CALL(gpioMock, gpioWrite(0, 1)).Times(1); // Motor 1 dirección
    EXPECT_CALL(gpioMock, gpioWrite(1, 0)).Times(1); // Motor 2 dirección
    EXPECT_CALL(gpioMock, gpioWrite(2, 1)).Times(1); // Motor 3 dirección
    EXPECT_CALL(gpioMock, gpioWrite(3, 0)).Times(1); // Motor 4 dirección

    moveBackward();
}

// Prueba para turnLeft
TEST_F(MotorControlTest, TurnLeft_CorrectCalls) {
    // Esperar llamadas a gpioPWM y gpioWrite para todos los motores
    EXPECT_CALL(gpioMock, gpioPWM(::testing::_, 128)).Times(4);
    EXPECT_CALL(gpioMock, gpioWrite(0, 1)).Times(1); // Motor 1 dirección
    EXPECT_CALL(gpioMock, gpioWrite(1, 1)).Times(1); // Motor 2 dirección
    EXPECT_CALL(gpioMock, gpioWrite(2, 1)).Times(1); // Motor 3 dirección
    EXPECT_CALL(gpioMock, gpioWrite(3, 1)).Times(1); // Motor 4 dirección

    turnLeft();
}

// Prueba para turnRight
TEST_F(MotorControlTest, TurnRight_CorrectCalls) {
    // Esperar llamadas a gpioPWM y gpioWrite para todos los motores
    EXPECT_CALL(gpioMock, gpioPWM(::testing::_, 128)).Times(4);
    EXPECT_CALL(gpioMock, gpioWrite(0, 0)).Times(1); // Motor 1 dirección
    EXPECT_CALL(gpioMock, gpioWrite(1, 0)).Times(1); // Motor 2 dirección
    EXPECT_CALL(gpioMock, gpioWrite(2, 0)).Times(1); // Motor 3 dirección
    EXPECT_CALL(gpioMock, gpioWrite(3, 0)).Times(1); // Motor 4 dirección

    turnRight();
}

// Prueba para stopMotors
TEST_F(MotorControlTest, StopMotors_CorrectCalls) {
    // Esperar que gpioPWM sea llamado con duty cycle de 0 para todos los motores
    EXPECT_CALL(gpioMock, gpioPWM(::testing::_, 0)).Times(4);
    stopMotors();
}
