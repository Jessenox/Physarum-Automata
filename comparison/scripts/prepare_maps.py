#!/usr/bin/env python3
"""Create common PACGRID scenarios from the article's cave/catacomb images."""

from __future__ import annotations

import argparse
import hashlib
import json
import subprocess
from collections import deque
from pathlib import Path

import cv2
import numpy as np


DEFAULT_DOWNLOADS = Path("/home/eduardohv/Descargas")
DEFAULT_SIZES = (10_000,)
FAMILIES = ("free", "cave-system", "paris-catacombs")


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest()


def largest_component(mask: np.ndarray) -> np.ndarray:
    interior = mask.astype(np.uint8).copy()
    interior[[0, -1], :] = 0
    interior[:, [0, -1]] = 0
    count, labels, stats, _ = cv2.connectedComponentsWithStats(interior, connectivity=4)
    if count <= 1:
        raise RuntimeError("Map preprocessing produced no traversable component")
    label = 1 + int(np.argmax(stats[1:, cv2.CC_STAT_AREA]))
    return labels == label


def farthest(mask: np.ndarray, origin: tuple[int, int]) -> tuple[tuple[int, int], np.ndarray]:
    height, width = mask.shape
    distance = np.full(height * width, -1, dtype=np.int32)
    origin_index = origin[1] * width + origin[0]
    distance[origin_index] = 0
    queue: deque[int] = deque([origin_index])
    farthest_index = origin_index
    while queue:
        current = queue.popleft()
        x = current % width
        y = current // width
        next_distance = int(distance[current]) + 1
        for next_index in (
            current - 1 if x > 0 else -1,
            current + 1 if x + 1 < width else -1,
            current - width if y > 0 else -1,
            current + width if y + 1 < height else -1,
        ):
            if next_index < 0 or distance[next_index] >= 0:
                continue
            ny, nx = divmod(next_index, width)
            if not mask[ny, nx]:
                continue
            distance[next_index] = next_distance
            queue.append(next_index)
            if next_distance > distance[farthest_index]:
                farthest_index = next_index
    return (farthest_index % width, farthest_index // width), distance.reshape(mask.shape)


def diameter_endpoints(mask: np.ndarray) -> tuple[tuple[int, int], tuple[int, int], int]:
    y, x = np.argwhere(mask)[0]
    first, _ = farthest(mask, (int(x), int(y)))
    second, distances = farthest(mask, first)
    return first, second, int(distances[second[1], second[0]])


def nearest_free(mask: np.ndarray, point: tuple[int, int]) -> tuple[int, int]:
    height, width = mask.shape
    center_x = min(width - 1, max(0, point[0]))
    center_y = min(height - 1, max(0, point[1]))
    if mask[center_y, center_x]:
        return center_x, center_y
    radius = 1
    while radius < max(width, height):
        left = max(0, center_x - radius)
        right = min(width, center_x + radius + 1)
        top = max(0, center_y - radius)
        bottom = min(height, center_y + radius + 1)
        candidates = np.argwhere(mask[top:bottom, left:right])
        if candidates.size:
            candidates[:, 0] += top
            candidates[:, 1] += left
            distance = ((candidates[:, 1] - center_x) ** 2
                        + (candidates[:, 0] - center_y) ** 2)
            y, x = candidates[int(np.argmin(distance))]
            return int(x), int(y)
        radius *= 2
    raise RuntimeError("Could not project an endpoint onto the traversable component")


def scalable_endpoints(mask: np.ndarray) -> tuple[tuple[int, int], tuple[int, int], int | None]:
    """Find distant endpoints without a Python BFS over 100 million cells."""
    height, width = mask.shape
    if max(width, height) <= 2_000:
        return diameter_endpoints(mask)
    reference_width = min(1_000, width)
    reference_height = min(1_000, height)
    reduced = cv2.resize(
        mask.astype(np.uint8), (reference_width, reference_height), interpolation=cv2.INTER_AREA
    ) >= 0.25
    reduced = largest_component(reduced)
    first, second, _ = diameter_endpoints(reduced)

    def project(point: tuple[int, int]) -> tuple[int, int]:
        x = int(round((point[0] + 0.5) * width / reference_width - 0.5))
        y = int(round((point[1] + 0.5) * height / reference_height - 0.5))
        return nearest_free(mask, (x, y))

    # The exact four-neighbour cost is produced later by Dijkstra itself.
    return project(first), project(second), None


def free_map(size: int) -> tuple[np.ndarray, tuple[int, int], tuple[int, int]]:
    mask = np.ones((size, size), dtype=bool)
    mask[[0, -1], :] = False
    mask[:, [0, -1]] = False
    return mask, (1, size // 2), (size - 2, size // 2)


def cave_map(image: np.ndarray, size: int) -> np.ndarray:
    source_height, source_width = image.shape[:2]
    gray = cv2.cvtColor(image, cv2.COLOR_BGR2GRAY)
    gray = cv2.resize(gray, (size, size), interpolation=cv2.INTER_AREA)

    # Hand-traced outer envelope of the Ponderosa Cave System drawing. It
    # rejects the white paper surrounding the cave while retaining the
    # internal breakdown, ledges and vegetation as obstacles.
    polygon = np.array([
        (18, 260), (29, 214), (78, 204), (150, 222), (226, 202),
        (315, 184), (354, 151), (370, 76), (398, 45), (470, 62),
        (565, 39), (628, 18), (660, 45), (636, 76), (570, 105),
        (544, 151), (571, 229), (554, 304), (478, 331), (398, 349),
        (352, 367), (301, 341), (252, 310), (191, 327), (132, 348),
        (72, 322), (28, 304),
    ], dtype=np.float32)
    polygon[:, 0] *= size / source_width
    polygon[:, 1] *= size / source_height
    envelope = np.zeros((size, size), dtype=np.uint8)
    cv2.fillPoly(envelope, [np.rint(polygon).astype(np.int32)], 1)

    dark = (gray < 150).astype(np.uint8)
    count, labels, stats, _ = cv2.connectedComponentsWithStats(dark, connectivity=8)
    minimum_ink_area = max(1, size * size // 35_000)
    obstacles = np.zeros_like(dark)
    for label in range(1, count):
        if stats[label, cv2.CC_STAT_AREA] >= minimum_ink_area:
            obstacles[labels == label] = 1
    free = (envelope != 0) & (obstacles == 0)
    return largest_component(free)


def extract_catacomb_alpha(xcf_path: Path) -> np.ndarray:
    command = [
        "convert", f"{xcf_path}[1]", "-alpha", "extract", "-resize", "1400x1400", "png:-"
    ]
    completed = subprocess.run(command, check=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    image = cv2.imdecode(np.frombuffer(completed.stdout, dtype=np.uint8), cv2.IMREAD_GRAYSCALE)
    if image is None:
        raise RuntimeError(f"ImageMagick could not extract the tunnel layer from {xcf_path}")
    tunnel_pixels = np.argwhere(image < 250)
    if tunnel_pixels.size == 0:
        raise RuntimeError("Paris Catacombs alpha layer contains no tunnel pixels")
    top, left = tunnel_pixels.min(axis=0)
    bottom, right = tunnel_pixels.max(axis=0)
    margin = 12
    return image[
        max(0, top - margin): min(image.shape[0], bottom + margin + 1),
        max(0, left - margin): min(image.shape[1], right + margin + 1),
    ]


def catacomb_map(alpha: np.ndarray, size: int) -> np.ndarray:
    resized = cv2.resize(alpha, (size, size), interpolation=cv2.INTER_AREA)
    tunnels = (resized < 245).astype(np.uint8)
    radius = size // 500
    if radius > 0:
        kernel = cv2.getStructuringElement(cv2.MORPH_ELLIPSE, (radius * 2 + 1, radius * 2 + 1))
        tunnels = cv2.dilate(tunnels, kernel)
    return largest_component(tunnels != 0)


def write_pacgrid(
    path: Path,
    free: np.ndarray,
    start: tuple[int, int],
    goal: tuple[int, int],
    seed: int,
) -> tuple[int, int, int]:
    height, width = free.shape
    obstacle_count = int(free.size - np.count_nonzero(free))
    use_runs = free.size > 4_000_000
    with path.open("w", encoding="utf-8") as stream:
        stream.write(f"PACGRID {3 if use_runs else 2}\n")
        stream.write(f"size {width} {height}\n")
        stream.write(f"start {start[0]} {start[1]}\n")
        stream.write("goals 1\n")
        stream.write(f"{goal[0]} {goal[1]}\n")
        stream.write(f"seed {seed}\n")
        stream.write(f"obstacles {obstacle_count}\n")
        if not use_runs:
            obstacles = np.argwhere(~free)
            for y, x in obstacles:
                stream.write(f"{int(x)} {int(y)}\n")
            return obstacle_count, 2, obstacle_count

        blocked = ~free
        run_count = 0
        for row in blocked:
            run_count += int(row[0]) + int(np.count_nonzero((~row[:-1]) & row[1:]))
        stream.write(f"runs {run_count}\n")
        for y, row in enumerate(blocked):
            transitions = np.diff(np.pad(row.astype(np.int8), (1, 1)))
            starts = np.flatnonzero(transitions == 1)
            ends = np.flatnonzero(transitions == -1)
            for first_x, end_x in zip(starts, ends, strict=True):
                stream.write(f"{y} {int(first_x)} {int(end_x - first_x)}\n")
    return obstacle_count, 3, run_count


def write_preview(path: Path, free: np.ndarray, start: tuple[int, int], goal: tuple[int, int]) -> None:
    height, width = free.shape
    scale = min(1.0, 2_000.0 / max(width, height))
    preview_width = max(1, int(round(width * scale)))
    preview_height = max(1, int(round(height * scale)))
    preview_free = cv2.resize(
        free.astype(np.uint8), (preview_width, preview_height), interpolation=cv2.INTER_AREA
    ) >= 0.25
    preview = np.zeros((preview_height, preview_width, 3), dtype=np.uint8)
    preview[preview_free] = (245, 245, 245)
    preview_start = (int(round(start[0] * scale)), int(round(start[1] * scale)))
    preview_goal = (int(round(goal[0] * scale)), int(round(goal[1] * scale)))
    radius = max(1, preview_height // 180)
    cv2.circle(preview, preview_start, radius, (40, 220, 40), -1)
    cv2.circle(preview, preview_goal, radius, (40, 40, 240), -1)
    cv2.imwrite(str(path), preview)


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--downloads", type=Path, default=DEFAULT_DOWNLOADS)
    parser.add_argument("--output", type=Path, default=Path("comparison/generated-10k"))
    parser.add_argument("--sizes", type=int, nargs="+", default=DEFAULT_SIZES)
    parser.add_argument("--families", nargs="+", choices=FAMILIES, default=FAMILIES)
    parser.add_argument("--seed", type=int, default=12345)
    return parser.parse_args()


def main() -> None:
    args = parse_args()
    if any(size < 8 or size > 10_000 for size in args.sizes):
        raise SystemExit("Prepared comparison sizes must be between 8 and 10000")
    scenario_dir = args.output / "scenarios"
    preview_dir = args.output / "previews"
    scenario_dir.mkdir(parents=True, exist_ok=True)
    preview_dir.mkdir(parents=True, exist_ok=True)

    cave_path = args.downloads / "cave.png"
    catacomb_path = args.downloads / "Mapa catacumbas.xcf"
    cave = None
    alpha = None
    if "cave-system" in args.families:
        cave = cv2.imread(str(cave_path), cv2.IMREAD_COLOR)
        if cave is None:
            raise SystemExit(f"Could not load {cave_path}")
    if "paris-catacombs" in args.families:
        if not catacomb_path.exists():
            raise SystemExit(f"Could not load {catacomb_path}")
        alpha = extract_catacomb_alpha(catacomb_path)

    metadata: list[dict[str, object]] = []
    for family in args.families:
        for size in args.sizes:
            if family == "free":
                free, start, goal = free_map(size)
                source = None
                source_hash = None
                canonical_distance = abs(goal[0] - start[0]) + abs(goal[1] - start[1])
            elif family == "cave-system":
                assert cave is not None
                free = cave_map(cave, size)
                start, goal, canonical_distance = scalable_endpoints(free)
                source = str(cave_path)
                source_hash = sha256(cave_path)
            else:
                assert alpha is not None
                free = catacomb_map(alpha, size)
                start, goal, canonical_distance = scalable_endpoints(free)
                source = str(catacomb_path)
                source_hash = sha256(catacomb_path)

            name = f"{family}-{size}x{size}"
            scenario_path = scenario_dir / f"{name}.pacgrid"
            obstacle_count, pacgrid_version, obstacle_runs = write_pacgrid(
                scenario_path, free, start, goal, args.seed
            )
            write_preview(preview_dir / f"{name}.png", free, start, goal)
            item = {
                "name": name,
                "family": family,
                "width": size,
                "height": size,
                "start": list(start),
                "goal": list(goal),
                "canonical_4n_distance": canonical_distance,
                "free_cells": int(free.sum()),
                "obstacles": obstacle_count,
                "pacgrid_version": pacgrid_version,
                "obstacle_runs": obstacle_runs,
                "source_image": source,
                "source_sha256": source_hash,
                "scenario": str(scenario_path),
                "preview": str(preview_dir / f"{name}.png"),
            }
            metadata.append(item)
            print(
                f"prepared {name}: free={item['free_cells']} obstacles={obstacle_count} "
                f"start={start} goal={goal} reference={canonical_distance}"
            )

    article_path = args.downloads / "main.pdf"
    article = {
        "path": str(article_path),
        "sha256": sha256(article_path),
    } if article_path.exists() else None
    with (args.output / "manifest.json").open("w", encoding="utf-8") as stream:
        json.dump({"version": 1, "article": article, "maps": metadata}, stream,
                  indent=2, ensure_ascii=False)
        stream.write("\n")


if __name__ == "__main__":
    main()
