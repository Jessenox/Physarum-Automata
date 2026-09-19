# User Guide

Language: [[Espanol|Guia-de-uso]] | **English**

This guide assumes `PhysarumVulkan` is already built. If not, read [[Installation-and-Build]].

## Open the simulator

From `PhysarumVulkan/build`:

```bash
./PhysarumVulkan
```

With a specific grid:

```bash
./PhysarumVulkan --grid 512x512
```

## First exercise: simple route

Use this flow to understand the system:

1. Open the simulator.
2. Press `4` to select `START`.
3. Click a cell on the canvas.
4. Press `2` to select `NUTR NO`.
5. Click another cell away from the start.
6. Press `3` if you want to place `REPELLENT` obstacles.
7. Press `Enter` to start.
8. Watch the expansion front search for the nutrient.
9. Press `Enter` again to pause.

Note: keys `1` to `9` select states `0` to `8`. Therefore `4` selects state `3 START`.

## Main controls

| Control | Action |
| --- | --- |
| `1` to `9` | Select states `0` to `8`. |
| `Enter` | Pause or resume simulation. |
| Left mouse | Paint on the grid. |
| Left mouse on menu | Activate buttons or change selection. |
| `Ctrl + scroll` | Zoom centered on cursor. |
| Middle mouse drag | Pan the view. |
| `Ctrl + left click drag` | Alternative pan. |
| `R` | Reset zoom and pan. |
| `F1` | Change to `200x200`. |
| `F2` | Change to `512x512`. |
| `F3` | Change to `1024x1024`. |
| `F4` | Change to `4000x4000`. |

Changing grid size pauses the simulation, resets generation to `0`, and recreates the Vulkan texture if needed.

## Available states

| Key | State | UI name | Common use |
| --- | --- | --- | --- |
| `1` | `0` | `FREE` | Erase or keep open space. |
| `2` | `1` | `NUTR NO` | Place destination. |
| `3` | `2` | `REPELLENT` | Place wall or obstacle. |
| `4` | `3` | `START` | Place origin. |
| `5` | `4` | `GEL CONT` | Internal/intermediate state. |
| `6` | `5` | `GEL COMP` | Internal/route state. |
| `7` | `6` | `NUTR OK` | Nutrient already found. |
| `8` | `7` | `EXPANSION` | Growth front. |
| `9` | `8` | `GEL SIN` | Transitional state. |

New users usually only need `START`, `NUTR NO`, `REPELLENT`, and `FREE`.

## Side menu

The side menu allows operation without remembering all keyboard controls:

- **CARGAR MAPA**: opens a file picker and converts an image into obstacles.
- **ATRACTORES**: generates an attractor graph for a subgrid.
- **REFINAR**: adds samples to the approximate attractor graph.
- **EXPORT SVG**: saves the last attractor as a vector file.
- **EXPORT PNG**: saves the last attractor as an image.
- **ESTADOS**: selects one of the nine states.
- **COLOR RGB**: changes the selected state color.
- **ATR WxH**: adjusts attractor subgrid width and height.

## Load a map

1. Press **CARGAR MAPA**.
2. Choose an image.
3. The system converts it to grayscale.
4. Dark areas are detected as obstacles.
5. Those obstacles become `REPELLENT`.
6. External borders are forced as walls.

After loading a map, place `START` and `NUTR NO` on free areas.

## Use attractors

1. Adjust `ATR W` and `ATR H`.
2. Press **ATRACTORES**.
3. Wait for the graph window.
4. If the mode is approximate, use **REFINAR** to explore more samples.
5. Export with **EXPORT SVG** or **EXPORT PNG**.

Recommendation: start with `2x2` or `3x3`. State spaces grow very quickly.

## Common problems

| Problem | Likely cause | Fix |
| --- | --- | --- |
| File picker does not open | Missing `zenity`, `kdialog`, or `tkinter`. | Install one of them. |
| `CARGAR MAPA` reports OpenCV disabled | CMake did not find OpenCV. | Install `libopencv-dev` and rebuild. |
| No route is found | Obstacles block the path or start/nutrient is missing. | Check initial states. |
| Mouse paints wrong cell | Old build or window scaling issue. | Rebuild and press `R`. |
| Large grid is slow | Too many cells or limited hardware. | Try `512x512` or smaller. |

