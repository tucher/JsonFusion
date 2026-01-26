#!/usr/bin/env bash
# Generates headers-manifest.json listing all JsonFusion + PFR header paths
# relative to include/. Run from project root or this script's directory.
set -euo pipefail
cd "$(dirname "$0")/.."

(
  find include/JsonFusion -type f \( -name '*.hpp' -o -name '*.h' \)
  find include/pfr -type f \( -name '*.hpp' -o -name '*.h' \)
) | sort | jq -R . | jq -s . > clang_emscripten_constexpr_run_tool/headers-manifest.json

echo "Generated headers-manifest.json with $(jq length clang_emscripten_constexpr_run_tool/headers-manifest.json) entries"
