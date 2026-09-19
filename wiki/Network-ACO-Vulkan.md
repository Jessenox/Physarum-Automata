# Network ACO con Vulkan

`NetworkACOVulkan` busca rutas con una colonia ACO y reconstruye la red de influencia entre hormigas. Su grilla rectangular implícita admite desde `8x8` hasta `10000x10000` sin materializar todos los nodos y aristas.

El escenario inicial es un campo libre. Los obstáculos se guardan de forma dispersa y se agregan o eliminan manualmente con `OBSTACULO`/`O`. La rueda controla el zoom y el botón derecho desplaza la cámara.

Existe un origen y hasta 30 destinos. `DESTINO`/`G` alterna cada destino con un clic; por CLI puede repetirse el argumento:

```bash
./NetworkACOVulkan --grid 10000x10000 --start 1,5000 \
  --goal 9998,2500 --goal 9998,7500 --iterations 1200
```

El número de iteraciones acepta valores de `1` a `10000`; puede fijarse exactamente con `--iterations N` o ajustarse en pasos de 100 desde la UI con `-100`/`+100` y las teclas `[`/`]`.

## Representación escalable

- Los vecinos se calculan directamente desde `(x,y)`.
- Solo se almacenan los obstáculos existentes, con límite de `1,048,576`.
- La feromona direccional reside en una malla GPU de hasta `128x128x4`.
- Las coordenadas de las rutas conservan la resolución completa.
- Cada hormiga mantiene una tabla hash privada de visitas, acotada por el presupuesto de pasos.
- Cada hormiga explora el destino `antIndex % goalCount`.
- `GUIADO` usa hasta `4*(ancho+alto)+1024` pasos por hormiga; `SIN GUIA` usa hasta `16*(ancho+alto)+4096`, con límite absoluto de `262144`.

La cantidad de hormigas por generación escala automáticamente entre 30 y 128 según el lado mayor de la grilla: `24x16` usa 30, `1000x600` usa 57 y `10000x10000` usa 128 en una GPU compatible. El límite se reduce automáticamente si `maxStorageBufferRange` no permite guardar rutas, visitados e historial completo.

La red de interacción conserva un nodo por hormiga activa. Al recorrer una dirección, la hormiga receptora registra la contribución atribuida previamente a las demás hormigas. La diagonal `I[i][i]` permanece en cero.

## Guía y olfato independientes

La UI conserva `GUIADO`/`SIN GUIA`, alternable con `M` o `--guided`/`--unguided`. `OLFATO` es otro interruptor, controlado con su botón, `L` o `--smell`/`--no-smell`. Cambiar cualquiera reinicia la colonia y sus feromonas.

- En `GUIADO`, los movimientos que reducen la distancia Manhattan reciben el factor geométrico original. `SIN GUIA` elimina ese conocimiento global.
- Con `OLFATO`, cada destino emite desde T1 el campo local `C(n)=max(0,1-d(n,goal)/64)^2`. Fuera de 64 celdas la señal es exactamente cero; dentro del radio se favorece el gradiente ascendente. En `1000x1000`, el origen y destino predeterminados están separados por 997 celdas, por lo que el origen no detecta olor. El campo simplificado atraviesa obstáculos, aunque estos siguen bloqueando el movimiento.

Guía y olfato pueden estar activos juntos, separados o ambos apagados; sus factores se multiplican y ninguno sustituye al otro.

En ambos modos, una celda ya visitada queda excluida de la ruleta con probabilidad cero. Si no existe una salida nueva, la hormiga retrocede de forma determinista por su ruta hasta otra bifurcación; ese retroceso no participa en la ruleta. La ruta activa no contiene ciclos y los sellos por recorrido, conservados entre bloques, permiten reutilizar la tabla hash.

La exploración sin guía y fuera del radio de olor es un control experimental adecuado para grillas pequeñas. En `10000x10000`, `T1` puede requerir muchísimos bloques y tiempo de cómputo; permanece abierta hasta encontrar los destinos o hasta que el usuario intervenga.

Cada `T` construye rutas estocásticas independientes en bloques GPU de hasta 2048 movimientos. T1 termina y publica la primera solución en cuanto existe una ruta hacia cada alimento, sin esperar al resto de la colonia. Cada fundadora deposita el máximo entre `(Q/L)*(hormigas asignadas/hormigas exitosas)` y `(8*tau0)/hormigas exitosas`, de modo que el corredor no pierde fuerza al aumentar la grilla. Desde T2 ninguna hormiga copia la mejor ruta: todas regresan al origen y deciden cada arista con la ruleta `tau^alpha * eta^beta`, memoria tabú y azar independiente. La feromona evaporada usa como piso solamente un cuanto de punto fijo, `1/65536`; las zonas sin uso dejan de competir artificialmente con las rutas reforzadas. Solo al cerrar la generación se evapora feromona y cada ruta exitosa deposita `Q/L`. El retroceso DFS no agrega ciclos ni participa en la ruleta. `GUIADO` y `OLFATO` permanecen independientes.

## UI y validación

Las vistas `GRAFO`, `RED` y `METRICAS` muestran las rutas por destino, la influencia temporal y las estadísticas. La referencia óptima suma la ruta mínima hacia cada destino. A* con Manhattan produce el mismo costo que Dijkstra en esta grilla unitaria sin reservar un arreglo de 100 millones de distancias.

La vista `GRAFO` anima todas las hormigas activas como círculos de colores sobre sus rutas individuales y muestra `HORM N AUTO`, `PASOS N` y la duración aproximada. Los botones `GUIADO`, `SIN GUIA` y `OLFATO` controlan los dos factores independientes. Cuando está activo, el campo magenta representa olor y el cian representa feromona. `X1`, `X5` y `X20` controlan la velocidad. Al encontrar la primera solución, la hormiga ganadora reproduce la ruta completa desde el origen mientras la línea dorada aparece detrás de ella; T2 espera a que termine. Las rutas con decenas de miles de giros se reducen solo al dibujar la polilínea dorada; la ruta completa y sus métricas permanecen intactas. El modo `--verify` conserva la ejecución rápida por lotes.

La versión con olfato y memoria tabú estricta fue validada en `16x16` libre con costo óptimo `13`, y en `32x32` con 284 obstáculos con costo ACO `37` frente al óptimo `35`. La validación comprueba además que la ruta no repita ninguna celda.

`PhysarumVulkan` permanece sin modificaciones. Physarum usa vecindad de Moore y esta aplicación usa cuatro vecinos ortogonales.

Consulta comandos, formato `PACGRID 2` y controles completos en [`NetworkACOVulkan/README.md`](../NetworkACOVulkan/README.md).
