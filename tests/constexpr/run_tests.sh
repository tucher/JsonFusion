#!/bin/bash
# Constexpr test runner - compiles each test file to verify static_assert passes

set -e  # Exit on first failure

cd "$(dirname "$0")/../.."
INCLUDE_DIR="./include"
TMP_DIR="/tmp/jsonfusion_constexpr_tests"
mkdir -p "$TMP_DIR"

echo "Running JsonFusion constexpr tests..."
echo "======================================="

: ${CXX:=g++}
$CXX -v

PASS=0
FAIL=0
WARNINGS=0

# CRITICAL: Run structural detection tests FIRST
# If type detection is broken, all other tests are meaningless
STRUCTURAL_TEST="tests/constexpr/concepts/test_structural_detection.cpp"
if [ -f "$STRUCTURAL_TEST" ]; then
    test_name="test_structural_detection"
    category="concepts"
    printf "%-20s %-20s ... " "$category" "$test_name"
    
    if $CXX -std=c++23 -I"$INCLUDE_DIR" -Itests/constexpr -c "$STRUCTURAL_TEST" -o "$TMP_DIR/$test_name.o" 2>&1 | tee "$TMP_DIR/$test_name.log" | grep -q "error:"; then
        echo "❌ FAIL"
        echo ""
        echo "============================================================"
        echo "CRITICAL FAILURE: Type detection/classification is broken!"
        echo "All other tests are meaningless until this is fixed."
        echo "============================================================"
        echo ""
        cat "$TMP_DIR/$test_name.log"
        exit 1
    else
        if ! grep -q "warning:" "$TMP_DIR/$test_name.log" | grep -qv "overriding deployment version from"; then
            echo "✅ PASS"
        else
            echo "⚠️  PASS (with warnings)"
            WARNINGS=$((WARNINGS + 1))
        fi
        PASS=$((PASS + 1))
    fi
fi

# Detect number of CPUs
if command -v nproc > /dev/null 2>&1; then
    NPROCS=$(nproc)
elif command -v sysctl > /dev/null 2>&1; then
    NPROCS=$(sysctl -n hw.ncpu)
else
    NPROCS=4  # fallback
fi

echo ""
echo "Running remaining tests in parallel (using $NPROCS CPUs)..."
echo ""

# Function to run a single test (will be run in parallel)
run_test() {
    test_file="$1"
    test_name=$(basename "$test_file" .cpp)
    category=$(basename $(dirname "$test_file"))
    
    if $CXX -std=c++23 -I"$INCLUDE_DIR" -Itests/constexpr -c "$test_file" -o "$TMP_DIR/$test_name.o" 2>&1 > "$TMP_DIR/$test_name.log" 2>&1; then
        if ! grep -q "warning:" "$TMP_DIR/$test_name.log" | grep -qv "overriding deployment version from"; then
            echo "PASS|$category|$test_name"
        else
            echo "WARN|$category|$test_name"
        fi
    else
        echo "FAIL|$category|$test_name"
        cat "$TMP_DIR/$test_name.log" > "$TMP_DIR/$test_name.errors"
    fi
}

export -f run_test
export INCLUDE_DIR
export TMP_DIR
export CXX
# Run all other tests in parallel
find tests/constexpr -name "test_*.cpp" | grep -v "test_structural_detection.cpp" | sort | xargs -P "$NPROCS" -I {} bash -c 'run_test "$@"' _ {} > "$TMP_DIR/parallel_results.txt"

# Process results
while IFS='|' read -r status category test_name; do
    printf "%-20s %-20s ... " "$category" "$test_name"
    case "$status" in
        PASS)
            echo "✅ PASS"
            PASS=$((PASS + 1))
            ;;
        WARN)
            echo "⚠️  PASS (with warnings)"
            PASS=$((PASS + 1))
            WARNINGS=$((WARNINGS + 1))
            ;;
        FAIL)
            echo "❌ FAIL"
            FAIL=$((FAIL + 1))
            if [ -f "$TMP_DIR/$test_name.errors" ]; then
                cat "$TMP_DIR/$test_name.errors"
            fi
            ;;
    esac
done < "$TMP_DIR/parallel_results.txt"

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

