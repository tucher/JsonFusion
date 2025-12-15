#!/usr/bin/env python3
"""
Build script for ARM Cortex-M7 embedded binary size benchmark
Measures code footprint of different JSON libraries on a realistic embedded target
"""

import argparse
import subprocess
import sys
from dataclasses import dataclass
from pathlib import Path
from typing import List, Dict, Optional
import urllib.request


# ============================================================================
# Configuration
# ============================================================================

AVR_ATMEGA_2560_TEST = False

# ARM Cortex-M7 target configuration
compiler_prefix="arm-none-eabi-"
CPU_FLAGS = [
    "-mcpu=cortex-m7",
    "-mthumb",
    "-mfloat-abi=hard",
    "-mfpu=fpv5-d16",
]

if AVR_ATMEGA_2560_TEST:
    compiler_prefix="avr-"
    CPU_FLAGS = [
        f"-I/libstdcpp",
        "-mmcu=atmega2560",
        "-g0",
        "-Wall",
        "-fno-threadsafe-statics"
    ]

@dataclass
class Library:
    """Configuration for a library to benchmark"""
    name: str
    source_file: str
    description: str
    dependencies: List[str] = None  # URLs or file paths to download
    
    def __post_init__(self):
        if self.dependencies is None:
            self.dependencies = []


@dataclass
class BuildConfig:
    """Build configuration (optimization level, flags)"""
    name: str
    opt_flags: List[str]




# Embedded-friendly flags
EMBEDDED_FLAGS = [
    "-fno-exceptions",
    "-fno-rtti",
    "-ffunction-sections",
    "-fdata-sections",
    "-DNDEBUG",                          # Disable assertions
    "-fno-unwind-tables",                # Remove exception unwinding tables
    "-fno-asynchronous-unwind-tables",   # Remove async unwind info
]

# Build configurations
BUILD_CONFIGS = [
    BuildConfig("aggressive_opt", ["-O3", "-flto"]),
    BuildConfig("minimal_size", ["-Os", "-flto"]),
]

# Libraries to benchmark
LIBRARIES = [
    Library(
        name="JsonFusion",
        source_file="parse_config.cpp",
        description="JsonFusion with in-house float parser",
    ),
    Library(
        name="ArduinoJson",
        source_file="parse_config_arduinojson.cpp",
        description="ArduinoJson v7.2.1",
        dependencies=["https://github.com/bblanchon/ArduinoJson/releases/download/v7.2.1/ArduinoJson.h"],
    ),
    Library(
        name="jsmn",
        source_file="parse_config_jsmn.cpp",
        description="jsmn - minimalist JSON tokenizer",
        dependencies=["https://raw.githubusercontent.com/zserge/jsmn/master/jsmn.h"],
    ),
    Library(
        name="cJSON",
        source_file="parse_config_cjson.cpp",
        description="cJSON - lightweight JSON parser in C",
        dependencies=[
            "https://raw.githubusercontent.com/DaveGamble/cJSON/master/cJSON.h",
            "https://raw.githubusercontent.com/DaveGamble/cJSON/master/cJSON.c",
        ],
    ),
]
if not AVR_ATMEGA_2560_TEST:
    LIBRARIES += [
        Library(
            name="Glaze",
            source_file="parse_config_glaze.cpp",
            description="Glaze - probably the fastest JSON parser in C++",
            dependencies=[
        
            ],
        )
    ]

# Linker warning patterns to filter out
LINKER_WARNING_FILTERS = [
    "is not implemented and will always fail",
    "the message above does not take linker garbage collection into account",
    "thumb/v7e-m+dp/hard/libc_nano.a",
]


# ============================================================================
# Colors
# ============================================================================

