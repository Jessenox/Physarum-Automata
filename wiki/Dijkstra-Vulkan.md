# Dijkstra Vulkan

`DijkstraVulkan` es un visualizador interactivo de rutas minimas sobre una grilla ponderada. Es una aplicacion independiente con la misma composicion visual de `PhysarumVulkan`: lienzo de `500x500`, panel de estado inferior y menu lateral oscuro.

## Vulkan Compute

Cada iteracion ejecuta cuatro pases compute: limpiar el minimo, buscar en paralelo la menor distancia tentativa, elegir un nodo de forma determinista y relajar sus vecinos. Pesos, distancias, predecesores y nodos visitados viven en buffers SSBO compartidos con el render.

Al terminar se reconstruye la ruta y su costo se compara con una implementacion Dijkstra de referencia en CPU. El estado `RUTA GPU VALIDADA` indica que ambos coinciden.

## Compilar y ejecutar

```bash
cd DijkstraVulkan
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
ctest --test-dir build --output-on-failure
./build/DijkstraVulkan
```

Para probar automaticamente el pipeline compute local:

```bash
./build/DijkstraVulkan --grid 16x16 --verify
```

Los controles completos y la explicacion de los pesos estan en `DijkstraVulkan/README.md`.

## Capacidad de grilla

El backend denso interactivo tiene un limite de seguridad de `1,048,576` nodos, equivalente a `1024x1024`. Una grilla `1000x1000` esta admitida; `10000x10000` se rechaza antes de inicializar Vulkan o reservar memoria. La consola muestra por separado la capacidad bruta de buffers de la GPU y el limite interactivo.

Cada paso exacto de Dijkstra hace tres recorridos compute completos de la grilla. Por eso la cantidad solicitada de pasos por frame se limita automaticamente segun el numero de nodos; la UI muestra el maximo efectivo. Esto evita monopolizar la GPU que usa el escritorio.

El render tambien usa presentacion FIFO y queda limitado a `60 FPS`, incluso con la busqueda pausada.
