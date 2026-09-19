# Guia de uso

Esta guia asume que ya compilaste `PhysarumVulkan`. Si aun no lo hiciste, ve a [[Instalacion-y-build]].

## Abrir el simulador

Desde `PhysarumVulkan/build`:

```bash
./PhysarumVulkan
```

Con grilla especifica:

```bash
./PhysarumVulkan --grid 512x512
```

## Primer ejercicio: ruta simple

Este flujo es el recomendado para entender el sistema.

1. Abre el simulador.
2. Presiona `4` para seleccionar `INICIO`.
3. Haz clic en una celda del lienzo.
4. Presiona `2` para seleccionar `NUTR NO`.
5. Haz clic en otra celda separada del inicio.
6. Presiona `3` si quieres poner obstaculos `REPELENTE`.
7. Presiona `Enter` para iniciar.
8. Observa como el frente de expansion busca el nutriente.
9. Presiona `Enter` de nuevo para pausar.

Nota: las teclas `1` a `9` seleccionan estados `0` a `8`. Por eso `4` selecciona el estado `3 INICIO`.

## Controles principales

| Control | Accion |
| --- | --- |
| `1` a `9` | Seleccionan estados `0` a `8`. |
| `Enter` | Pausa o reanuda la simulacion. |
| `Mouse izquierdo` | Pinta sobre la grilla. |
| `Mouse izquierdo` en menu | Activa botones o cambia seleccion. |
| `Ctrl + scroll` | Zoom centrado sobre el cursor. |
| `Mouse medio + drag` | Desplaza la vista. |
| `Ctrl + clic izquierdo + drag` | Desplazamiento alternativo. |
| `R` | Reinicia zoom y desplazamiento. |
| `F1` | Cambia a `200x200`. |
| `F2` | Cambia a `512x512`. |
| `F3` | Cambia a `1024x1024`. |
| `F4` | Cambia a `4000x4000`. |

Cuando se cambia el tamanio de la grilla, la simulacion se pausa, la generacion vuelve a `0` y la textura Vulkan se recrea si hace falta.

## Estados disponibles

| Tecla | Estado | Nombre | Uso comun |
| --- | --- | --- | --- |
| `1` | `0` | `LIBRE` | Borrar o dejar espacio abierto. |
| `2` | `1` | `NUTR NO` | Colocar destino. |
| `3` | `2` | `REPELENTE` | Colocar pared u obstaculo. |
| `4` | `3` | `INICIO` | Colocar origen. |
| `5` | `4` | `GEL CONT` | Estado interno/intermedio. |
| `6` | `5` | `GEL COMP` | Estado interno/ruta. |
| `7` | `6` | `NUTR OK` | Nutriente ya encontrado. |
| `8` | `7` | `EXPANSION` | Frente de crecimiento. |
| `9` | `8` | `GEL SIN` | Estado transitorio. |

Para un usuario nuevo normalmente solo hacen falta `INICIO`, `NUTR NO`, `REPELENTE` y `LIBRE`.

## Menu lateral

El menu lateral permite operar sin recordar todo el teclado:

- **CARGAR MAPA**: abre un selector de archivo y convierte una imagen en obstaculos.
- **ATRACTORES**: genera un grafo de atractores para una subrejilla.
- **REFINAR**: agrega mas muestras al atractor aproximado.
- **EXPORT SVG**: guarda el ultimo atractor como vector.
- **EXPORT PNG**: guarda el ultimo atractor como imagen.
- **ESTADOS**: permite seleccionar uno de los nueve estados.
- **COLOR RGB**: cambia el color del estado seleccionado.
- **ATR WxH**: ajusta ancho y alto de la subrejilla para atractores.

## Cargar un mapa

1. Presiona **CARGAR MAPA**.
2. Elige una imagen.
3. El sistema la convierte a escala de grises.
4. Detecta zonas oscuras como obstaculos.
5. Convierte esos obstaculos a `REPELENTE`.
6. Fuerza los bordes como paredes.

Despues de cargar el mapa, coloca `INICIO` y `NUTR NO` sobre zonas libres.

## Usar atractores

1. Ajusta `ATR W` y `ATR H`.
2. Presiona **ATRACTORES**.
3. Espera a que aparezca la ventana del grafo.
4. Si el modo es aproximado, usa **REFINAR** para explorar mas.
5. Exporta con **EXPORT SVG** o **EXPORT PNG**.

Recomendacion: empieza con `2x2` o `3x3`. Los espacios de estados crecen muy rapido.

## Problemas comunes

| Problema | Causa probable | Solucion |
| --- | --- | --- |
| No abre el selector de archivos | Falta `zenity`, `kdialog` o `tkinter`. | Instala alguno o usa una build con soporte. |
| `CARGAR MAPA` reporta OpenCV apagado | CMake no encontro OpenCV. | Instala `libopencv-dev` y recompila. |
| La simulacion no encuentra ruta | Obstaculos bloquean totalmente el camino o falta nutriente/inicio. | Revisa estados iniciales. |
| El mouse pinta en lugar equivocado | Build vieja o problema de escalado/ventana. | Recompila y prueba `R`. |
| La grilla grande va lenta | Demasiadas celdas o hardware limitado. | Prueba `512x512` o menor. |

## Validacion rapida

Al iniciar una build nueva, valida:

1. el panel inferior se ve abajo;
2. el menu lateral se ve a la derecha;
3. el clic izquierdo pinta la celda bajo el cursor;
4. `Ctrl + scroll` hace zoom sobre el punto correcto;
5. `Mouse medio + drag` mueve la vista sin pintar;
6. `F1` a `F4` cambian el tamanio de grilla.

