#include "CYdLidar.h"
#include <SFML/Graphics.hpp>
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
#include <pthread.h>
#include <vector>
#include <mutex>
#define PI 180.f

using namespace std;
using namespace ydlidar;

// Pines y configuracin para los motores
const int PWM_PINS[] = {13, 19, 18, 12};  // Pines PWM para los motores
const int DIR_PINS[] = {5, 6, 23, 24};    // Pines de direccin para los motores
const int SENSOR_PROFUNIDAD_PINS[] = {2, 3};
int frequency = 400; // Frecuencia inicial
const sf::Vector2f robotFixedPosition(110, 110);
std::atomic<bool> is_running(true);
std::atomic<bool> is_manual_mode(true);

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
        gpioPWM(PWM_PINS[i], 128);  // Establecer ciclo de trabajo al 50%
    }
    setMotorDirection(0, 0); // Motor 1
    setMotorDirection(2, 0); // Motor 3
    setMotorDirection(1, 1); // Motor 2
    setMotorDirection(3, 1); // Motor 4
}

void moveBackward() {
    for (int i = 0; i < 4; ++i) {
        gpioPWM(PWM_PINS[i], 128);  // Establecer ciclo de trabajo al 50%
    }
    setMotorDirection(0, 1); // Motor 1
    setMotorDirection(2, 1); // Motor 3
    setMotorDirection(1, 0); // Motor 2
    setMotorDirection(3, 0); // Motor 4
}

void turnLeft() {
    for (int i = 0; i < 4; ++i) {
        gpioPWM(PWM_PINS[i], 128);  // Establecer ciclo de trabajo al 50%
    }
    setMotorDirection(0, 1); // Motor 1
    setMotorDirection(2, 1); // Motor 3
    setMotorDirection(1, 1); // Motor 2
    setMotorDirection(3, 1); // Motor 4
}

void turnRight() {
    for (int i = 0; i < 4; ++i) {
        gpioPWM(PWM_PINS[i], 128);  // Establecer ciclo de trabajo al 50%
    }
    setMotorDirection(0, 0); // Motor 1
    setMotorDirection(2, 0); // Motor 3
    setMotorDirection(1, 0); // Motor 2
    setMotorDirection(3, 0); // Motor 4
}

void increaseSpeed() {
    if (frequency < 2000) {
        frequency += 100;
        if (frequency > 2000) frequency = 2000;
        for (int i = 0; i < 4; ++i) {
            setMotorSpeed(i, frequency);
        }
    }
}

void decreaseSpeed() {
    if (frequency > 400) {
        frequency -= 100;
        if (frequency < 400) frequency = 400;
        for (int i = 0; i < 4; ++i) {
            setMotorSpeed(i, frequency);
        }
    }
}


sf::Color getPointColor(float distance, float maxRange) {
    float ratio = distance / maxRange;
    return sf::Color(255 * (1 - ratio), 255 * ratio, 0); // Color de rojo a verde
}


std::vector<sf::Vector2f> routePoints;
bool isDrawing = false;  // Bandera para rastrear si el raton esta presionado




void drawLiDARPoints(sf::RenderWindow &window, const std::vector<LaserPoint> &lidarPoints, const sf::Vector2f &robotPosition, float scale) {
    for (const auto& point : lidarPoints) {
        // Convertir las coordenadas polares del LiDAR a coordenadas cartesianas
        float x = point.range * cos(point.angle);
        float y = point.range * sin(point.angle);

        // Ajustar la posición de los puntos del LiDAR en función de la posición del robot
        float adjustedX = robotFixedPosition.x + (x * scale) - robotPosition.x;
        float adjustedY = robotFixedPosition.y - (y * scale) - robotPosition.y;  // Invertir Y para la pantalla

        // Dibujar los puntos del LiDAR
        if (point.range > 0.05) {  // Excluir puntos cercanos al centro
            sf::CircleShape lidarPoint(2);  // Tamaño del punto
            lidarPoint.setPosition(adjustedX, adjustedY);
            lidarPoint.setFillColor(getPointColor(point.range, max_range));  // Color basado en la distancia

            window.draw(lidarPoint);
        }
    }
}





