#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
jobs="${JOBS:-$(nproc)}"

cmake -S "$repo_root/PhysarumVulkan" -B "$repo_root/PhysarumVulkan/build" -DCMAKE_BUILD_TYPE=Release
cmake --build "$repo_root/PhysarumVulkan/build" -j "$jobs"

cmake -S "$repo_root/comparison" -B "$repo_root/comparison/build" -DCMAKE_BUILD_TYPE=Release
cmake --build "$repo_root/comparison/build" -j "$jobs"
