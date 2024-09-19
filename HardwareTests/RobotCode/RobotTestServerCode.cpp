#include "CYdLidar.h"
#include <opencv2/opencv.hpp>
#include <iostream>
#include <cstdlib>
#include <cstdio>
#include <pigpio.h>
#include <string>
#include <map>
#include <vector>
#include <atomic>
#include <thread>
#include <random>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <cstring>
#include <sstream>  // Para std::ostringstream

using namespace std;
using namespace ydlidar;

// Pines y configuración para los motores
const int PWM_PINS[] = {13, 19, 18, 12};  // Pines PWM para los motores
const int DIR_PINS[] = {5, 6, 23, 24};    // Pines de dirección para los motores
int frequency = 400; // Frecuencia inicial

std::atomic<bool> is_running(true);
std::atomic<bool> is_manual_mode(true);

// Socket paths
const char* lidar_socket_path = "/tmp/unixSocket8";  // For LiDAR data
const char* control_socket_path = "/tmp/unixSocket10";  // For control commands

// Function to send data to the server
void sendDataToServer(int sock, const std::string &data) {
    if (send(sock, data.c_str(), data.length(), 0) < 0) {
        std::cerr << "Failed to send data. Error: " << strerror(errno) << std::endl;
    } else {
        std::cout << "Data sent to server: " << data << std::endl;
    }
}

// Function to format LiDAR data as a JSON-like string
std::string formatLidarDataManually(const LaserScan& scan) {
    std::ostringstream ss;
    ss << "{\"points\":[";

    for (size_t i = 0; i < scan.points.size(); ++i) {
        float x = scan.points[i].range * cos(scan.points[i].angle);
        float y = scan.points[i].range * sin(scan.points[i].angle);
        ss << "{\"x\":" << x << ",\"y\":" << y << "}";

        if (i < scan.points.size() - 1) {
            ss << ",";
        }
    }

    ss << "]}";
    return ss.str();
}

void setMotorSpeed(int motor, int frequency) {
    if (motor >= 0 && motor < 4) {
        gpioSetPWMfrequency(PWM_PINS[motor], frequency);
    }
}

void setMotorDirection(int motor, int direction) {
    if (motor >= 0 && motor < 4) {
        gpioWrite(DIR_PINS[motor], direction);
    }
}

void stopMotors() {
    for (int i = 0; i < 4; ++i) {
        gpioPWM(PWM_PINS[i], 0);
    }
}

void moveForward() {
    for (int i = 0; i < 4; ++i) {
        gpioPWM(PWM_PINS[i], 128);
    }
    setMotorDirection(0, 0);
    setMotorDirection(2, 0);
    setMotorDirection(1, 1);
    setMotorDirection(3, 1);
}

void moveBackward() {
    for (int i = 0; i < 4; ++i) {
        gpioPWM(PWM_PINS[i], 128);
    }
    setMotorDirection(0, 1);
    setMotorDirection(2, 1);
    setMotorDirection(1, 0);
    setMotorDirection(3, 0);
}

void turnLeft() {
    for (int i = 0; i < 4; ++i) {
        gpioPWM(PWM_PINS[i], 128);
    }
    setMotorDirection(0, 1);
    setMotorDirection(2, 1);
    setMotorDirection(1, 1);
    setMotorDirection(3, 1);
}

void turnRight() {
    for (int i = 0; i < 4; ++i) {
        gpioPWM(PWM_PINS[i], 128);
    }
    setMotorDirection(0, 0);
    setMotorDirection(2, 0);
    setMotorDirection(1, 0);
    setMotorDirection(3, 0);
}

void increaseSpeed() {
    if (frequency < 2000) {
        frequency += 100;
        if (frequency > 2000) frequency = 2000;
        for (int i = 0; i < 4; ++i) {
            setMotorSpeed(i, frequency);
        }
        std::cout << "Increased speed to " << frequency << " Hz" << std::endl;
    }
}

void decreaseSpeed() {
    if (frequency > 400) {
        frequency -= 100;
        if (frequency < 400) frequency = 400;
        for (int i = 0; i < 4; ++i) {
            setMotorSpeed(i, frequency);
        }
        std::cout << "Decreased speed to " << frequency << " Hz" << std::endl;
    }
}

void controlMovementFromSocket() {
    int sock = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sock < 0) {
        std::cerr << "Failed to create socket for control commands." << std::endl;
        return;
    }

    struct sockaddr_un addr;
    std::memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    std::strncpy(addr.sun_path, control_socket_path, sizeof(addr.sun_path) - 1);

    if (connect(sock, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        std::cerr << "Failed to connect to the control socket." << std::endl;
        close(sock);
        return;
    }

    char buffer[256];

    while (is_running) {
        int bytes_received = recv(sock, buffer, sizeof(buffer) - 1, 0);
        if (bytes_received > 0) {
            buffer[bytes_received] = '\0';
            std::string command(buffer);
            std::cout << "Received command: " << command << std::endl;

            if (command == "w") {
                is_manual_mode = true;
                moveForward();
            } else if (command == "s") {
                is_manual_mode = true;
                moveBackward();
            } else if (command == "a") {
                is_manual_mode = true;
                turnLeft();
            } else if (command == "d") {
                is_manual_mode = true;
                turnRight();
            } else if (command == "v") {
                is_manual_mode = true;
                increaseSpeed();
            } else if (command == "b") {
                is_manual_mode = true;
                decreaseSpeed();
            } else if (command == "space") {
                is_manual_mode = true;
                stopMotors();
            } else if (command == "k") {
                is_manual_mode = false;
            } else if (command == "m") {
                is_manual_mode = true;
            }
        }
    }

    close(sock);
}

