#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")"

SCRIPT_DIR=$PWD
LLVM_TAG=llvmorg-21.1.8

# ── Clone LLVM (skip if already present) ───────────────────────────
if [ ! -d llvm-project/.git ]; then
  git clone https://github.com/llvm/llvm-project.git
fi

cd llvm-project

# Checkout desired tag (only if not already on it)
if [ "$(git describe --tags --exact-match 2>/dev/null)" != "$LLVM_TAG" ]; then
  git checkout "$LLVM_TAG"
fi

# ── Inject tool sources (always refresh) ──────────────────────────
mkdir -p clang/tools/clang-constexpr-run
cp "$SCRIPT_DIR/clang-constexpr-run.cpp" clang/tools/clang-constexpr-run/
cp "$SCRIPT_DIR/CMakeLists.txt"          clang/tools/clang-constexpr-run/

grep -q 'clang-constexpr-run' clang/tools/CMakeLists.txt || \
  printf '\nadd_clang_subdirectory(clang-constexpr-run)\n' >> clang/tools/CMakeLists.txt

# ── Host build (tablegen tools, skip if already built) ─────────────
if [ ! -x build-host/bin/llvm-tblgen ] || [ ! -x build-host/bin/clang-tblgen ]; then
  cmake -S llvm -B build-host -G Ninja \
    -DCMAKE_BUILD_TYPE=Release \
    -DLLVM_ENABLE_PROJECTS="clang" \
    -DLLVM_INCLUDE_TESTS=OFF -DLLVM_INCLUDE_EXAMPLES=OFF -DLLVM_INCLUDE_BENCHMARKS=OFF \
    -DLLVM_ENABLE_TERMINFO=OFF -DLLVM_ENABLE_ZLIB=OFF -DLLVM_ENABLE_ZSTD=OFF

  ninja -C build-host llvm-tblgen clang-tblgen llvm-config
else
  echo ">>> Host tablegen tools already built, skipping."
fi

# ── Generate libc++ configured headers (proper CMake way) ─────────
# This generates __config_site, __assertion_handler, and all other
# CMake-produced files instead of crafting them by hand.
LIBCXX_HEADER_BUILD=build-libcxx-headers
if [ ! -f "$LIBCXX_HEADER_BUILD/include/c++/v1/__config_site" ]; then
  cmake -S runtimes -B "$LIBCXX_HEADER_BUILD" -G Ninja \
    -DLLVM_ENABLE_RUNTIMES="libcxx" \
    -DCMAKE_CXX_COMPILER="$(command -v clang++ || command -v g++)" \
    -DCMAKE_C_COMPILER="$(command -v clang || command -v gcc)" \
    -DLIBCXX_CXX_ABI=none \
    -DLIBCXX_ENABLE_SHARED=OFF \
    -DLIBCXX_ENABLE_STATIC=OFF \
    -DLIBCXX_ENABLE_FILESYSTEM=OFF \
    -DLIBCXX_ENABLE_LOCALIZATION=OFF \
    -DLIBCXX_ENABLE_RANDOM_DEVICE=OFF \
    -DLIBCXX_ENABLE_THREADS=OFF \
    -DLIBCXX_ENABLE_MONOTONIC_CLOCK=OFF \
    -DLIBCXX_ENABLE_UNICODE=ON \
    -DLIBCXX_ENABLE_WIDE_CHARACTERS=OFF \
    -DLIBCXX_ENABLE_TIME_ZONE_DATABASE=OFF \
    -DLIBCXX_HAS_TERMINAL_AVAILABLE=OFF \
    -DLIBCXX_HAS_MUSL_LIBC=ON \
    -DLIBCXX_HARDENING_MODE=none

  ninja -C "$LIBCXX_HEADER_BUILD" generate-cxx-headers
else
  echo ">>> libc++ headers already generated, skipping."
fi

# Find where CMake put the generated headers
LIBCXX_GEN_DIR="$LIBCXX_HEADER_BUILD/include/c++/v1"
if [ ! -f "$LIBCXX_GEN_DIR/__config_site" ]; then
  # Fallback: search for it
  LIBCXX_GEN_DIR=$(dirname "$(find "$LIBCXX_HEADER_BUILD" -name '__config_site' -print -quit)")
  echo ">>> Found generated headers at: $LIBCXX_GEN_DIR"
fi

# ── Stage headers for embedding into WASM ──────────────────────────
STAGING=$PWD/staging
rm -rf "$STAGING"
mkdir -p "$STAGING/lib/clang/21/include"
mkdir -p "$STAGING/include/c++/v1"