void handleMouseClick(const sf::Event &event, const sf::RectangleShape &minimap) {
    // Verificar si el raton esta presionado
    if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
        isDrawing = true;  // Marcar que estamos dibujando

    // Verificar si el raton fue soltado
    } else if (event.type == sf::Event::MouseButtonReleased && event.mouseButton.button == sf::Mouse::Left) {
        isDrawing = false;  // Dejar de dibujar
    }
}

void updateRoute(const sf::RectangleShape &minimap, sf::RenderWindow &window) {
    // Si el raton esta presionado, agregar puntos a la ruta
    if (isDrawing) {
        // Obtener la posicion actual del raton
        sf::Vector2i mousePosition = sf::Mouse::getPosition(window);
        sf::Vector2f clickPosition(static_cast<float>(mousePosition.x), static_cast<float>(mousePosition.y));

        // Verificar si la posicion del raton esta dentro del minimapa
        if (minimap.getGlobalBounds().contains(clickPosition)) {
            sf::Vector2f relativePosition = clickPosition - minimap.getPosition();
            routePoints.push_back(relativePosition);
            std::cout << "Dibujando punto: (" << relativePosition.x << ", " << relativePosition.y << ")" << std::endl;
        }
    }
}



void drawRoute(sf::RenderWindow &window, const std::vector<sf::Vector2f> &routePoints, const sf::Vector2f &robotPosition) {
    if (routePoints.size() > 1) {
        for (size_t i = 0; i < routePoints.size() - 1; ++i) {
            // Dibujar las líneas de la ruta, ajustando las coordenadas en función de la posición del robot
            sf::Vertex line[] = {
                sf::Vertex(robotFixedPosition + (routePoints[i] - robotPosition), sf::Color::Red),  // Ajustar posición en función del robot
                sf::Vertex(robotFixedPosition + (routePoints[i + 1] - robotPosition), sf::Color::Red)
            };
            window.draw(line, 2, sf::Lines);
        }
    }
}





// Ajustar la referencia para que el norte sea -1.57 rad (sur en coordenadas cartesianas es 0 rad)
float adjustAngleForNorth(float angle) {
    float adjustedAngle = angle - (-M_PI / 2);  // Ajuste de 90 (norte en -90)
    
    // Asegurarse de que el angulo este dentro de los limites de -PI a PI
    if (adjustedAngle > M_PI) {
        adjustedAngle -= 2 * M_PI;
    } else if (adjustedAngle < -M_PI) {
        adjustedAngle += 2 * M_PI;
    }
    
    return adjustedAngle;
}







void rotateTowardsPoint(float currentAngle, float targetAngle) {
    float angleDifference = targetAngle - currentAngle;

    // Asegurate de que el angulo este entre -PI y PI
    if (angleDifference > M_PI) {
        angleDifference -= 2 * M_PI;
    } else if (angleDifference < -M_PI) {
        angleDifference += 2 * M_PI;
    }

    // Gira en la direccion correcta
    if (angleDifference > 0) {
        turnRight();  // Girar hacia la derecha
    } else {
        turnLeft();  // Girar hacia la izquierda
    }

    // Esperar hasta que el robot este alineado
    std::this_thread::sleep_for(std::chrono::milliseconds(static_cast<int>(std::fabs(angleDifference) * 1000)));
    
    stopMotors();
}






void moveTowardsPoint(float distance) {
    moveForward();  // Mover el robot hacia adelante

    // Esperar un tiempo proporcional a la distancia
    std::this_thread::sleep_for(std::chrono::milliseconds(static_cast<int>(distance * 1000)));
    
    stopMotors();  // Detener el robot
}




void moveTo(sf::Vector2f point, sf::Vector2f &robotPosition, float &currentAngle) {
    // Calcular la diferencia en X e Y
    float deltaX = point.x - robotPosition.x;
    float deltaY = robotPosition.y - point.y;  // Invertir la Y para la pantalla

    // Calcular la distancia al punto
    float distanceToPoint = sqrt(deltaX * deltaX + deltaY * deltaY);

    // Si la distancia es mayor a un umbral, mover el robot
    const float threshold = 5.0f;  // Tolerancia para considerar que el punto ha sido alcanzado
    if (distanceToPoint > threshold) {
        // Calcular el angulo hacia el punto de destino
        float targetAngle = atan2(deltaY, deltaX);

        // Ajustar el angulo para que el norte sea -90°
        targetAngle = adjustAngleForNorth(targetAngle);

        // Girar hacia el angulo correcto
        rotateTowardsPoint(currentAngle, targetAngle);  // Función para girar hacia el ángulo

        // Mover hacia el punto
        moveTowardsPoint(distanceToPoint);
        
        // Actualizar la posicion y el ángulo del robot
        robotPosition = point;
        currentAngle = targetAngle;
    }
}




