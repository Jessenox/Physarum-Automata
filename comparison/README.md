# Benchmark reproducible 10k: Physarum y Dijkstra

Este directorio ejecuta Physarum y Dijkstra sobre exactamente el mismo `PACGRID` de `10000x10000` y conserva cada ejecución, incluidos fallos y timeouts. La comparación anterior de 50–1000 con ACO permanece en `results/`.

## Casos incluidos

- campo libre;
- Ponderosa Cave System, a partir de `/home/eduardohv/Descargas/cave.png`;
- Catacumbas de París, extrayendo la capa de túneles de `/home/eduardohv/Descargas/Mapa catacumbas.xcf`;
- tamaño exacto `10000x10000` (100 millones de celdas).

Los extremos de los mapas reales se eligen como una aproximación al diámetro del componente transitable principal. `generated-10k/manifest.json` registra coordenadas, cantidad de obstáculos, ruta original y SHA-256 de cada imagen. Las vistas binarias quedan en `generated-10k/previews/` para revisión antes del experimento.

Los escenarios grandes usan `PACGRID 3`: comprime obstáculos consecutivos por fila, pero al cargarlos reconstruye las 100 millones de celdas exactas. No es una simulación reducida ni un reescalado interno.

La procedencia se tomó del artículo local `/home/eduardohv/Descargas/main.pdf`: el sistema de cuevas corresponde a la Figura 6 y las catacumbas a la Figura 7. El manifest también registra el SHA-256 del PDF. El mapa libre sirve como control.

## Requisitos

- compilador C++20, CMake y Vulkan;
- `glslc` para compilar el shader original de Physarum;
- Python 3 con NumPy y OpenCV;
- ImageMagick para extraer la capa transitable del archivo XCF de las catacumbas.

## Ejecución completa

```bash
./comparison/scripts/run_all.sh
```

Por defecto se hacen tres repeticiones de Physarum y Dijkstra, con un timeout de dos horas por proceso. Physarum puede tardar decenas de minutos por mapa; una batería completa puede durar varias horas. Si se interrumpe, puede continuar sin repetir resultados terminados:

```bash
./comparison/scripts/run_all.sh --resume
```

Prueba rápida de la preparación y de Dijkstra, sin iniciar todavía el cálculo largo de Physarum:

```bash
./comparison/scripts/run_all.sh \
  --algorithms dijkstra --repetitions 1 --timeout 600
```

También se pueden ejecutar las fases por separado:

```bash
./comparison/scripts/build_benchmarks.sh
python3 comparison/scripts/prepare_maps.py \
  --output comparison/generated-10k --sizes 10000
python3 comparison/scripts/run_benchmarks.py \
  --manifest comparison/generated-10k/manifest.json \
  --output comparison/results-10k \
  --sizes 10000 --algorithms physarum dijkstra --repetitions 3
```

## Salidas

- `results-10k/runs.csv`: tabla completa para Excel, R o Python;
- `results-10k/runs.jsonl`: una observación estructurada por línea;
- `results-10k/summary.md`: éxito, tiempo medio, mejor ruta, brecha y pico de memoria;
- `results-10k/logs/`: salida íntegra y medición de `/usr/bin/time` de cada proceso;
- `generated-10k/previews/`: mapas con inicio verde y destino rojo.

## Qué se mide

- tiempo total del proceso;
- tiempo aislado del algoritmo cuando el runner puede medirlo;
- pico RSS;
- éxito, timeout o error;
- generaciones de Physarum;
- costo de la mejor ruta;
- costo óptimo y brecha para cada topología (cuatro vecinos en Dijkstra y Moore en Physarum);
- celdas de la red final de Physarum;
- nodos visitados por Dijkstra.

## Límites de interpretación

Dijkstra usa cuatro vecinos con costo unitario. Physarum conserva la vecindad de Moore descrita en el artículo; su ruta final se mide con costo `1` ortogonal y `1.414` diagonal. La brecha de Physarum usa como referencia una ruta óptima Moore calculada sobre el mismo mapa y con la misma regla de esquinas del shader; no se mezcla con el óptimo de cuatro vecinos.

El tiempo se resume como media y desviación estándar. El pico RSS es memoria del proceso y no representa toda la memoria privada ocupada dentro de la GPU.

`DijkstraBenchmark` reutiliza `GridModel::solveCpu`, porque el Dijkstra Vulkan interactivo realiza una reducción global densa por nodo y bloquearía la máquina a esta escala. `PhysarumBenchmark` ejecuta directamente el SPIR-V original `physarum.comp.spv` en Vulkan compute, sin modificar la regla ni el programa interactivo.

En la validación local, Dijkstra resolvió los tres mapas 10k en 1.79–4.84 segundos y alcanzó un pico RSS máximo aproximado de 1.29 GiB. Physarum se deja para la batería larga.