# 1. Clang builtin headers (stddef.h, stdarg.h, intrinsics, etc.)
cp clang/lib/Headers/*.h "$STAGING/lib/clang/21/include/"
rm -f "$STAGING/lib/clang/21/include/__clang_cuda_"*
rm -f "$STAGING/lib/clang/21/include/__clang_hip_"*
rm -f "$STAGING/lib/clang/21/include/cuda_wrappers"*
rm -f "$STAGING/lib/clang/21/include/__gpu_"*
for d in clang/lib/Headers/*/; do
  dname=$(basename "$d")
  case "$dname" in cuda_wrappers|openmp_wrappers|llvm_libc_wrappers|hlsl|ppc_wrappers) continue ;; esac
  cp -r "$d" "$STAGING/lib/clang/21/include/$dname"
done

# 2. libc++ headers: source include/ + CMake-generated files merged
rsync -a --exclude='__cxx03' --exclude='CMakeLists.txt' \
  libcxx/include/ "$STAGING/include/c++/v1/"
# Overlay generated files (__config_site, __assertion_handler, etc.)
rsync -a "$LIBCXX_GEN_DIR/" "$STAGING/include/c++/v1/"

# 3. Musl C headers from Emscripten sysroot
EMSYSROOT="$(em-config CACHE)/sysroot/include"
cp "$EMSYSROOT"/*.h "$STAGING/include/" 2>/dev/null || true
for d in bits sys arpa netinet wasi; do
  [ -d "$EMSYSROOT/$d" ] && cp -r "$EMSYSROOT/$d" "$STAGING/include/"
done

# 4. JsonFusion + PFR headers
JFROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
cp -r "$JFROOT/include/JsonFusion" "$STAGING/include/"
cp -r "$JFROOT/include/pfr"       "$STAGING/include/"
[ -f "$JFROOT/include/rapidyaml.hpp" ] && cp "$JFROOT/include/rapidyaml.hpp" "$STAGING/include/"

echo "=== Staged headers ==="
du -sh "$STAGING"
find "$STAGING" -type f | wc -l
echo "======================"

# ── Emscripten cross-build ─────────────────────────────────────────
if [ ! -d build-ems ]; then
  emcmake cmake -S llvm -B build-ems -G Ninja \
    -DCMAKE_BUILD_TYPE=MinSizeRel \
    -DLLVM_ENABLE_PROJECTS="clang" \
    -DLLVM_TABLEGEN="$PWD/build-host/bin/llvm-tblgen" \
    -DCLANG_TABLEGEN="$PWD/build-host/bin/clang-tblgen" \
    -DLLVM_CONFIG_PATH="$PWD/build-host/bin/llvm-config" \
    -DLLVM_TARGETS_TO_BUILD="WebAssembly" \
    -DLLVM_INCLUDE_TESTS=OFF -DLLVM_INCLUDE_EXAMPLES=OFF -DLLVM_INCLUDE_BENCHMARKS=OFF \
    -DLLVM_ENABLE_THREADS=OFF \
    -DLLVM_ENABLE_TERMINFO=OFF -DLLVM_ENABLE_ZLIB=OFF -DLLVM_ENABLE_ZSTD=OFF \
    -DBUILD_SHARED_LIBS=OFF \
    -DCLANG_ENABLE_STATIC_ANALYZER=OFF \
    -DCLANG_ENABLE_ARCMT=OFF \
    -DCMAKE_EXE_LINKER_FLAGS="\
      -sALLOW_MEMORY_GROWTH=1 \
      -sSTACK_SIZE=50MB \
      -sMODULARIZE=1 \
      -sEXPORT_ES6=1 \
      -sENVIRONMENT=web,worker,node \
      -sFILESYSTEM=1 \
      -sEXPORTED_RUNTIME_METHODS=FS,callMain \
      -sEXPORTED_FUNCTIONS=_main \
      --embed-file $STAGING@/sysroot"
else
  echo ">>> build-ems already configured, rebuilding. To reconfigure: rm -rf llvm-project/build-ems"
fi

ninja -C build-ems clang-constexpr-run

ls -1 build-ems/bin | grep clang-constexpr-run
cp build-ems/bin/clang-constexpr-run.js build-ems/bin/clang-constexpr-run.mjs
node "$SCRIPT_DIR/js_example.mjs" file://$PWD/build-ems/bin/clang-constexpr-run.mjs