void followRoute(sf::RenderWindow &window, std::vector<sf::Vector2f> &routePoints, const std::vector<LaserPoint> &lidarPoints) {
    sf::Vector2f robotPosition(110, 110);  // Posición inicial del robot en el minimapa
    float currentAngle = -M_PI / 2;  // Ángulo inicial del robot (norte en -90°)

    // Seguir la ruta mientras haya puntos
    while (!routePoints.empty()) {
        sf::Vector2f targetPoint = routePoints.front();  // Obtener el primer punto de la ruta

        // Mover al robot al primer punto
        moveTo(targetPoint, robotPosition, currentAngle);

        // Eliminar el punto de la ruta una vez alcanzado
        routePoints.erase(routePoints.begin());

        // Actualizar la posición del robot al punto alcanzado
        robotPosition = targetPoint;

        // Mover el entorno en función de la nueva posición del robot
        drawRoute(window, routePoints, robotPosition);  // Dibujar la ruta ajustada
        // Asumiremos que lidarPoints es global o será pasada como parámetro
        drawLiDARPoints(window, lidarPoints, robotPosition, 25.0f);  // Dibujar los puntos del LiDAR ajustados
    }
}









int contadorObstaculoTotal = 0;
int obstaculoHola = 0;
int obsRelativo = 0;
int idTipoObstaculo = 0;
void randomMovement(CYdLidar &laser) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::discrete_distribution<> dist({0, 5}); // Distribucin para la probabilidad de movimiento

    const float FRONT_MIN_ANGLE = -10.0f * (M_PI / 180.0f); // -15 grados en radaianes
    const float FRONT_MAX_ANGLE = 10.0f * (M_PI / 180.0f);  // 15 grados en radianes
    const float DETECTION_RADIUS = 0.25f; // 35 cm

    std::vector<float> previousScanPoints;

    while (is_running) {
        if (!is_manual_mode) {
            LaserScan scan;
            if (laser.doProcessSimple(scan)) {
                bool obstacle_detected = false;
                std::vector<float> currentScanPoints;
                
                for (const auto &point : scan.points) {
                    // Convertir el ngulo del punto al ngulo relativo al "sur" del robot
                    float adjusted_angle = point.angle + PI;
                    //std::cout << "Er " << point.range << std::endl;
                    // Verificar si el punto est dentro del rango frontal de 30
                    //std::cout << point.angle << std::endl;
                    if (point.range > 0 && point.range < 0.30 && ( point.angle <= -0.5235f  && point.angle >= -2.617f)) { // Norte
                        obstacle_detected = true;
                        currentScanPoints.push_back(point.range);
                        idTipoObstaculo = 1;
                        std::cout << "Obstacle distance N: " << (float)point.range  << " Y en el angulo  "<<point.angle << std::endl;
                        break;
                    } else if(point.range > 0 && point.range < 0.25 && (point.angle <= 0.872f  && point.angle>= -0.5235f)){ // Este
                        obstacle_detected = true;
                        currentScanPoints.push_back(point.range);
                        idTipoObstaculo = 2;
                        std::cout << "Obstacle distance E: " << (float)point.range  << " Y en el angulo  "<<point.angle << std::endl;
                        break;
                    }else if(point.range > 0 && point.range < 0.45 && (point.angle <= 2.26f && point.angle >= 0.872f)){ // Sur
                        obstacle_detected = true;
                        currentScanPoints.push_back(point.range);
                        idTipoObstaculo = 3;
                        std::cout << "Obstacle distance S:" << (float)point.range  << " Y en el angulo  "<<point.angle << std::endl;
                        break;
                    }else if (point.range > 0 && point.range < 0.25 && (point.angle <= -2.61f  || point.angle >= 2.27f)){ // Oeste
                        obstacle_detected = true;
                        currentScanPoints.push_back(point.range);
                        idTipoObstaculo = 4;
                        std::cout << "Obstacle distance O:" << (float)point.range  << " Y en el angulo  "<<point.angle << std::endl;
                        break;
                    }
                    
                    
                }
                
                if (obstacle_detected) {
                    // Si detecta un obstculo, retrocede por 4 segundos
                    contadorObstaculoTotal++;
                    std::cout << "Obs: " << contadorObstaculoTotal << std::endl;
                    //obsRelativo++;
                    stopMotors();
                    //Luego gira aleatoriamente a la izquierda o derecha
                    switch(idTipoObstaculo) {
                        case 1: {
                            int turn = dist(gen) % 2;
                            moveBackward();
                            std::this_thread::sleep_for(std::chrono::seconds(5));
                            if (turn == 0) {
                                turnLeft();
                                std::this_thread::sleep_for(std::chrono::seconds(5));
                            } else {
                                turnRight();
                                std::this_thread::sleep_for(std::chrono::seconds(5));
                            }
                            std::this_thread::sleep_for(std::chrono::seconds(2));
                            stopMotors();
                        }
                        break;
                        
                        case 2:
                            turnLeft();
                            std::this_thread::sleep_for(std::chrono::seconds(5));
                            std::this_thread::sleep_for(std::chrono::seconds(2));
                            stopMotors();
                        break;
                        
                        case 3:// este es el 4 y el de abajo el 3
                            moveForward();
                            std::this_thread::sleep_for(std::chrono::seconds(2));
                            stopMotors();
                            
                        break;
                        
                        case 4:
                            turnRight();
                            std::this_thread::sleep_for(std::chrono::seconds(7));
                            stopMotors();
                        break;
                        
                        default: {
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
                        }
                        break;
                    }

                    
                    if(!previousScanPoints.empty() && previousScanPoints.size() == currentScanPoints.size()){
                            bool mismoObs = true;
                            for(size_t i = 0; i < currentScanPoints.size(); ++i){
                                if(fabs(currentScanPoints[i] - previousScanPoints[i]) > 0.05){
                                        mismoObs = false;
                                        break;
                                }
                            }
                            if(mismoObs){
                                obsRelativo++;
                            }else{
                                obsRelativo = 0;
                            }
                    }
                    
                    previousScanPoints = currentScanPoints;
                    
                    if((obsRelativo > 3)){
                        //obsRelativo = 0;
                        moveBackward();
                        std::this_thread::sleep_for(std::chrono::seconds(3));
                        stopMotors();
                        
                        
                        switch(idTipoObstaculo) {
                            case 1: {
                                int turn = dist(gen) % 2;
                                moveBackward();
                                std::this_thread::sleep_for(std::chrono::seconds(5));
                                if (turn == 0) {
                                    turnLeft();
                                    std::this_thread::sleep_for(std::chrono::seconds(5));
                                } else {
                                    turnRight();
                                    std::this_thread::sleep_for(std::chrono::seconds(5));
                                }
                                std::this_thread::sleep_for(std::chrono::seconds(2));
                                stopMotors();
                            }
                            break;
                            
                            case 2:
                                turnLeft();
                                std::this_thread::sleep_for(std::chrono::seconds(5));
                                std::this_thread::sleep_for(std::chrono::seconds(2));
                                stopMotors();
                            break;
                            
                            case 3:
                                turnRight();
                                std::this_thread::sleep_for(std::chrono::seconds(7));
                                stopMotors();
                            break;
                            
                            case 4:
                                moveForward();
                                std::this_thread::sleep_for(std::chrono::seconds(2));
                                stopMotors();
                            break;
                            
                            default: {
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
                            }
                            break;
                        }

                        
                        
                    }
                } else {
                    moveForward();
                    // Si no hay obstculo, elige un movimiento aleatorio
                    //
                    //int move = dist(gen);
                    //switch (move) {
                      //  case 0: moveBackward(); break;
                      //  case 1: turnRight(); break;
                      //  case 2: turnLeft(); break;
                      //  case 3: moveForward(); break;
                    //}
                }
            }
            
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
            //stopMotors();
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}






int main() {
    // Inicializar pigpio
    if (gpioInitialise() < 0) {
        std::cerr << "Error: No se pudo inicializar pigpio." << std::endl;
        return -1;
    }

    // Configurar pines de direccin como salida
    for (int i = 0; i < 4; ++i) {
        gpioSetMode(DIR_PINS[i], PI_OUTPUT);
        gpioSetMode(PWM_PINS[i], PI_OUTPUT);
        setMotorSpeed(i, frequency);  // Inicializar PWM con frecuencia inicial
    }

    // Asegrate de establecer XDG_RUNTIME_DIR
    if (getenv("XDG_RUNTIME_DIR") == nullptr) {
        setenv("XDG_RUNTIME_DIR", "/tmp/runtime-$(id -u)", 1);
    }

    // Ejecuta libcamera-vid en un proceso separado y captura la salida en YUV, sin previsualizacn
    FILE* pipe = popen("libcamera-vid -t 0 --codec yuv420 --nopreview -o -", "r");
    if (!pipe) {
        std::cerr << "Error: No se pudo ejecutar libcamera-vid." << std::endl;
        return -1;
    }

    // Configura la ventana SFML
    sf::RenderWindow window(sf::VideoMode(1280, 720), "Camera Visualization with LiDAR");
    sf::Texture cameraTexture;
    sf::Sprite cameraSprite;

    // Buffer para leer los datos de video
    const int width = 640;
    const int height = 480;
    std::vector<uint8_t> buffer(width * height * 3 / 2); // Ajusta el tamao del buffer para YUV420

    cv::Mat yuvImage(height + height / 2, width, CV_8UC1, buffer.data());
    cv::Mat rgbImage(height, width, CV_8UC3);

    std::string port;
    ydlidar::os_init();

    // Obtener los puertos disponibles de LiDAR
    std::map<std::string, std::string> ports = ydlidar::lidarPortList();
    if (ports.size() > 1) {
        auto it = ports.begin();
        std::advance(it, 1); // Selecciona el segundo puerto disponible
        port = it->second;
    } else if (ports.size() == 1) {
        port = ports.begin()->second;
    } else {
        std::cerr << "No se detect ningn LiDAR. Verifica la conexin." << std::endl;
        return -1;
    }

    // Configuracin del LiDAR
    int baudrate = 115200;
    std::cout << "Baudrate: " << baudrate << std::endl;

    CYdLidar laser;
    laser.setlidaropt(LidarPropSerialPort, port.c_str(), port.size());
    laser.setlidaropt(LidarPropSerialBaudrate, &baudrate, sizeof(int));

    bool isSingleChannel = true;
    laser.setlidaropt(LidarPropSingleChannel, &isSingleChannel, sizeof(bool));

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

    // Inicializar LiDAR
    if (!laser.initialize()) {
        std::cerr << "Error al inicializar el LiDAR." << std::endl;
        return -1;
    }

    // Iniciar el escaneo
    if (!laser.turnOn()) {
        std::cerr << "Error al encender el LiDAR." << std::endl;
        return -1;
    }

    // Thread para movimiento aleatorio
    std::thread randomMoveThread(randomMovement, std::ref(laser));

    while (window.isOpen()) {
        // Crear un minimapa para el LiDAR
        sf::RectangleShape minimap(sf::Vector2f(200, 200));
        minimap.setFillColor(sf::Color(200, 200, 200, 150)); // Fondo semitransparente
        minimap.setPosition(10, 10); // Esquina superior izquierda
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed){
                window.close();
            }

            if(event.type == sf::Event::MouseButtonPressed){
                std::cout << "Mouse pressed" << std::endl;
                handleMouseClick(event, minimap);
            }else if(event.type == sf::Event::MouseButtonReleased){
                std::cout << "Mouse released" << std::endl;
                handleMouseClick(event, minimap);
                isDrawing = false;
            }
            



            if (event.type == sf::Event::KeyPressed) {
                switch (event.key.code) {
                    case sf::Keyboard::W:
                        is_manual_mode = true;
                        std::cout << "W" << std::endl;
                        moveForward();
                        break;
                    case sf::Keyboard::S:
                        is_manual_mode = true;
                        std::cout << "S" << std::endl;
                        moveBackward();
                        break;
                    case sf::Keyboard::A:
                        is_manual_mode = true;
                        std::cout << "A" << std::endl;
                        turnLeft();
                        break;
                    case sf::Keyboard::D:
                        is_manual_mode = true;
                        std::cout << "D" << std::endl;
                        turnRight();
                        break;
                    case sf::Keyboard::V:
                        is_manual_mode = true;
                        std::cout << "V" << std::endl;
                        increaseSpeed();
                        break;
                    case sf::Keyboard::B:
                        is_manual_mode = true;
                        std::cout << "B" << std::endl;
                        decreaseSpeed();
                        break;
                    case sf::Keyboard::Space:
                        is_manual_mode = true;
                        std::cout << "Space" << std::endl;
                        stopMotors();
                        break;
                    case sf::Keyboard::R: {
                        is_manual_mode = false;
                        std::thread routeThread(followRoute, std::ref(window), std::ref(routePoints), std::ref(lidarPoints));
                        routeThread.detach();
                        break;
                    }
                    case sf::Keyboard::K:
                        is_manual_mode = false;
                        std::cout << "K" << std::endl;
                        break;
                    case sf::Keyboard::M:
                        is_manual_mode = true;
                        std::cout << "M" << std::endl;
                        break;
                    default:
                        break;
                }
            }
        }

        // Leer los datos del video desde la tubera
        size_t bytesRead = fread(buffer.data(), 1, buffer.size(), pipe);
        if (bytesRead != buffer.size()) {
            std::cerr << "Error: No se pudo leer suficientes datos de video." << std::endl;
            continue;
        }

        // Convertir YUV420 a RGB
        cv::cvtColor(yuvImage, rgbImage, cv::COLOR_YUV2RGB_I420);

        // Convertir a RGBA aadiendo un canal alfa
        cv::Mat frame_rgba;
        cv::cvtColor(rgbImage, frame_rgba, cv::COLOR_RGB2RGBA);

        // Actualizar la textura de la cmara con los datos del frame
        if (!cameraTexture.create(frame_rgba.cols, frame_rgba.rows)) {
            std::cerr << "Error: No se pudo crear la textura." << std::endl;
            continue;
        }
        cameraTexture.update(frame_rgba.ptr());

        cameraSprite.setTexture(cameraTexture);
        cameraSprite.setScale(
            window.getSize().x / static_cast<float>(cameraTexture.getSize().x),
            window.getSize().y / static_cast<float>(cameraTexture.getSize().y)
        );


        // Actualizar la ruta en el minimapa
        
        window.clear();
        window.draw(cameraSprite);

        
        window.draw(minimap);
        updateRoute(minimap, window);
        // Dibujar la ruta en el minimapa
        drawRoute(window, routePoints, minimap);
        // Dibujar el centro del LiDAR (color azul)
        sf::CircleShape lidarCenter(5); // Radio del crculo del LiDAR
        lidarCenter.setFillColor(sf::Color::Blue);
        lidarCenter.setPosition(105, 105); // Posicin del centro en el minimapa

        window.draw(lidarCenter);

        // Dibujar la lnea hacia el norte
        sf::Vertex line[] =
        {
            sf::Vertex(sf::Vector2f(110, 110), sf::Color::Black),
            sf::Vertex(sf::Vector2f(110, 160), sf::Color::Black) // Lnea hacia arriba (norte)
        };

        window.draw(line, 2, sf::Lines);

        LaserScan scan;
        if (laser.doProcessSimple(scan)) {
            drawLiDARPoints(window, scan.points, sf::Vector2f(110, 110), 25.0f);
        } else {
            std::cerr << "No se pudieron obtener los datos del LiDAR." << std::endl;
        }

        window.display();
    }

    // Detener el escaneo del LiDAR
    laser.turnOff();
    laser.disconnecting();

    // Cierra la tubera, detiene los motores y apaga el robot
    pclose(pipe);
    stopMotors();
    gpioTerminate();

    is_running = false;
    randomMoveThread.join();

    return 0;
}