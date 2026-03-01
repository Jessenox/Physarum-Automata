# PhysarumVulkan

Migración del simulador de `PhyTest` a Vulkan sin usar SFML. El render principal sigue siendo una textura `R8G8B8A8_UNORM` con `nearest sampling` sobre un canvas fijo de `500x500`, ahora acompañado por un menú lateral en una ventana `900x700`. El panel inferior original y el indicador de estado siguen intactos en la columna izquierda.

## Estado actual

- Render principal funcionando con Vulkan y GLFW
- Selección de estado y pintado con mouse funcionando
- Menú lateral con botones y edición de colores por estado
- Carga de mapa desde imagen con OpenCV y selector de archivo del sistema
- Lista visible de los 9 estados con su significado
- Cálculo y visualización radial de atractores para subrejillas pequeñas
- Zoom con `Ctrl + scroll` centrado sobre el cursor
- Pan con `mouse medio + drag` o `Ctrl + clic izquierdo + drag` con suavizado e inercia
- Grid size dinámico funcionando en runtime
- Upload parcial/full de textura funcionando

Validación visual rápida:

- el panel oscuro de control debe verse abajo
- el menú lateral debe verse a la derecha sin alterar el canvas de `500x500`
- el clic izquierdo debe pintar sobre la celda visible bajo el cursor

## Notas de implementación

Quedaron aplicadas estas correcciones importantes en la versión actual:

- corrección de la conversión vertical a NDC para Vulkan, para que la geometría no quede invertida en `Y`
- mapeo del mouse usando el viewport real de la ventana, no un `500x500` fijo
- sampleo exacto por texel en el shader (`texelFetch`) para que el píxel visible coincida con la celda pintada
- prioridad temporal del pan sobre otras entradas de mouse mientras la vista se está moviendo
- cursor de mano como feedback visual cuando `Ctrl` activa el modo pan sobre el canvas

Esto es especialmente importante en Ubuntu GNOME, donde el escalado de pantalla o la diferencia entre tamaño de ventana y framebuffer puede hacer visible cualquier error de mapeo.

## Controles

- `1`..`9`: seleccionan el estado `0`..`8`
- `Enter`: pausa / reanuda la simulación
- `Mouse izquierdo`: pinta sobre la grilla
- `Mouse izquierdo` sobre el menú: selecciona estados, ajusta color RGB o abre acciones del menú
- `Ctrl + scroll`: zoom centrado sobre el cursor
- `Mouse medio + drag`: pan de la vista con suavizado
- `Ctrl + clic izquierdo + drag`: pan alternativo con suavizado
- `R`: reset de zoom/pan
- `F1`: `200x200`
- `F2`: `512x512`
- `F3`: `1024x1024`
- `F4`: `4000x4000`

Al cambiar el tamaño de la grilla se pausa la simulación, `generation` vuelve a `0` y la textura Vulkan se recrea si hace falta.

Mientras el pan está activo o la vista sigue moviéndose por inercia, se bloquean temporalmente el pintado y el zoom para que la navegación se sienta más limpia.

## Menú lateral

- `CARGAR MAPA`: abre un selector de archivo y convierte la imagen a mapa binario usando OpenCV, siguiendo la lógica de `PhyTest` (`gris + threshold`, bordes en estado `2`)
- `ATRACTORES`: genera el grafo de atractores de una subrejilla configurable usando la regla del autómata y lo abre en una ventana dedicada grande con autoencuadre mientras descubre nodos
- `REFINAR`: en modo aproximado continúa explorando el mismo grafo sin empezar desde cero
- `EXPORT SVG`: guarda el último atractor calculado como `SVG` vectorial con el layout actual
- `EXPORT PNG`: guarda el último atractor calculado como `PNG` con el mismo layout claro de la vista dedicada
- `ESTADOS`: muestra los nueve estados disponibles y permite seleccionarlos con click
- `COLOR RGB`: ajusta el color del estado seleccionado con botones `+/-` por canal, actualizando la textura completa al instante
- `ATR WxH`: tamaño de subrejilla para atractores; se ajusta con `W-/W+` y `H-/H+` en un rango de `1x1` a `5x5`