class Colors:
    RED = '\033[0;31m'
    GREEN = '\033[0;32m'
    YELLOW = '\033[1;33m'
    BLUE = '\033[0;34m'
    NC = '\033[0m'  # No Color
    
    @staticmethod
    def red(text): return f"{Colors.RED}{text}{Colors.NC}"
    
    @staticmethod
    def green(text): return f"{Colors.GREEN}{text}{Colors.NC}"
    
    @staticmethod
    def yellow(text): return f"{Colors.YELLOW}{text}{Colors.NC}"
    
    @staticmethod
    def blue(text): return f"{Colors.BLUE}{text}{Colors.NC}"


# ============================================================================
# Build System
# ============================================================================

class EmbeddedBenchmark:
    def __init__(self, script_dir: Path):
        self.script_dir = script_dir
        self.project_root = script_dir.parent.parent.parent
        self.includes = [
            f"-I{self.project_root}/include",
            f"-I{self.script_dir}",
            f"-I{self.script_dir}/libs",  # For downloaded headers
        ]
        self.results: Dict[str, Dict[str, int]] = {}  # config -> {lib: text_size}
    
    def clean(self):
        """Remove all build artifacts"""
        print(Colors.yellow("=== Cleaning Build Artifacts ==="))
        
        patterns = ["*.o", "*.elf", "*.map"]
        removed_count = 0
        
        for pattern in patterns:
            for file in self.script_dir.glob(pattern):
                # print(f"Removing {file.name}")
                file.unlink()
                removed_count += 1
        
        if removed_count == 0:
            print("No build artifacts to remove")
        else:
            print(Colors.green(f"✓ Removed {removed_count} file(s)"))
        
        print()
    
    def download_dependencies(self):
        """Download required library headers"""
        print(Colors.green("=== Checking Dependencies ==="))
        print()
        
        # Ensure libs directory exists
        libs_dir = self.script_dir / "libs"
        libs_dir.mkdir(exist_ok=True)
        
        for lib in LIBRARIES:
            for dep in lib.dependencies:
                if dep.startswith("http"):
                    # Extract filename from URL
                    filename = Path(dep).name
                    dest_path = libs_dir / filename
                    
                    if dest_path.exists():
                        print(f"✓ {filename} already exists")
                    else:
                        print(f"Downloading {filename}...")
                        try:
                            urllib.request.urlretrieve(dep, dest_path)
                            print(Colors.green(f"✓ Downloaded {filename}"))
                        except Exception as e:
                            print(Colors.red(f"✗ Failed to download {filename}: {e}"))
                            sys.exit(1)
        print()
    
    def compile(self, lib: Library, config: BuildConfig) -> Path:
        """Compile source to object file"""
        output_o = self.script_dir / f"parse_config_{lib.name.lower()}_{config.name}.o"
        source = self.script_dir / lib.source_file
        
        cmd = [
            f"{compiler_prefix}g++",
            "-std=c++23",
            *CPU_FLAGS,
            *EMBEDDED_FLAGS,
            *config.opt_flags,
            *self.includes,
            "-c", str(source),
            "-o", str(output_o),
        ]
        
        print("Compiling to object file...")
        result = subprocess.run(cmd, capture_output=True, text=True)
        
        if result.returncode != 0:
            print(Colors.red(f"✗ Compilation failed"))
            print(result.stderr)
            sys.exit(1)
        print(result.stderr)
        print(result.stdout)
        return output_o
    
    def link(self, lib: Library, config: BuildConfig, obj_file: Path) -> Path:
        """Link object file to ELF executable"""
        output_elf = self.script_dir / f"parse_config_{lib.name.lower()}_{config.name}.elf"
        map_file = self.script_dir / f"parse_config_{lib.name.lower()}_{config.name}.map"
        
        cmd = [
            f"{compiler_prefix}g++",
            *CPU_FLAGS,
            *EMBEDDED_FLAGS,
            *config.opt_flags,
            # "-specs=nano.specs", "-specs=nosys.specs",
            "-Wl,--gc-sections",
            f"-Wl,-Map={map_file}",
            str(obj_file),
            "-o", str(output_elf),
        ]
        if not AVR_ATMEGA_2560_TEST:
            cmd += ["-specs=nano.specs", "-specs=nosys.specs"]
        
        print("Linking to ELF...")
        result = subprocess.run(cmd, capture_output=True, text=True)
        
        # Filter out expected warnings
        stderr_lines = result.stderr.split('\n')
        filtered_stderr = [
            line for line in stderr_lines
            if not any(pattern in line for pattern in LINKER_WARNING_FILTERS)
        ]
        if filtered_stderr and any(filtered_stderr):
            print('\n'.join(filtered_stderr))
        
        if result.returncode != 0:
            print(Colors.red(f"✗ Linking failed"))
            sys.exit(1)
        
        print(Colors.green(f"✓ Built: {output_elf.name}"))
        return output_elf
    
    def analyze_size(self, elf_file: Path) -> int:
        """Analyze ELF file and return .text section size"""
        print()
        print("=== Size Analysis ===")
        
        # Overall size
        result = subprocess.run(
            [f"{compiler_prefix}size", str(elf_file)],
            capture_output=True, text=True
        )
        print(result.stdout)
        
        # Detailed by section
        print("=== Detailed Size (by section) ===")
        result = subprocess.run(
            [f"{compiler_prefix}size", "-A", str(elf_file)],
            capture_output=True, text=True
        )
        for line in result.stdout.split('\n'):
            if any(x in line for x in ["section", ".text", ".rodata", ".data", ".bss", "Total"]):
                print(line)
        
        # Top symbols
        print()
        # print("=== Top 10 Symbols by Size ===")
        nm_result = subprocess.run(
            [f"{compiler_prefix}gcc-nm", "--size-sort", "--reverse-sort", "--radix=d", str(elf_file)],
            capture_output=True, text=True
        )
        # Filter out BSS symbols and take top 10
        lines = [line for line in nm_result.stdout.split('\n') if ' B ' not in line and ' b ' not in line]
        top_symbols = '\n'.join(lines[:10])
        
        # Demangle
        demangle_result = subprocess.run(
            [f"{compiler_prefix}c++filt"],
            input=top_symbols, capture_output=True, text=True
        )
        # print(demangle_result.stdout)
        
        # Extract .text size
        result = subprocess.run(
            [f"{compiler_prefix}size", str(elf_file)],
            capture_output=True, text=True
        )
        lines = result.stdout.strip().split('\n')
        if len(lines) >= 2:
            # Second line has the actual numbers
            text_size = int(lines[1].split()[0])
            return text_size
        
        return 0
    
    def build_library(self, lib: Library, config: BuildConfig):
        """Build a single library with given config"""
        print(Colors.green(f"[{LIBRARIES.index(lib)+1}/{len(LIBRARIES)}] Building {lib.name}..."))
        print(f"Description: {lib.description}")
        print()
        
        # Compile
        obj_file = self.compile(lib, config)
        
        # Link
        elf_file = self.link(lib, config, obj_file)
        
        # Analyze
        text_size = self.analyze_size(elf_file)
        
        # Store result
        if config.name not in self.results:
            self.results[config.name] = {}
        self.results[config.name][lib.name] = text_size
        
        print()
        print()
    
    def compare_results(self, config_name: str):
        """Compare all libraries for a given config"""
        if config_name not in self.results:
            return
        
        sizes = self.results[config_name]
        if len(sizes) < 2:
            return
        
        print(Colors.yellow(f"=== Comparison ({config_name}) ==="))
        
        # Print all sizes
        for lib_name, size in sizes.items():
            print(f"{lib_name:20} .text: {size:7} bytes")
        
        # Compare JsonFusion against each other library
        if "JsonFusion" in sizes:
            jf_size = sizes["JsonFusion"]
            print()
            for lib_name, lib_size in sizes.items():
                if lib_name == "JsonFusion":
                    continue
                
                diff = jf_size - lib_size
                pct = abs(diff) * 100 // lib_size
                
                if diff < 0:
                    print(Colors.green(f"→ JsonFusion is {-diff} bytes ({pct}%) smaller than {lib_name}"))
                elif diff > 0:
                    print(Colors.red(f"→ JsonFusion is {diff} bytes ({pct}%) larger than {lib_name}"))
                else:
                    print(f"→ JsonFusion and {lib_name} are the same size")
        
        print()
        print("---")
        print()
    
    def print_summary(self):
        """Print final summary"""
        print(Colors.green("=== Summary ==="))
        print()
        
        # List all built files
        print("Built ELF files:")
        elf_files = sorted(self.script_dir.glob("*.elf"))
        for elf in elf_files:
            size_kb = elf.stat().st_size / 1024
            print(f"  {elf.name:<50} {size_kb:6.1f} KB")
        
        print()
        
        # Results table
        if self.results:
            print("Code Size Summary (.text section):")
            print()
            
            # Header
            config_names = list(self.results.keys())
            lib_names = list(self.results[config_names[0]].keys())
            
            print(f"{'Library':<20}", end="")
            for config_name in config_names:
                print(f" {config_name:<20}", end="")
            print()
            print("-" * (20 + 20 * len(config_names)))
            
            # Data
            for lib_name in lib_names:
                print(f"{lib_name:<20}", end="")
                for config_name in config_names:
                    size = self.results[config_name].get(lib_name, 0)
                    size_kb = size / 1024
                    print(f" {size_kb:6.1f} KB ({size:5} B)  ", end="")
                print()
        
        print()
        print(Colors.green("Done!"))
    
    def run(self, clean: bool = True):
        """Main build process"""
        print(Colors.green("=== Embedded Binary Size Benchmark ==="))
        print("Target: ARM Cortex-M7 (Thumb-2, Hardware FP)")
        print(f"Comparing: {', '.join(lib.name for lib in LIBRARIES)}")
        print()
        
        # Clean old artifacts if requested
        if clean:
            self.clean()
        
        # Download dependencies
        self.download_dependencies()
        
        # Build all combinations
        for config in BUILD_CONFIGS:
            print(Colors.yellow("=" * 60))
            print(Colors.yellow(f"Configuration: {config.name}"))
            print(Colors.yellow(f"Flags: {' '.join(config.opt_flags)}"))
            print(Colors.yellow("=" * 60))
            print()
            
            for lib in LIBRARIES:
                self.build_library(lib, config)
            
            # Compare results for this config
            self.compare_results(config.name)
        
        # Print summary
        self.print_summary()


