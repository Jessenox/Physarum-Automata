# System Diagrams

Language: [[Espanol|Diagramas-del-sistema]] | **English**

This page groups the main diagrams. They use Mermaid so GitHub Wiki can render them directly.

## Project overview

```mermaid
flowchart TB
    Doc[Academic LaTeX document] --> Req[Requirements]
    Req --> Sim[Physarum simulator]
    Req --> Robot[Proposed robot]
    Sim --> Route[Generated route]
    Route --> Robot
    Robot --> Sensors[LiDAR and camera]
    Sensors --> Monitoring[Monitoring system]
    Monitoring --> User[User]
```

## Proposed distributed architecture

```mermaid
flowchart LR
    App[Physarum App / Interface] -->|HTTP commands| EC2[AWS EC2 instance]
    EC2 --> HTTP[HTTP server]
    EC2 --> WS[WebSocket server]
    EC2 --> Core[Physarum algorithm]

    Robot[ Raspberry Pi robot] --> Lidar[LiDAR]
    Robot --> Cam[Camera]
    Robot --> Motors[Motors]
    Robot --> UnixLidar[Unix Socket LiDAR]
    Robot --> UnixControl[Unix Socket Control]

    UnixLidar -->|LiDAR data| WS
    Core -->|route / decision| HTTP
    HTTP -->|orders| UnixControl
    Cam -->|multimedia| EC2
    EC2 -->|monitoring| App
```

## Current code architecture

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
```

## Simulator execution flow

```mermaid
flowchart TD
    Start[Start application] --> Parse[Read --grid argument]
    Parse --> App[Create VulkanApp]
    App --> Init[Initialize window, Vulkan, simulation, and UI]
    Init --> Loop[Main loop]
    Loop --> Input[Process keyboard, mouse, menu]
    Input --> Paint{User paints or loads map?}
    Paint -->|yes| UpdateCells[Update cells and dirty region]
    Paint -->|no| Running{Simulation active?}
    UpdateCells --> Running
    Running -->|yes| Eval[Evaluate PhysarumSim]
    Running -->|no| Render
    Eval --> Upload[Partial or full texture upload]
    Upload --> Render[Render canvas and menu]
    Render --> Loop
```

## Cell data flow

```mermaid
flowchart LR
    Mouse[User click] --> Coord[Screen to cell conversion]
    Coord --> State[Selected state]
    State --> Matrix[physarumMatrix]
    Matrix --> Memory[memoryMatrix]
    Matrix --> Pixels[rgbaPixels]
    Pixels --> Dirty[DirtyRegion]
    Dirty --> Staging[Staging buffer]
    Staging --> Texture[Vulkan texture]
    Texture --> Screen[Screen]
```

## Map loading flow

```mermaid
flowchart TD
    File[Selected image] --> Read[OpenCV reads image]
    Read --> Resize[Resize to grid]
    Resize --> Gray[Convert to grayscale]
    Gray --> Blur[Gaussian blur]
    Blur --> Otsu[Inverted Otsu threshold]
    Otsu --> Morph[Morphological close]
    Morph --> Clean[Remove noise and fill holes]
    Clean --> Cells[Convert pixels to states]
    Cells --> Borders[Borders as repellents]
    Borders --> Texture[Update texture]
```

## Attractor flow

```mermaid
flowchart TD
    UI[User chooses ATR WxH] --> Start[ATRACTORES button]
    Start --> Mode{Cells <= 9?}
    Mode -->|yes| Exact[Exact enumeration]
    Mode -->|no| Approx[Approximate sampling]
    Exact --> Successor[Evaluate deterministic successors]
    Approx --> Successor
    Successor --> Device{GPU with shaderInt64?}
    Device -->|yes| Compute[Vulkan compute batches]
    Device -->|no| CPU[CPU]
    Compute --> Graph[Build graph]
    CPU --> Graph
    Graph --> Layout[Radial layout]
    Layout --> Window[Attractor window]
    Window --> Export[Export SVG / PNG]
```

