# Network ACO with Vulkan

`NetworkACOVulkan` searches for routes with an ACO colony and reconstructs the influence network among ants. Its implicit rectangular grid supports sizes from `8x8` through `10000x10000` without materializing every node and edge.

The initial scenario is an empty field. Obstacles are stored sparsely and can be toggled manually with `OBSTACULO`/`O`. The mouse wheel zooms and right-button dragging pans the camera.

There is one start and up to 30 goals. `DESTINO`/`G` toggles goals with a click, while `--goal` can be repeated on the command line:

```bash
./NetworkACOVulkan --grid 10000x10000 --start 1,5000 \
  --goal 9998,2500 --goal 9998,7500 --iterations 1200
```

The iteration count accepts values from `1` through `10000`. Set an exact value with `--iterations N`, or change it in steps of 100 from the UI with `-100`/`+100` and the `[`/`]` keys.

## Scalable representation

- Neighbors are calculated directly from `(x,y)`.
- Only existing obstacles are stored, up to `1,048,576`.
- Directional pheromone uses a GPU lattice of at most `128x128x4`.
- Route coordinates retain full grid resolution.
- Every ant keeps a private visit hash bounded by the step budget.
- Each ant explores goal `antIndex % goalCount`.
- `GUIADO` gives each ant up to `4*(width+height)+1024` steps; `SIN GUIA` uses up to `16*(width+height)+4096`, capped at `262144`.

The ant count per generation scales automatically from 30 through 128 based on the grid's longest side: `24x16` uses 30, `1000x600` uses 57, and `10000x10000` uses 128 on a compatible GPU. The count is automatically reduced when `maxStorageBufferRange` cannot hold routes, visit memory, and the complete interaction history.

The interaction network retains one node per active ant. When an ant follows a direction, it records the pheromone previously attributable to every other ant. `I[i][i]` stays zero.

## Independent guidance and smell

The UI retains `GUIADO`/`SIN GUIA`, toggled with `M` or `--guided`/`--unguided`. `OLFATO` is a separate switch controlled by its button, `L`, or `--smell`/`--no-smell`. Changing either control resets the colony and its pheromone.

- In `GUIADO`, moves that reduce Manhattan distance receive the original geometric factor. `SIN GUIA` removes that global knowledge.
- With `OLFATO`, every goal emits the local field `C(n)=max(0,1-d(n,goal)/64)^2` from T1 onward. The signal is exactly zero beyond 64 cells; within that radius the ant favors an increasing gradient. On `1000x1000`, the default start and goal are 997 cells apart, so the start detects no odor. The simplified airborne field crosses obstacles, although obstacles still block movement.

Guidance and smell can be enabled together, separately, or both disabled; their factors multiply and neither replaces the other.

In both modes, a cell already visited by that ant is excluded from the roulette wheel with zero probability. If no new exit remains, the ant deterministically backtracks along its route to another branch; that movement does not enter the roulette wheel. The active route has no cycles, and per-traversal stamps preserved across GPU blocks allow the visit hash to be reused.

Unguided exploration outside the smell radius is intended as an experimental control on small grids. At `10000x10000`, `T1` may require very many blocks and substantial compute time; it remains open until all goals are found or the user intervenes.

Every `T` builds independent stochastic routes in GPU blocks of up to 2048 movements. T1 ends and publishes the first solution as soon as every food target has a route, without waiting for the rest of the colony. Each founder deposits the greater of `(Q/L)*(assigned ants/successful ants)` and `(8*tau0)/successful ants`, so the corridor does not fade as the grid grows. From T2 onward no ant copies the best route: all return to the source and choose every edge with the `tau^alpha * eta^beta` roulette, private taboo memory, and independent randomness. Evaporated pheromone is floored only at one fixed-point quantum, `1/65536`; unused regions therefore stop competing artificially with reinforced routes. Pheromone evaporates only when the generation closes, then every successful route deposits `Q/L`. DFS backtracking neither adds cycles nor enters the roulette wheel. `GUIADO` and `OLFATO` remain independent.

## UI and validation

The `GRAFO`, `RED`, and `METRICAS` views show per-goal routes, temporal influence, and statistics. The optimal reference sums the shortest route to each goal. Manhattan A* returns the same unit-grid optimum as Dijkstra without allocating 100 million distance entries.

The `GRAFO` view animates every active ant as a colored circle along its individual route and displays `HORM N AUTO`, `PASOS N`, and the approximate duration. `GUIADO`, `SIN GUIA`, and `OLFATO` control the two independent factors. When enabled, magenta represents odor and cyan represents pheromone. `X1`, `X5`, and `X20` control speed. When the first solution is found, the winning ant replays its complete route from the source while the gold line grows behind it; T2 waits for this playback to finish. Routes with tens of thousands of turns are downsampled only when drawing the gold polyline; the complete route and its metrics remain intact. `--verify` retains fast batched execution.

The smell-enabled strict-tabu version was validated on an open `16x16` grid with the optimal cost `13`, and on a `32x32` grid with 284 obstacles at ACO cost `37` versus the `35` optimum. Validation also checks that no route repeats a cell.

`PhysarumVulkan` remains unchanged. Physarum uses Moore neighborhoods while this application uses four orthogonal neighbors.

See [`NetworkACOVulkan/README.md`](../NetworkACOVulkan/README.md) for full commands, `PACGRID 2`, and controls.