# ============================================================================
# Main
# ============================================================================

def main():
    parser = argparse.ArgumentParser(
        description="Build and benchmark JSON libraries for ARM Cortex-M7",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  %(prog)s                    # Build all (clean first)
  %(prog)s --no-clean         # Build all (keep old artifacts)
  %(prog)s --clean-only       # Just clean, don't build
        """
    )
    parser.add_argument(
        "--no-clean",
        action="store_false",
        help="Don't remove old build artifacts before building (default: clean first)"
    )
    parser.add_argument(
        "--clean-only",
        action="store_true",
        help="Only clean build artifacts, don't build"
    )
    
    args = parser.parse_args()
    
    script_dir = Path(__file__).parent.resolve()
    benchmark = EmbeddedBenchmark(script_dir)
    
    try:
        # Clean-only mode
        if args.clean_only:
            benchmark.clean()
            return
        
        # Normal build (with or without cleaning)
        benchmark.run(clean=not args.no_clean)
        # benchmark.clean()
    except KeyboardInterrupt:
        print()
        print(Colors.yellow("Interrupted by user"))
        sys.exit(1)
    except Exception as e:
        print(Colors.red(f"Error: {e}"))
        import traceback
        traceback.print_exc()
        sys.exit(1)


if __name__ == "__main__":
    main()

