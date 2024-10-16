#include "motor_control.h"
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "mock_pigpio.h"  // Incluimos el mock de pigpio

// Usamos Google Mock para crear una instancia del mock de pigpio
MockPigpio mock_pigpio;

// Clase de prueba para controlar los motores
class MotorControlTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Simulamos que gpioInitialise se ejecuta correctamente
        ON_CALL(mock_pigpio, gpioInitialise()).WillByDefault(testing::Return(0));
        //gpioInitialise();  // Llamada simulada
    }

    void TearDown() override {
        // Simulamos que gpioTerminate se ejecuta sin error
        ON_CALL(mock_pigpio, gpioTerminate()).WillByDefault(testing::Return());
        //gpioTerminate();  // Llamada simulada
    }
};

// Test para verificar si los motores pueden detenerse correctamente
TEST_F(MotorControlTest, StopMotorsTest) {
    // Simulamos que gpioPWM se ejecuta correctamente para todos los pines
    ON_CALL(mock_pigpio, gpioPWM(testing::_, testing::_)).WillByDefault(testing::Return(0));
    stopMotors();  // Llamamos la función que estamos probando
}

// Test para verificar el movimiento hacia adelante
TEST_F(MotorControlTest, MoveForwardTest) {
    // Simulamos que gpioPWM y gpioWrite se ejecutan correctamente
    ON_CALL(mock_pigpio, gpioPWM(testing::_, testing::_)).WillByDefault(testing::Return(0));
    ON_CALL(mock_pigpio, gpioWrite(testing::_, testing::_)).WillByDefault(testing::Return(0));
    moveForward();  // Llamamos la función que estamos probando
}

// Test para verificar el movimiento hacia atrás
TEST_F(MotorControlTest, MoveBackwardTest) {
    ON_CALL(mock_pigpio, gpioPWM(testing::_, testing::_)).WillByDefault(testing::Return(0));
    ON_CALL(mock_pigpio, gpioWrite(testing::_, testing::_)).WillByDefault(testing::Return(0));
    moveBackward();  // Llamamos la función que estamos probando
}

// Test para verificar si se puede aumentar la velocidad
TEST_F(MotorControlTest, IncreaseSpeedTest) {
    ON_CALL(mock_pigpio, gpioSetPWMfrequency(testing::_, testing::_)).WillByDefault(testing::Return(0));
    increaseSpeed();  // Llamamos la función que estamos probando
}

// Test para verificar si se puede disminuir la velocidad
TEST_F(MotorControlTest, DecreaseSpeedTest) {
    ON_CALL(mock_pigpio, gpioSetPWMfrequency(testing::_, testing::_)).WillByDefault(testing::Return(0));
    decreaseSpeed();  // Llamamos la función que estamos probando
}

// Test para verificar si el robot puede girar a la derecha
TEST_F(MotorControlTest, TurnRightTest) {
    ON_CALL(mock_pigpio, gpioPWM(testing::_, testing::_)).WillByDefault(testing::Return(0));
    ON_CALL(mock_pigpio, gpioWrite(testing::_, testing::_)).WillByDefault(testing::Return(0));
    turnRight();  // Llamamos la función que estamos probando
}

// Test para verificar si el robot puede girar a la izquierda
TEST_F(MotorControlTest, TurnLeftTest) {
    ON_CALL(mock_pigpio, gpioPWM(testing::_, testing::_)).WillByDefault(testing::Return(0));
    ON_CALL(mock_pigpio, gpioWrite(testing::_, testing::_)).WillByDefault(testing::Return(0));
    turnLeft();  // Llamamos la función que estamos probando
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
