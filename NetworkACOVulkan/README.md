# NetworkACOVulkan

Aplicación independiente de búsqueda de rutas mediante Ant Colony Optimization (ACO) y análisis de la red de interacción de Krömer, Gajdoš y Zelinka. Mantiene el estilo visual oscuro de `PhysarumVulkan`, pero no modifica ni depende de esa aplicación.

## Grilla rectangular escalable

La aplicación abre por defecto un campo libre de `10000x10000`. También acepta cualquier rectángulo desde `8x8` hasta `10000x10000`:

```bash
./build/NetworkACOVulkan
./build/NetworkACOVulkan --grid 10000x6000
./build/NetworkACOVulkan --grid 48x32 --seed 9001
./build/NetworkACOVulkan --grid 48x32 --iterations 1200
./build/NetworkACOVulkan --unguided --smell --grid 32x24
```

La grilla es implícita: no se crean 100 millones de nodos ni cientos de millones de aristas. Solo se conservan los obstáculos realmente dibujados, hasta `1,048,576`. Cada hormiga tiene una tabla hash privada de celdas visitadas, dimensionada por el presupuesto máximo de pasos y no por el área completa de la grilla.

La feromona se mantiene en una malla direccional GPU de hasta `128x128x4`. Las rutas siguen usando coordenadas de celda completas, cuatro vecinos ortogonales y costo unitario. En `GUIADO`, cada hormiga dispone de `4*(ancho+alto)+1024` pasos. En `SIN GUIA`, el presupuesto aumenta a `16*(ancho+alto)+4096`, siempre limitado a `262144` pasos. La UI muestra el valor activo como `PASOS N`.

La colonia escala automáticamente entre 30 y 128 hormigas por generación según el lado mayor de la grilla. El crecimiento usa una curva de raíz cuadrada: grillas de hasta `32` celdas por lado conservan 30 hormigas, `1000x600` usa 57 y `10000x10000` solicita 128. La aplicación también calcula un límite a partir de `maxStorageBufferRange`; si la GPU no puede alojar las rutas, la memoria de visitados o el historial completo de interacción, reduce el conteo de forma segura y lo informa en consola.

## Inicio, varios destinos y obstáculos

Puede indicarse un origen y repetirse `--goal` para proporcionar hasta 30 destinos:

```bash
./build/NetworkACOVulkan --grid 10000x10000 \
  --start 1,5000 \
  --goal 9998,2500 \
  --goal 9998,7500
```

Las hormigas se distribuyen entre los destinos con `antIndex % goalCount`; así cada destino recibe exploradores. La UI conserva y dibuja la mejor ruta encontrada para cada uno, y compara la suma de sus costos con la suma de los óptimos de la grilla.

Herramientas de edición:

- `INICIO` o tecla `S`: mueve el único origen.
- `DESTINO` o tecla `G`: agrega un destino; otro clic sobre él lo elimina. Siempre debe quedar al menos uno.
- `OBSTACULO` o tecla `O`: agrega o elimina obstáculos con clic o arrastrando.
- Rueda del mouse: zoom centrado en el cursor.
- Arrastre con botón derecho: desplaza la cámara.
- `-100`/`+100` o teclas `[`/`]`: cambia el número de iteraciones y reinicia la colonia.

El valor exacto se configura con `--iterations N`. Se aceptan entre `1` y `10000`; el valor predeterminado es `300`:

```bash
./build/NetworkACOVulkan --iterations 750
./build/NetworkACOVulkan --iterations=2000
```

Cualquier edición reinicia la colonia y actualiza la referencia óptima. No se permite colocar un obstáculo sobre el origen o un destino.

## Escenarios reproducibles

El formato disperso `PACGRID 2` guarda dimensiones, origen, todos los destinos y únicamente las coordenadas bloqueadas:

```bash
./build/NetworkACOVulkan --grid 10000x10000 \
  --goal 9998,2500 --goal 9998,7500 \
  --save-scenario escenario.pacgrid
./build/NetworkACOVulkan --scenario escenario.pacgrid
```

Los escenarios antiguos `PACGRID 1` continúan siendo legibles.

## Modelo Network ACO

La elección roulette-wheel utiliza:

```text
weight = pheromone^alpha * heuristic^beta
```

La guía geométrica y el olfato son controles independientes:

