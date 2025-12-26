#!/usr/bin/env bash
set -euo pipefail

# Where to put build dir
BUILD_DIR="${BUILD_DIR:-build}"
DATA_DIR="${DATA_DIR:-bench_data}"

mkdir -p "$BUILD_DIR"
cmake -S . -B "$BUILD_DIR" -DCMAKE_BUILD_TYPE=Release
cmake --build "$BUILD_DIR" --config Release -j"$(nproc)"

# Get twitter.json / canada.json if you don't want them in the repo
mkdir -p "$DATA_DIR"
if [ ! -f "$DATA_DIR/twitter.json" ]; then
  curl -L -o "$DATA_DIR/twitter.json" \
    https://raw.githubusercontent.com/miloyip/nativejson-benchmark/master/data/twitter.json
fi

if [ ! -f "$DATA_DIR/canada.json" ]; then
  curl -L -o "$DATA_DIR/canada.json" \
    https://raw.githubusercontent.com/miloyip/nativejson-benchmark/master/data/canada.json
fi

pushd "$BUILD_DIR" >/dev/null

echo "=== Running micro-benchmarks ==="
./benchmarks

echo
echo "=== Running twitter.json benchmark ==="
./twitter_json_parsing "../$DATA_DIR/twitter.json"

echo
echo "=== Running canada.json benchmark ==="
./canada_json_parsing "../$DATA_DIR/canada.json"

popd >/dev/null