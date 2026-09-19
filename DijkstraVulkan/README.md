# DijkstraVulkan

Visualizador interactivo del algoritmo de Dijkstra sobre una grilla ponderada. Es una aplicación hermana de `PhysarumVulkan`: conserva el lienzo de `500x500`, el panel inferior, la barra lateral oscura y el render directo con Vulkan/GLFW.

## Qué calcula la GPU

Cada iteración de Dijkstra se ejecuta en Vulkan Compute mediante cuatro despachos y barreras explícitas:

1. reinicio del mínimo de la iteración;
2. reducción paralela de la menor distancia tentativa con `atomicMin`;
3. selección determinista del nodo de menor índice en caso de empate;
4. marcado y relajación de sus vecinos.

Los buffers SSBO de pesos, distancias, predecesores y visitados se comparten con el render. Al finalizar, la aplicación reconstruye la ruta y compara su costo contra una implementación de referencia en CPU; `RUTA GPU VALIDADA` confirma que ambos resultados coinciden.

## Controles

- `Space`: iniciar o pausar.
- `N`: ejecutar una iteración.
- `R`: reiniciar la búsqueda conservando el mapa.
- `C`: limpiar obstáculos y pesos.
- `M`: generar obstáculos aleatorios.
- `1`: colocar origen.
- `2`: colocar destino.
- `3`: pintar muros.
- `4`, `5`, `6`: pintar pesos `1`, `2` y `5`.
- Mouse izquierdo: pintar con la herramienta seleccionada.

La barra lateral también contiene todas estas acciones y permite ejecutar `1`, `8` o `64` iteraciones por frame.

## Compilación

En Ubuntu usa las mismas dependencias base que `PhysarumVulkan`:

```bash
sudo apt install build-essential cmake pkg-config libglfw3-dev vulkan-tools glslang-tools
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
ctest --test-dir build --output-on-failure
```

## Ejecución

```bash
./build/DijkstraVulkan
./build/DijkstraVulkan --grid 96x96
./build/DijkstraVulkan --grid 16x16 --verify
```

La grilla por defecto es `64x64`. Por seguridad, el backend denso interactivo admite como máximo `1,048,576` nodos, equivalente a `1024x1024`; por tanto `1000x1000` está admitido y `10000x10000` se rechaza antes de crear Vulkan o reservar memoria. La consola distingue la capacidad bruta de buffers de la GPU de este límite interactivo.

Los pesos usan `8 bits` y los estados visitado/ruta `2 bits`; distancias y predecesores conservan `32 bits`. Aunque la memoria física permita buffers mayores, cada paso de este Dijkstra exacto recorre toda la grilla para seleccionar el mínimo global. Permitir el máximo teórico monopolizaría la GPU que también dibuja el escritorio.

La velocidad solicitada (`X1`, `X8` o `X64`) se limita automáticamente según el número de nodos. La interfaz muestra `PASOS POR FRAME MAX` con el valor efectivo seguro. Para observar la animación con claridad se siguen recomendando tamaños de hasta `128x128`.

El render usa presentación FIFO y un límite adicional de `60 FPS`, incluso cuando la búsqueda está pausada, para no ocupar la GPU del escritorio con frames innecesarios.

`--verify` inicia automáticamente, compara el resultado Compute contra la referencia CPU, imprime el resultado y cierra la ventana. Sirve como prueba rápida del pipeline Vulkan en la GPU local.