- `GUIADO`/`SIN GUIA` se selecciona en la UI, con `M` o con `--guided`/`--unguided`. En `GUIADO`, reducir la distancia Manhattan aporta un factor `6.0` y aumentarla `0.35`. En `SIN GUIA` ese factor no existe y la hormiga no consulta la ubicación del destino para orientarse.
- `OLFATO` se activa o desactiva con su propio botón, la tecla `L` o `--smell`/`--no-smell`; está apagado por defecto. Cada destino emite desde T1 un campo volátil local `C(n)=max(0,1-d(n,goal)/64)^2`. Fuera de 64 celdas la concentración es exactamente cero; dentro del radio, subir por el gradiente modifica la probabilidad. Por ejemplo, en el escenario libre `1000x1000`, origen `(1,500)` y destino `(998,500)` están a 997 celdas: el olor inicial es `0`.

Si guía y olfato están activos, sus factores se multiplican; ninguno sustituye al otro. Los obstáculos impiden el movimiento, pero no bloquean este olor aéreo simplificado.

En las cuatro combinaciones la memoria tabú es estricta: un vecino ya visitado por esa hormiga queda fuera de la ruleta con probabilidad `0`. Si no queda una salida nueva, la hormiga retrocede de forma determinista por su propia ruta hasta encontrar otra bifurcación. Ese retroceso no entra en la ruleta ni recibe una probabilidad de visita. La ruta activa usada para puntuar y depositar feromona nunca contiene ciclos. La tabla usa un sello por recorrido que se conserva entre bloques GPU, por lo que no necesita borrar cientos de megabytes antes de cada exploración. Cambiar cualquiera de los dos controles reinicia la colonia para no mezclar feromonas de experimentos distintos.

Cada generación `T` construye una ruta estocástica e independiente por hormiga. La GPU las avanza en bloques seguros de hasta `2048` movimientos y conserva entre frames posición, ruta activa y memoria tabú. T1 es la fase de descubrimiento: termina inmediatamente cuando existe al menos una ruta válida hacia cada alimento, publica y anima esa primera solución, y no espera a que las demás exploradoras agoten sus recorridos. Como T1 se cierra antes de completar la colonia, cada fundadora deposita el máximo entre `(Q/L)*(hormigas asignadas/hormigas exitosas)` y `(8*tau0)/hormigas exitosas`: la primera ruta conserva una señal visible incluso cuando la grilla crece.

Desde T2 ninguna hormiga recibe ni copia la mejor ruta. Todas vuelven a construir desde el origen aplicando en cada arista la ruleta original `P(i,j) = tau(i,j)^alpha * eta(i,j)^beta / suma`, con su propia memoria tabú y su propio generador aleatorio. La feromona evaporada se lee con el piso numérico real de punto fijo `1/65536`, no con un piso artificial `1.0`; así las aristas reforzadas siguen siendo atractivas y las no usadas pierden influencia. Cada generación espera a que toda la colonia concluya antes de evaporar y sumar el `Q/L` de todas las rutas exitosas. Una hormiga sin vecino nuevo retrocede determinísticamente hasta otra bifurcación; ese retroceso no participa en la ruleta, no deposita feromona y la ruta activa continúa sin ciclos.

Solo al cerrar la T se aplica la actualización por lotes del ACO original: primero se evapora con `rho` y después cada ruta exitosa deposita `Q/L`. Las rutas cortas depositan más, la siguiente generación decide con esa feromona y la mejor ruta global solo cambia cuando aparece una solución realmente más corta. Las hormigas fallidas no depositan. `GUIADO` y `OLFATO` siguen siendo factores opcionales e independientes de este aprendizaje.

`SIN GUIA` con el destino fuera del radio de olor es útil como control experimental, pero encontrar por primera vez una celda concreta resulta muy improbable en una grilla como `10000x10000`. En ese caso T1 puede requerir muchísimos bloques y tiempo de cómputo; permanece abierta hasta encontrar los destinos o hasta que el usuario intervenga.

La feromona total y la contribución atribuida a cada hormiga usan punto fijo de 16 bits fraccionarios para permitir `atomicAdd` estándar en Vulkan. Cuando la hormiga `j` elige una dirección, se registra la feromona previa aportada por cada hormiga `i` en `I[i][j]`; `I[i][i]` permanece en cero. Después se evapora con `rho` y cada ruta exitosa deposita `Q/pathCost`.

Configuración inicial:

```text
ants = AUTO 30..128 iterations = 300
alpha = 1.0        beta = 2.0
rho = 0.10         Q = 100.0
initial = 1.0      seed = 12345
```

