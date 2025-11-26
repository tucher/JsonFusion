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

# CRITICAL: Run structural detection tests FIRST
# If type detection is broken, all other tests are meaningless
STRUCTURAL_TEST="tests/constexpr/concepts/test_structural_detection.cpp"
if [ -f "$STRUCTURAL_TEST" ]; then
    test_name="test_structural_detection"
    category="concepts"
    printf "%-20s %-20s ... " "$category" "$test_name"
    
    if g++ -std=c++23 -I"$INCLUDE_DIR" -Itests/constexpr -c "$STRUCTURAL_TEST" -o "$TMP_DIR/$test_name.o" 2>&1 | tee "$TMP_DIR/$test_name.log" | grep -q "error:"; then
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
        if grep -q "warning:" "$TMP_DIR/$test_name.log"; then
            echo "⚠️  PASS (with warnings)"
            WARNINGS=$((WARNINGS + 1))
        else
            echo "✅ PASS"
        fi
        PASS=$((PASS + 1))
    fi
fi

# Run all other tests
for test_file in $(find tests/constexpr -name "test_*.cpp" | grep -v "test_structural_detection.cpp" | sort); do
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

