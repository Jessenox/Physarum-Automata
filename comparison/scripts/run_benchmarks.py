#!/usr/bin/env python3
"""Run Physarum and Dijkstra over common PACGRID scenarios."""

from __future__ import annotations

import argparse
import csv
import json
import os
import signal
import statistics
import subprocess
import time
from datetime import datetime, timezone
from pathlib import Path
from typing import Any


ROOT = Path(__file__).resolve().parents[2]
SUPPORTED_ALGORITHMS = ("physarum", "dijkstra")


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--manifest", type=Path, default=ROOT / "comparison/generated-10k/manifest.json"
    )
    parser.add_argument("--output", type=Path, default=ROOT / "comparison/results-10k")
    parser.add_argument("--sizes", type=int, nargs="+", default=(10_000,))
    parser.add_argument(
        "--families", nargs="+", default=("free", "cave-system", "paris-catacombs")
    )
    parser.add_argument(
        "--algorithms", nargs="+", choices=SUPPORTED_ALGORITHMS, default=SUPPORTED_ALGORITHMS
    )
    parser.add_argument("--repetitions", type=int, default=3)
    parser.add_argument("--timeout", type=float, default=7200.0, help="Seconds per process")
    parser.add_argument("--physarum-max-generations", type=int, default=0)
    parser.add_argument("--resume", action="store_true")
    return parser.parse_args()


def resolve_manifest_path(manifest: Path, value: str) -> Path:
    path = Path(value)
    if path.is_absolute():
        return path
    candidate = ROOT / path
    return candidate if candidate.exists() else manifest.parent / path


def extract_json(output: str) -> dict[str, Any] | None:
    for line in output.splitlines():
        if line.startswith("BENCHMARK_JSON "):
            return json.loads(line.removeprefix("BENCHMARK_JSON "))
    return None


def run_process(command: list[str], timeout: float, time_file: Path) -> tuple[int, str, float, int | None, str]:
    time_file.unlink(missing_ok=True)
    wrapped = ["/usr/bin/time", "-q", "-f", "%e %M", "-o", str(time_file), *command]
    started = time.perf_counter()
    process = subprocess.Popen(
        wrapped,
        cwd=ROOT,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        text=True,
        start_new_session=True,
    )
    status = "ok"
    try:
        output, _ = process.communicate(timeout=timeout)
    except subprocess.TimeoutExpired:
        status = "timeout"
        os.killpg(process.pid, signal.SIGTERM)
        try:
            output, _ = process.communicate(timeout=5.0)
        except subprocess.TimeoutExpired:
            os.killpg(process.pid, signal.SIGKILL)
            output, _ = process.communicate()
    wall = time.perf_counter() - started
    peak_rss = None
    if time_file.exists():
        values = time_file.read_text(encoding="utf-8").strip().split()
        if len(values) == 2:
            try:
                peak_rss = int(values[1])
            except ValueError:
                pass
    return process.returncode or 0, output, wall, peak_rss, status


def command_for(
    algorithm: str,
    scenario: Path,
    size: int,
    args: argparse.Namespace,
) -> list[str]:
    if algorithm == "dijkstra":
        return [str(ROOT / "comparison/build/DijkstraBenchmark"), "--scenario", str(scenario)]
    if algorithm == "physarum":
        maximum = args.physarum_max_generations or max(600, size * 12)
        return [
            str(ROOT / "comparison/build/PhysarumBenchmark"),
            "--scenario", str(scenario),
            "--shader", str(ROOT / "PhysarumVulkan/build/shaders/physarum.comp.spv"),
            "--max-generations", str(maximum),
        ]
    raise ValueError(f"Unsupported benchmark algorithm: {algorithm}")


def parse_result(algorithm: str, output: str) -> dict[str, Any]:
    if algorithm not in SUPPORTED_ALGORITHMS:
        raise ValueError(f"Unsupported benchmark algorithm: {algorithm}")
    return extract_json(output) or {}


def numeric_mean(rows: list[dict[str, Any]], field: str) -> str:
    values = [float(row[field]) for row in rows if row.get(field) is not None]
    return f"{statistics.mean(values):.4f}" if values else "—"


def numeric_min(rows: list[dict[str, Any]], field: str) -> str:
    values = [float(row[field]) for row in rows if row.get(field) is not None]
    return f"{min(values):.3f}" if values else "—"


def mean_and_deviation(rows: list[dict[str, Any]], field: str) -> str:
    values = [float(row[field]) for row in rows if row.get(field) is not None]
    if not values:
        return "—"
    mean = statistics.mean(values)
    return f"{mean:.4f}" if len(values) == 1 else f"{mean:.4f} ± {statistics.stdev(values):.4f}"


