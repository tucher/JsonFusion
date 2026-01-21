#!/bin/bash
# Run all benchmark platforms and update README automatically

set -e  # Exit on error

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

echo "========================================"
echo "Running All Code Size Benchmarks"
echo "========================================"
echo

# Run ARM benchmarks
echo "🔨 Building ARM Cortex-M..."
python3 build.py --platform arm
echo

# Run ESP32 benchmarks
echo "🔨 Building ESP32..."
python3 build.py --platform esp32
echo

# Update README with results
echo "📝 Updating README.md..."
python3 update_readme.py
echo

echo "✅ All benchmarks complete and README updated!"
echo
echo "To commit changes:"
echo "  git add README.md"
echo "  git commit -m 'Update benchmark results'"
