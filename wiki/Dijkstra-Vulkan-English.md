# Dijkstra Vulkan

`DijkstraVulkan` is an interactive shortest-path visualizer for weighted grids. It is a separate application with the same visual composition as `PhysarumVulkan`: a `500x500` canvas, bottom status panel, and dark sidebar.

## Vulkan Compute

Every iteration runs four compute passes: clear the minimum, find the lowest tentative distance in parallel, select one node deterministically, and relax its neighbors. Weights, distances, predecessors, and visited nodes live in SSBOs shared with rendering.

After completion, the path is reconstructed and its cost is checked against a reference CPU Dijkstra implementation. `RUTA GPU VALIDADA` means both results match.

## Build and run

```bash
cd DijkstraVulkan
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
ctest --test-dir build --output-on-failure
./build/DijkstraVulkan
```

To verify the local compute pipeline automatically:

```bash
./build/DijkstraVulkan --grid 16x16 --verify
```

See `DijkstraVulkan/README.md` for all controls and weight details.

## Grid capacity

The dense interactive backend has a safety limit of `1,048,576` nodes, equivalent to `1024x1024`. A `1000x1000` grid is accepted; `10000x10000` is rejected before Vulkan initialization or memory allocation. The console reports the raw GPU buffer capacity separately from the interactive limit.

Each exact Dijkstra step performs three full-grid compute passes. The requested steps per frame are therefore clamped automatically according to the node count, and the UI displays the effective maximum. This prevents the application from monopolizing the GPU used by the desktop.

Rendering also uses FIFO presentation and is capped at `60 FPS`, including while the search is paused.