def write_reports(rows: list[dict[str, Any]], output: Path, manifest: Path, args: argparse.Namespace) -> None:
    output.mkdir(parents=True, exist_ok=True)
    fields = sorted({key for row in rows for key in row})
    with (output / "runs.csv").open("w", newline="", encoding="utf-8") as stream:
        writer = csv.DictWriter(stream, fieldnames=fields)
        writer.writeheader()
        writer.writerows(rows)
    with (output / "runs.jsonl").open("w", encoding="utf-8") as stream:
        for row in rows:
            stream.write(json.dumps(row, ensure_ascii=False, sort_keys=True) + "\n")

    grouped: dict[tuple[str, str], list[dict[str, Any]]] = {}
    for row in rows:
        grouped.setdefault((str(row["scenario"]), str(row["algorithm"])), []).append(row)
    labels = {"physarum": "Physarum", "dijkstra": "Dijkstra"}
    title = " / ".join(labels[name] for name in args.algorithms)
    lines = [
        f"# Comparación {title}",
        "",
        f"Generado: `{datetime.now(timezone.utc).isoformat()}`  ",
        f"Manifest: `{manifest}`  ",
        f"Repeticiones solicitadas: `{args.repetitions}`  ",
        f"Timeout por proceso: `{args.timeout}` segundos",
    ]
    lines.extend([
        "",
        "| Escenario | Algoritmo | Éxito | Tiempo proceso (media ± DE, s) | Tiempo algoritmo (media ± DE, s) | Mejor ruta | Brecha media (%) | Pico RSS medio (KiB) |",
        "|---|---:|---:|---:|---:|---:|---:|---:|",
    ])
    for (scenario, algorithm), group in sorted(grouped.items()):
        successes = [row for row in group if row.get("success") is True]
        lines.append(
            f"| {scenario} | {algorithm} | {len(successes)}/{len(group)} | "
            f"{mean_and_deviation(group, 'wall_seconds')} | {mean_and_deviation(successes, 'algorithm_seconds')} | "
            f"{numeric_min(successes, 'route_cost')} | {numeric_mean(successes, 'gap_percent')} | "
            f"{numeric_mean(group, 'peak_rss_kib')} |"
        )
    interpretation = [
        "",
        "## Lectura correcta",
        "",
    ]
    if "dijkstra" in args.algorithms:
        interpretation.append("- Dijkstra usa cuatro vecinos y costo unitario; es la referencia óptima exacta.")
    if "physarum" in args.algorithms:
        interpretation.append("- Physarum ejecuta el shader Vulkan original con vecindad de Moore. `route_cost` usa 1 para movimientos ortogonales y 1.414 para diagonales dentro de la red final; su brecha se calcula contra el óptimo Moore del mismo mapa, no contra Dijkstra de cuatro vecinos.")
    interpretation.extend([
        "- `wall_seconds` incluye carga del proceso. `algorithm_seconds` se informa cuando el ejecutable puede aislar el núcleo.",
        "- `peak_rss_kib` es memoria residente del proceso; no incluye toda la memoria privada de la GPU.",
        "- Un `timeout` o una ejecución sin ruta permanece en el CSV/JSONL; no se elimina del porcentaje de éxito.",
        "",
    ])
    lines.extend(interpretation)
    (output / "summary.md").write_text("\n".join(lines), encoding="utf-8")


def main() -> None:
    args = parse_args()
    if args.repetitions < 1:
        raise SystemExit("--repetitions must be positive")
    manifest_path = args.manifest.resolve()
    manifest = json.loads(manifest_path.read_text(encoding="utf-8"))
    selected = [
        item for item in manifest["maps"]
        if int(item["width"]) in args.sizes and str(item["family"]) in args.families
    ]
    if not selected:
        raise SystemExit("No scenarios match --sizes/--families")

    output = args.output.resolve()
    logs = output / "logs"
    logs.mkdir(parents=True, exist_ok=True)
    rows: list[dict[str, Any]] = []
    jsonl = output / "runs.jsonl"
    if args.resume and jsonl.exists():
        rows = [json.loads(line) for line in jsonl.read_text(encoding="utf-8").splitlines() if line]
    completed = {
        (row["scenario"], row["algorithm"], int(row["repetition"])) for row in rows
    }

    for item in selected:
        scenario_name = str(item["name"])
        scenario_path = resolve_manifest_path(manifest_path, str(item["scenario"])).resolve()
        size = int(item["width"])
        for repetition in range(1, args.repetitions + 1):
            for algorithm in args.algorithms:
                key = (scenario_name, algorithm, repetition)
                if key in completed:
                    continue
                command = command_for(algorithm, scenario_path, size, args)
                stem = f"{scenario_name}__{algorithm}__r{repetition}"
                print(f"[{stem}] {' '.join(command)}", flush=True)
                returncode, text, wall, peak, process_status = run_process(
                    command, args.timeout, logs / f"{stem}.time"
                )
                (logs / f"{stem}.log").write_text(text, encoding="utf-8")
                parsed = parse_result(algorithm, text)
                success = parsed.get("success") is True and returncode == 0 and process_status == "ok"
                row: dict[str, Any] = {
                    "timestamp_utc": datetime.now(timezone.utc).isoformat(),
                    "scenario": scenario_name,
                    "family": item["family"],
                    "width": item["width"],
                    "height": item["height"],
                    "algorithm": algorithm,
                    "repetition": repetition,
                    "success": success,
                    "process_status": process_status if process_status != "ok" else ("ok" if returncode == 0 else "error"),
                    "returncode": returncode,
                    "wall_seconds": round(wall, 6),
                    "peak_rss_kib": peak,
                    "canonical_4n_distance": item.get("canonical_4n_distance"),
                    "scenario_obstacles": item.get("obstacles"),
                    "scenario_free_cells": item.get("free_cells"),
                    "command": command,
                    "log": str(logs / f"{stem}.log"),
                    **parsed,
                }
                row["algorithm"] = algorithm
                if algorithm == "dijkstra" and success:
                    row["optimal_cost"] = row.get("route_cost")
                    row["gap_percent"] = 0.0
                rows.append(row)
                completed.add(key)
                write_reports(rows, output, manifest_path, args)
                print(
                    f"  status={row['process_status']} success={success} wall={wall:.3f}s "
                    f"cost={row.get('route_cost', 'n/a')}", flush=True
                )


if __name__ == "__main__":
    main()
