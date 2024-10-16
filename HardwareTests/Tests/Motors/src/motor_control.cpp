#include "motor_control.h"
#include "mock_pigpio.h"  // Incluimos el mock para usar las funciones simuladas

void stopMotors() {
    for (int i = 0; i < 4; ++i) {
        //gpioPWM(i, 0);  // Detener los motores
    }

}

void moveForward() {
    for (int i = 0; i < 4; ++i) {
        //gpioPWM(i, 128);  // Ciclo de trabajo al 50%
        //gpioWrite(i, 0);  // Movimiento hacia adelante
    }

}

void moveBackward() {
    for (int i = 0; i < 4; ++i) {
        //gpioPWM(i, 128);  // Ciclo de trabajo al 50%
        //gpioWrite(i, 1);  // Movimiento hacia atrás
    }

}

void increaseSpeed() {
    for (int i = 0; i < 4; ++i) {
        //gpioSetPWMfrequency(i, 500);  // Aumenta la frecuencia PWM
    }
}

void decreaseSpeed() {
    for (int i = 0; i < 4; ++i) {
        //gpioSetPWMfrequency(i, 200);  // Disminuye la frecuencia PWM
    }
 
}

void turnRight() {

}

void turnLeft() {

}
