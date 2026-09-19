#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"

"$repo_root/comparison/scripts/build_benchmarks.sh"
python3 "$repo_root/comparison/scripts/prepare_maps.py" \
  --output "$repo_root/comparison/generated-10k" \
  --sizes 10000
python3 "$repo_root/comparison/scripts/run_benchmarks.py" \
  --manifest "$repo_root/comparison/generated-10k/manifest.json" \
  --output "$repo_root/comparison/results-10k" \
  --sizes 10000 \
  --algorithms physarum dijkstra \
  --timeout 7200 \
  "$@"
