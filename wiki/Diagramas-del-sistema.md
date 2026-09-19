# Diagramas del sistema

Esta pagina concentra los diagramas principales. Estan escritos en Mermaid para que GitHub Wiki los renderice directamente.

## Vista general del proyecto

```mermaid
flowchart TB
    Doc[Documento academico LaTeX] --> Req[Requerimientos]
    Req --> Sim[Simulador Physarum]
    Req --> Robot[Robot propuesto]
    Sim --> Ruta[Ruta generada]
    Ruta --> Robot
    Robot --> Sensores[LiDAR y camara]
    Sensores --> Monitoreo[Sistema de monitoreo]
    Monitoreo --> Usuario[Usuario]
```

## Arquitectura distribuida propuesta

```mermaid
flowchart LR
    App[Physarum App / Interfaz] -->|HTTP comandos| EC2[Instancia AWS EC2]
    EC2 --> HTTP[Servidor HTTP]
    EC2 --> WS[Servidor WebSocket]
    EC2 --> Core[Algoritmo Physarum]

    Robot[Robot Raspberry Pi] --> Lidar[LiDAR]
    Robot --> Cam[Camara]
    Robot --> Motors[Motores]
    Robot --> UnixLidar[Unix Socket LiDAR]
    Robot --> UnixControl[Unix Socket Control]

    UnixLidar -->|datos LiDAR| WS
    Core -->|ruta / decision| HTTP
    HTTP -->|ordenes| UnixControl
    Cam -->|multimedia| EC2
    EC2 -->|monitoreo| App
```

## Arquitectura del codigo actual

```mermaid
flowchart TB
    Main[src/main.cpp] --> VulkanApp[presentation/app/VulkanApp]
    VulkanApp --> VM[presentation/mvvm/AppViewModel]
    VulkanApp --> Model[presentation/mvvm/AppModel]
    VulkanApp --> Controller[application/AppController]
    Controller --> Sim[domain/simulation/PhysarumSim]
    Controller --> Attractor[domain/attractor/AttractorGenerator]

    VulkanApp --> ViewTransform[presentation/ui/ViewTransform]
    VulkanApp --> Preview[presentation/ui/AttractorPreviewWindow]
    VulkanApp --> VkHelpers[infrastructure/vulkan/VulkanHelpers]
    VulkanApp --> AttractorCompute[infrastructure/vulkan/AttractorCompute]
    VulkanApp --> FileDialog[infrastructure/platform/FileDialog]
    VulkanApp --> Exporter[infrastructure/export/AttractorGraphExporter]

    Sim --> Dirty[DirtyRegion]
    Sim --> Grid[GridSize]
    Attractor --> Graph[AttractorGraph]
```

## Flujo de ejecucion del simulador

```mermaid
flowchart TD
    Start[Iniciar aplicacion] --> Parse[Leer argumento --grid]
    Parse --> App[Crear VulkanApp]
    App --> Init[Inicializar ventana, Vulkan, simulacion y UI]
    Init --> Loop[Bucle principal]
    Loop --> Input[Procesar teclado, mouse, menu]
    Input --> Paint{Usuario pinta o carga mapa?}
    Paint -->|si| UpdateCells[Actualizar celdas y region sucia]
    Paint -->|no| Running{Simulacion activa?}
    UpdateCells --> Running
    Running -->|si| Eval[Evaluar PhysarumSim]
    Running -->|no| Render
    Eval --> Upload[Subir textura parcial o completa]
    Upload --> Render[Renderizar canvas y menu]
    Render --> Loop
```

## Flujo de datos de una celda

```mermaid
flowchart LR
    Mouse[Click del usuario] --> Coord[Conversion pantalla a celda]
    Coord --> State[Estado seleccionado]
    State --> Matrix[physarumMatrix]
    Matrix --> Memory[memoryMatrix]
    Matrix --> Pixels[rgbaPixels]
    Pixels --> Dirty[DirtyRegion]
    Dirty --> Staging[Staging buffer]
    Staging --> Texture[Textura Vulkan]
    Texture --> Screen[Pantalla]
```

## Flujo de carga de mapa

```mermaid
flowchart TD
    File[Imagen seleccionada] --> Read[OpenCV lee imagen]
    Read --> Resize[Redimensionar a grilla]
    Resize --> Gray[Convertir a gris]
    Gray --> Blur[Gaussian blur]
    Blur --> Otsu[Threshold Otsu invertido]
    Otsu --> Morph[Cierre morfologico]
    Morph --> Clean[Eliminar ruido y rellenar huecos]
    Clean --> Cells[Convertir pixeles a estados]
    Cells --> Borders[Bordes como repelente]
    Borders --> Texture[Actualizar textura]
```

## Flujo de atractores

```mermaid
flowchart TD
    UI[Usuario elige ATR WxH] --> Start[Boton ATRACTORES]
    Start --> Mode{Celdas <= 9?}
    Mode -->|si| Exact[Enumeracion exacta]
    Mode -->|no| Approx[Muestreo aproximado]
    Exact --> Successor[Evaluar sucesores deterministas]
    Approx --> Successor
    Successor --> Device{GPU con shaderInt64?}
    Device -->|si| Compute[Vulkan compute por lotes]
    Device -->|no| CPU[CPU]
    Compute --> Graph[Construir grafo]
    CPU --> Graph
    Graph --> Layout[Layout radial]
    Layout --> Window[Ventana de atractor]
    Window --> Export[Export SVG / PNG]
```

## Secuencia de una generacion

```mermaid
sequenceDiagram
    participant App as VulkanApp
    participant Sim as PhysarumSim
    participant Aux as auxMatrix
    participant GPU as Textura Vulkan

    App->>Sim: evaluatePhysarum()
    Sim->>Aux: copia estado actual
    loop por cada celda
        Sim->>Sim: gatherNeighbours()
        Sim->>Sim: physarumTransitionConditions()
        Sim->>Aux: escribe nuevo estado si cambia
    end
    Sim->>Sim: swap(physarumMatrix, auxMatrix)
    Sim->>Sim: calcula DirtyRegion
    App->>GPU: upload parcial o completo
    App->>App: render
```

