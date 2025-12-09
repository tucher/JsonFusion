#!/bin/bash
# Build script for ARM Cortex-M7 embedded binary size benchmark
# Measures JsonFusion's code footprint on a realistic embedded target

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$SCRIPT_DIR/../../.."

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo -e "${GREEN}=== Embedded Binary Size Benchmark ===${NC}"
echo "Target: ARM Cortex-M7 (Thumb-2, Hardware FP)"
echo "Comparing: JsonFusion vs ArduinoJson"
echo ""

# ARM Cortex-M7 compilation flags
ARM_FLAGS=(
    -mcpu=cortex-m7
    -mthumb
    -mfloat-abi=hard
    -mfpu=fpv5-d16
)

# Embedded-friendly flags
EMBEDDED_FLAGS=(
    -fno-exceptions
    -fno-rtti
    -ffunction-sections
    -fdata-sections
    -DNDEBUG                           # Disable assertions
    -fno-unwind-tables                 # Remove exception unwinding tables
    -fno-asynchronous-unwind-tables    # Remove async unwind info
)

# Include paths
INCLUDES=(
    -I"$PROJECT_ROOT/include"
    -I"$SCRIPT_DIR"  # For ArduinoJson.h
)

# Build configurations to test (name:flags pairs)
CONFIGS=(
    "aggressive_opt:-O3 -flto"
    "minimal_size:-Os -flto"
)

echo "Building configurations..."
echo ""

# Array to store results for comparison
declare -a RESULTS

