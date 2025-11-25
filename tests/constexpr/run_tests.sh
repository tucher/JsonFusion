#!/bin/bash
# Constexpr test runner - compiles each test file to verify static_assert passes

set -e  # Exit on first failure

cd "$(dirname "$0")/../.."
INCLUDE_DIR="./include"
TMP_DIR="/tmp/jsonfusion_constexpr_tests"
mkdir -p "$TMP_DIR"

echo "Running JsonFusion constexpr tests..."
echo "======================================="

PASS=0
FAIL=0
WARNINGS=0

for test_file in $(find tests/constexpr -name "test_*.cpp" | sort); do
    test_name=$(basename "$test_file" .cpp)
    category=$(basename $(dirname "$test_file"))
    printf "%-20s %-20s ... " "$category" "$test_name"
    
    if g++ -std=c++23 -I"$INCLUDE_DIR" -Itests/constexpr -c "$test_file" -o "$TMP_DIR/$test_name.o" 2>&1 | tee "$TMP_DIR/$test_name.log" | grep -q "error:"; then
        echo "❌ FAIL"
        cat "$TMP_DIR/$test_name.log"
        FAIL=$((FAIL + 1))
    else
        if grep -q "warning:" "$TMP_DIR/$test_name.log"; then
            echo "⚠️  PASS (with warnings)"
            WARNINGS=$((WARNINGS + 1))
        else
            echo "✅ PASS"
        fi
        PASS=$((PASS + 1))
    fi
done

echo "======================================="
echo "Results: $PASS passed, $FAIL failed"
if [ $WARNINGS -gt 0 ]; then
    echo "         $WARNINGS tests passed with warnings"
fi

if [ $FAIL -eq 0 ]; then
    echo "✅ All tests passed!"
    exit 0
else
    echo "❌ Some tests failed"
    exit 1
fi