void randomMovement(CYdLidar &laser) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::discrete_distribution<> dist({1, 0, 10, 70});

    while (is_running) {
        if (!is_manual_mode) {
            LaserScan scan;
            if (laser.doProcessSimple(scan)) {
                bool obstacle_detected = false;

                for (const auto &point : scan.points) {
                    float adjusted_angle = point.angle + M_PI;
                    if (point.range > 0 && point.range < 0.40 && abs(point.angle) < adjusted_angle) {
                        obstacle_detected = true;
                        std::cout << "Obstacle distance: " << point.range << " Y en el angulo " << point.angle << std::endl;
                        break;
                    }
                }

                if (obstacle_detected) {
                    stopMotors();
                    int turn = dist(gen) % 2;
                    if (turn == 0) {
                        turnLeft();
                        std::this_thread::sleep_for(std::chrono::seconds(5));
                    } else {
                        turnRight();
                        std::this_thread::sleep_for(std::chrono::seconds(5));
                    }
                    std::this_thread::sleep_for(std::chrono::seconds(2));
                    stopMotors();
                } else {
                    moveForward();
                }
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

void lidarDataProcessing(CYdLidar &laser) {
    int sock = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sock < 0) {
        std::cerr << "Failed to create socket for LiDAR data." << std::endl;
        return;
    }

    struct sockaddr_un addr;
    std::memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    std::strncpy(addr.sun_path, lidar_socket_path, sizeof(addr.sun_path) - 1);

    if (connect(sock, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        std::cerr << "Failed to connect to the LiDAR socket." << std::endl;
        close(sock);
        return;
    }

    while (is_running) {
        LaserScan scan;
        if (laser.doProcessSimple(scan)) {
            std::string lidarData = formatLidarDataManually(scan);

            if (!lidarData.empty()) {
                sendDataToServer(sock, lidarData);
            }
        } else {
            std::cerr << "Failed to get LiDAR data." << std::endl;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    close(sock);
}

void cameraStreaming(const std::string& rtmp_url) {
    std::string command = "libcamera-vid -t 0 --inline --nopreview -o - | ffmpeg -i - -vcodec libx264 -preset veryfast -tune zerolatency -f flv " + rtmp_url;
    std::cout << "Starting camera stream with command: " << command << std::endl;
    system(command.c_str());
}

int main() {
    std::string rtmp_url = "rtmp://<ingest-endpoint>/app/<stream-key>"; // Replace with the actual RTMP URL

    // Start the camera streaming in a separate thread
    std::thread cameraThread(cameraStreaming, rtmp_url);

    if (gpioInitialise() < 0) {
        std::cerr << "Error: No se pudo inicializar pigpio." << std::endl;
        return -1;
    }

    for (int i = 0; i < 4; ++i) {
        gpioSetMode(DIR_PINS[i], PI_OUTPUT);
        gpioSetMode(PWM_PINS[i], PI_OUTPUT);
        setMotorSpeed(i, frequency);
    }

    std::thread controlThread(controlMovementFromSocket);

    CYdLidar laser;
    std::string port;
    ydlidar::os_init();
    std::map<std::string, std::string> ports = ydlidar::lidarPortList();
    if (ports.size() > 0) {
        port = ports.begin()->second;
    } else {
        std::cerr << "No se detectó ningún LiDAR. Verifica la conexión." << std::endl;
        return -1;
    }

    int baudrate = 115200;
    laser.setlidaropt(LidarPropSerialPort, port.c_str(), port.size());
    laser.setlidaropt(LidarPropSerialBaudrate, &baudrate, sizeof(int));
    float max_range = 8.0f;
    float min_range = 0.1f;
    float max_angle = 180.0f;
    float min_angle = -180.0f;
    float frequency = 8.0f;
    laser.setlidaropt(LidarPropMaxRange, &max_range, sizeof(float));
    laser.setlidaropt(LidarPropMinRange, &min_range, sizeof(float));
    laser.setlidaropt(LidarPropMaxAngle, &max_angle, sizeof(float));
    laser.setlidaropt(LidarPropMinAngle, &min_angle, sizeof(float));
    laser.setlidaropt(LidarPropScanFrequency, &frequency, sizeof(float));

    if (!laser.initialize()) {
        std::cerr << "Error al inicializar el LiDAR." << std::endl;
        return -1;
    }

    if (!laser.turnOn()) {
        std::cerr << "Error al encender el LiDAR." << std::endl;
        return -1;
    }

    std::thread lidarThread(lidarDataProcessing, std::ref(laser));
    std::thread randomMoveThread(randomMovement, std::ref(laser));

    controlThread.join();
    lidarThread.join();

    laser.turnOff();
    laser.disconnecting();
    is_running = false;
    randomMoveThread.join();
    cameraThread.detach();  // Detach the camera thread to avoid blocking on join

    stopMotors();
    gpioTerminate();

    return 0;
}