for config in "${CONFIGS[@]}"; do
    config_name="${config%%:*}"
    opt_flags="${config#*:}"
    
    echo -e "${YELLOW}========================================${NC}"
    echo -e "${YELLOW}Configuration: $config_name${NC}"
    echo -e "${YELLOW}Flags: $opt_flags${NC}"
    echo -e "${YELLOW}========================================${NC}"
    echo ""
    
    # Build JsonFusion version
    output_o="parse_config_jsonfusion_${config_name}.o"
    output_elf="parse_config_jsonfusion_${config_name}.elf"
    
    echo -e "${GREEN}[1/2] Building JsonFusion version...${NC}"
    
    # Step 1: Compile to object file
    echo "Compiling to object file..."
    arm-none-eabi-g++ \
        -std=c++23 \
        "${ARM_FLAGS[@]}" \
        "${EMBEDDED_FLAGS[@]}" \
        $opt_flags \
        "${INCLUDES[@]}" \
        -c "$SCRIPT_DIR/parse_config.cpp" \
        -o "$SCRIPT_DIR/$output_o"
    
    if [ $? -ne 0 ]; then
        echo -e "${RED}✗ Compilation failed${NC}"
        exit 1
    fi
    
    # Step 2: Link to ELF (this is where LTO generates actual code)
    echo "Linking to ELF..."
    map_file="parse_config_${config_name}.map"
    arm-none-eabi-g++ \
        "${ARM_FLAGS[@]}" \
        "${EMBEDDED_FLAGS[@]}" \
        $opt_flags \
        -specs=nano.specs -specs=nosys.specs \
        -Wl,--gc-sections \
        -Wl,-Map="$SCRIPT_DIR/$map_file" \
        "$SCRIPT_DIR/$output_o" \
        -o "$SCRIPT_DIR/$output_elf" 2>&1 | grep -v -e "is not implemented and will always fail" -e "the message above does not take linker garbage collection into account" -e "thumb/v7e-m+dp/hard/libc_nano.a" || true
    
    if [ $? -eq 0 ]; then
        echo -e "${GREEN}✓ Built: $output_elf${NC}"
        
        # Analyze the ELF file
        echo ""
        echo "=== Size Analysis ==="
        arm-none-eabi-size "$SCRIPT_DIR/$output_elf"
        
        echo ""
        echo "=== Detailed Size (by section) ==="
        arm-none-eabi-size -A "$SCRIPT_DIR/$output_elf" | grep -E "section|\.text|\.rodata|\.data|\.bss|Total"
        
        # Show top symbols by size (demangled)
        echo ""
        echo "=== Top 10 Symbols by Size ==="
        arm-none-eabi-nm --size-sort --reverse-sort --radix=d "$SCRIPT_DIR/$output_elf" | \
            grep -v " [bB] " | \
            head -10 | \
            arm-none-eabi-c++filt
        
        # Store JsonFusion size for comparison
        jf_text=$(arm-none-eabi-size "$SCRIPT_DIR/$output_elf" | tail -1 | awk '{print $1}')
        
        echo ""
        echo ""
    else
        echo -e "${RED}✗ JsonFusion linking failed${NC}"
        exit 1
    fi
    
    # Build ArduinoJson version for comparison
    output_o_aj="parse_config_arduinojson_${config_name}.o"
    output_elf_aj="parse_config_arduinojson_${config_name}.elf"
    
    echo -e "${GREEN}[2/2] Building ArduinoJson version...${NC}"
    
    # Compile
    echo "Compiling to object file..."
    arm-none-eabi-g++ \
        -std=c++23 \
        "${ARM_FLAGS[@]}" \
        "${EMBEDDED_FLAGS[@]}" \
        $opt_flags \
        "${INCLUDES[@]}" \
        -c "$SCRIPT_DIR/parse_config_arduinojson.cpp" \
        -o "$SCRIPT_DIR/$output_o_aj"
    
    if [ $? -ne 0 ]; then
        echo -e "${RED}✗ ArduinoJson compilation failed${NC}"
        exit 1
    fi
    
    # Link
    echo "Linking to ELF..."
    arm-none-eabi-g++ \
        "${ARM_FLAGS[@]}" \
        "${EMBEDDED_FLAGS[@]}" \
        $opt_flags \
        -specs=nano.specs -specs=nosys.specs \
        -Wl,--gc-sections \
        "$SCRIPT_DIR/$output_o_aj" \
        -o "$SCRIPT_DIR/$output_elf_aj" 2>&1 | grep -v -e "is not implemented and will always fail" -e "the message above does not take linker garbage collection into account" -e "thumb/v7e-m+dp/hard/libc_nano.a" || true
    
    if [ $? -eq 0 ]; then
        echo -e "${GREEN}✓ Built: $output_elf_aj${NC}"
        
        # Analyze
        echo ""
        echo "=== Size Analysis ==="
        arm-none-eabi-size "$SCRIPT_DIR/$output_elf_aj"
        
        echo ""
        echo "=== Detailed Size (by section) ==="
        arm-none-eabi-size -A "$SCRIPT_DIR/$output_elf_aj" | grep -E "section|\.text|\.rodata|\.data|\.bss|Total"
        
        echo ""
        echo "=== Top 10 Symbols by Size ==="
        arm-none-eabi-nm --size-sort --reverse-sort --radix=d "$SCRIPT_DIR/$output_elf_aj" | \
            grep -v " [bB] " | \
            head -10 | \
            arm-none-eabi-c++filt
        
        # Store size for comparison
        aj_text=$(arm-none-eabi-size "$SCRIPT_DIR/$output_elf_aj" | tail -1 | awk '{print $1}')
        
        # Comparison
        echo ""
        echo -e "${YELLOW}=== Comparison (${config_name}) ===${NC}"
        echo "JsonFusion .text:   $jf_text bytes"
        echo "ArduinoJson .text:  $aj_text bytes"
        
        if [ "$jf_text" -lt "$aj_text" ]; then
            diff=$((aj_text - jf_text))
            pct=$((100 * diff / aj_text))
            echo -e "${GREEN}→ JsonFusion is ${diff} bytes (${pct}%) smaller${NC}"
        else
            diff=$((jf_text - aj_text))
            pct=$((100 * diff / jf_text))
            echo -e "${RED}→ ArduinoJson is ${diff} bytes (${pct}%) smaller${NC}"
        fi
        
        echo ""
        echo "---"
        echo ""
    else
        echo -e "${RED}✗ ArduinoJson linking failed${NC}"
        exit 1
    fi
done

echo -e "${GREEN}=== Summary ===${NC}"
echo ""
echo "Built ELF files:"
ls -lh "$SCRIPT_DIR"/*.elf 2>/dev/null | awk '{print $9, "-", $5}' || echo "No ELF files found"

echo ""
echo -e "${GREEN}Done!${NC}"