Notas de atractores:

- la simulación normal no cambia; el modo atractor usa una versión determinista de la misma regla para que el grafo exista y sea reproducible
- cuando la GPU soporta `shaderInt64`, la evaluación de sucesores se hace por lotes con `Vulkan compute`; si no, cae automáticamente a CPU
- el modo se decide por tamaño del estado: hasta `9` celdas es `MODO EXACTO`; arriba de eso es `MODO APROX`
- `MODO APROX` conserva la regla real de `PhysarumSim` y explora muestras sucesivas del espacio de estados sin simplificar el autómata
- el botón `REFINAR` agrega otro bloque de muestras al grafo aproximado ya descubierto
- la vista dedicada del atractor usa modo oscuro, una leyenda interna de colores (`CIAN TRANSICION`, `MAGENTA NODO`, `DORADO CICLO`) y va ampliando el encuadre hacia afuera conforme aparecen nodos nuevos
- la exportación `SVG` conserva ese layout y estilo claro, así que puedes escalarla sin perder nitidez
- la exportación `PNG` usa ese mismo layout, pero depende de OpenCV igual que la carga de mapas
- el contador `SEM` muestra semillas exactas o muestras procesadas y `NOD` los nodos descubiertos hasta ese momento; ambos se actualizan durante el cálculo

Estados mostrados en el menú:

- `0`: `LIBRE`
- `1`: `NUTR NO`
- `2`: `REPELENTE`
- `3`: `INICIO`
- `4`: `GEL CONT`
- `5`: `GEL COMP`
- `6`: `NUTR OK`
- `7`: `EXPANSION`
- `8`: `GEL SIN`

## Grid size dinámico

La grilla se configura en runtime, sin recompilar:

```bash
./PhysarumVulkan --grid 200x200
./PhysarumVulkan --grid 512x512
./PhysarumVulkan --grid 1024x1024
./PhysarumVulkan --grid 4000x4000
```

Si no se pasa `--grid`, el valor por defecto es `200x200`.

## Upload de textura

- Textura Vulkan: `VK_FORMAT_R8G8B8A8_UNORM`
- Sampler: `VK_FILTER_NEAREST`
- Fragment shader con `texelFetch` para lectura exacta por celda
- Staging buffers persistentes y mapeados, uno por frame en vuelo
- Full upload cuando el área dirty supera el `30%`
- Partial upload cuando el bounding box dirty es pequeño

En consola, al iniciar y al hacer resize de grilla, se imprime:

- tamaño de grilla
- memoria estimada de textura
- memoria total estimada para staging persistente
- política de upload full/partial

Si el GPU no soporta el tamaño solicitado, el programa termina con un error claro indicando `maxImageDimension2D`.

## Dependencias en Ubuntu

```bash
sudo apt update
sudo apt install -y build-essential cmake pkg-config libglfw3-dev vulkan-tools vulkan-validationlayers glslang-tools
sudo apt install -y libopencv-dev python3-tk
```

Para el selector de archivo se intenta usar `zenity`, `kdialog` o un fallback con `tkinter`. Si OpenCV no se encuentra al compilar, la build sigue funcionando, pero los botones de mapa y `EXPORT PNG` reportarán `OPENCV OFF`. La vista dedicada de atractores sigue funcionando porque ahora se renderiza con Vulkan en una segunda ventana GLFW.

## Build

```bash
mkdir -p build
cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
cmake --build .
```

## Ejecución

```bash
./PhysarumVulkan
./PhysarumVulkan --grid 200x200
./PhysarumVulkan --grid 4000x4000
```

## Diagnóstico rápido

Si en una build nueva el input vuelve a verse mal:

1. verificar que el panel esté abajo y no arriba
2. verificar que se esté ejecutando el binario recién compilado desde `build/PhysarumVulkan`
3. revisar `glfwGetCursorPos`, `glfwGetWindowSize`, `glfwGetFramebufferSize` y `glfwGetWindowContentScale`
4. revisar si la sesión está corriendo en Wayland o X11