## Vulkan Compute

Cada iteración utiliza despachos y barreras explícitas para:

1. limpiar `I(t)` e inicializar la memoria tabú al reiniciar la colonia;
2. construir todas las rutas de la colonia en paralelo sobre la grilla implícita;
3. guardar `I(t)` y acumular `I_total`;
4. evaporar la malla de feromona una vez al finalizar cada T;
5. depositar feromona atribuida y actualizar la mejor ruta por destino.

Se conserva una matriz de interacción y los costos de todas las hormigas por cada iteración configurada, además de la mejor ruta global de cada destino. La CPU comprueba que cada ruta GPU es contigua, evita obstáculos y termina exactamente en el destino asignado. Los buffers admiten hasta `10000` iteraciones y las gráficas reducen visualmente historiales grandes a un máximo de 512 muestras.

## Vistas y controles

- `GRAFO`: campo, obstáculos, feromona direccional y rutas encontradas.
- `RED`: red dirigida de influencia para la iteración seleccionada.
- `METRICAS`: costo, concentración y entropía.
- `Space`: iniciar o pausar.
- `N`: ejecutar una iteración.
- `R`: reiniciar la colonia con la misma grilla.
- `M`: alternar entre `GUIADO` y `SIN GUIA`; reinicia la colonia.
- `L`: activar o desactivar `OLFATO`; reinicia la colonia.
- `1`, `2`, `3`: cambiar de vista.
- `←`, `→`: recorrer `I(1)...I(t)`.
- `X1`, `X5`, `X20`: velocidad visual de las hormigas. En ejecución normal la siguiente iteración espera a que las hormigas terminen de recorrer la ruta mostrada.

Después de cada iteración se copian los movimientos reales de todas las hormigas activas y se dibuja un círculo animado por hormiga. La UI muestra `HORM N AUTO` con el conteo elegido. El color identifica el destino asignado; si el olfato está activo, el campo magenta representa olor y el cian representa feromona. En `GUIADO`, la primera iteración dura aproximadamente 8 segundos en `X1` y las siguientes 2.5 segundos. En `SIN GUIA`, la duración depende del recorrido real más largo de la generación y usa 240 pasos por segundo en `X1`. Al descubrir la primera solución, la hormiga ganadora reproduce la ruta completa desde el origen y la polilínea dorada aparece progresivamente detrás de ella; T2 espera a que termine esta visualización. La UI muestra `DURACION APROX Ns` y mantiene el estado de recorrido hasta finalizar visualmente. Si una ruta tiene decenas de miles de giros, solo su polilínea dorada se reduce visualmente para respetar la capacidad de la UI; la ruta GPU completa, su costo y su validación no se recortan. `--verify` omite esta espera visual y continúa procesando por lotes.

## Compilación y validación

```bash
sudo apt install build-essential cmake pkg-config libglfw3-dev vulkan-tools glslang-tools
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=ON
cmake --build build -j
ctest --test-dir build --output-on-failure

./build/NetworkACOVulkan --verify --grid 10000x10000 \
  --start 1,5000 --goal 9998,2500 --goal 9998,7500 \
  --iterations 300

./build/NetworkACOVulkan --verify --unguided --no-smell --grid 8x8 \
  --start 1,4 --goal 6,4 --iterations 20

./build/NetworkACOVulkan --verify --unguided --smell --grid 1000x1000 \
  --start 100,500 --goal 700,500 --iterations 20
```

`--verify` ejecuta la cantidad configurada, valida las rutas, informa `T1Blocks`, comprueba que la diagonal de interacción sea cero y compara el costo total ACO con la referencia óptima. En grillas unitarias se usa A* con heurística Manhattan admisible, que produce el mismo costo óptimo que Dijkstra sin reservar distancias para 100 millones de celdas.

La versión con memoria tabú estricta y olor fue validada sobre una AMD Radeon RX 7800 XT en `16x16` libre: costo ACO `13`, óptimo `13`; y en el escenario reproducible `32x32` con 284 obstáculos: costo ACO `37`, óptimo `35`. En ambos casos las comprobaciones GPU confirmaron rutas contiguas, sin celdas repetidas y diagonal de interacción igual a cero.

`PhysarumVulkan` no fue modificado. Physarum conserva su vecindad de Moore, mientras Network ACO utiliza cuatro vecinos ortogonales; esa diferencia debe considerarse al comparar resultados.
