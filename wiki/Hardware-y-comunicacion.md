# Hardware y comunicacion

El directorio `HardwareTests` agrupa pruebas experimentales para sensores, motores y servidores. No forma parte del binario principal `PhysarumVulkan`, pero documenta la linea de integracion con robotica planteada en el Trabajo Terminal.

## Idea del sistema robotico

El documento academico propone un sistema donde el simulador calcula rutas y un robot con Raspberry Pi recolecta informacion del entorno.

```mermaid
flowchart LR
    Sim[Simulador Physarum] --> Ruta[Ruta calculada]
    Ruta --> Robot[Robot Raspberry Pi]
    Robot --> Lidar[LiDAR]
    Robot --> Cam[Camara]
    Robot --> Motors[Motores]
    Robot --> Server[Servidor / monitoreo]
    Server --> User[Usuario]
```

## Arquitectura distribuida propuesta

La arquitectura descrita en LaTeX usa tres capas:

- aplicacion o interfaz de usuario;
- servidor en la nube o intermediario;
- robot fisico con sensores y control.

```mermaid
flowchart TB
    subgraph UI[Capa de usuario]
        App[Physarum App]
    end

    subgraph Cloud[Capa de procesamiento]
        HTTP[Servidor HTTP]
        WS[Servidor WebSocket]
        Physarum[Algoritmo Physarum]
    end

    subgraph Physical[Capa fisica]
        RPi[Raspberry Pi 4]
        Lidar[Sensor LiDAR]
        Camera[Camara]
        Motor[Motores Nema 23]
        Control[Control de motores]
    end

    App -->|comandos| HTTP
    HTTP --> Physarum
    Physarum -->|ruta / decision| HTTP
    HTTP -->|ordenes| RPi
    RPi --> Control
    Control --> Motor
    Lidar --> RPi
    Camera --> RPi
    RPi -->|datos LiDAR| WS
    RPi -->|multimedia| WS
    WS --> App
```

## Componentes de hardware propuestos

| Componente | Funcion |
| --- | --- |
| Raspberry Pi 4 B | Controlador principal del robot. |
| LiDAR DTOF STL27L | Deteccion de obstaculos y medicion de distancias. |
| Camara nocturna infrarroja 5MP | Vision y monitoreo remoto. |
| Motores Nema 23 | Movimiento preciso. |
| Controladores de motor a pasos | Control electrico de motores. |
| Ruedas omnidireccionales | Movimiento en multiples direcciones. |
| Baterias 12V 20000mAh | Alimentacion. |
| Convertidor Boost Buck | Regulacion de voltaje. |
| Estructura aluminio/acrilico | Soporte mecanico. |

## Areas del directorio

```text
HardwareTests/
  KinnectAndLiDAR/
  RobotCode/
  Servers/
  Tests/
```

## LiDAR y Kinect

`KinnectAndLiDAR` contiene pruebas con LiDAR, sockets y utilidades. Hay CMake, pruebas con Google Test y ejemplos de comunicacion.

`Tests/LiDAR` contiene una estructura mas orientada a pruebas:

- interfaz `ILidar`;
- implementacion `RealLidar`;
- pruebas con mocks;
- CMake con dependencias de `ydlidar_sdk`, SFML y OpenCV.

## Motores

`Tests/Motors` contiene pruebas unitarias para control de motores, incluyendo un mock de `pigpio`. Esto permite validar logica sin depender directamente del hardware real.

```mermaid
flowchart LR
    Test[Prueba unitaria] --> Mock[mock_pigpio]
    Mock --> MotorLogic[motor_control]
    MotorLogic --> Result[Validacion sin hardware real]
```

## Servidores

`Servers` contiene prototipos HTTP y WebSocket:

- `CommandServers`: servidor de comandos experimental.
- `LidarServers`: servidor WebSocket basado en Node.js y `ws`.
- `ServerDiscontinued`: implementaciones Java/UDP/TCP antiguas.

Para instalar dependencias del servidor WebSocket:

```bash
cd HardwareTests/Servers/LidarServers
npm install
```

## Flujo de control remoto

```mermaid
sequenceDiagram
    actor Usuario
    participant App as Interfaz
    participant HTTP as Servidor HTTP
    participant Robot as Raspberry Pi
    participant Motor as Motores
    participant WS as WebSocket

    Usuario->>App: Presiona movimiento
    App->>HTTP: Envia comando
    HTTP->>Robot: Reenvia orden
    Robot->>Motor: Ejecuta movimiento
    Robot->>Robot: Evalua obstaculos
    Robot-->>WS: Envia estado/sensores
    WS-->>App: Actualiza monitoreo
```

## Dependencias comunes

Las pruebas de hardware pueden requerir:

- `ydlidar_sdk`;
- SFML;
- OpenCV;
- libusb;
- libfreenect;
- Boost;
- Google Test / Google Mock;
- Node.js para servidores WebSocket.

## Relacion con `PhysarumVulkan`

La integracion ideal no deberia hacer que `PhysarumSim` dependa del hardware. La ruta recomendada es crear adaptadores:

```mermaid
flowchart LR
    Sensor[Datos reales del sensor] --> Adapter[Adaptador hardware a grilla]
    Adapter --> Sim[PhysarumSim]
    Sim --> Route[Ruta]
    Route --> RobotAdapter[Adaptador ruta a comandos]
    RobotAdapter --> Robot[Robot]
```

Asi el dominio sigue siendo portable y se puede probar sin tener el robot conectado.

