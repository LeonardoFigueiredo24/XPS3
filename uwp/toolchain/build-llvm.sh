#!/bin/sh
# Cross-build RPCS3's LLVM submodule (X86 target only, static libraries) for the UWP port.
# Usage: build-llvm.sh <build dir> <install prefix> [native build dir]
# Then configure RPCS3 with -DWITH_LLVM=ON -DBUILD_LLVM=OFF -DSTATIC_LINK_LLVM=ON
#   -DLLVM_DIR=<install prefix>/lib/cmake/llvm
# LLVM's table generators have to run on the build host, so a native llvm-tblgen and
# llvm-min-tblgen are built first from the same sources (with the host's clang).
set -e
build=$(realpath -m "$1"); prefix=$(realpath -m "$2")
native=$(realpath -m "${3:-$build-native}")
tc=$(dirname "$(realpath "$0")")
src=$(realpath "${XPS3_LLVM_SOURCE:-$tc/../../3rdparty/llvm/llvm/llvm}")
jobs=${XPS3_JOBS:-$(nproc)}
case "$jobs" in *[!0-9]*|'') echo "XPS3_JOBS must be a positive integer" >&2; exit 2;; esac
[ "$jobs" -gt 0 ] || { echo "XPS3_JOBS must be greater than zero" >&2; exit 2; }

if [ ! -x "$native/bin/llvm-tblgen" ] || [ ! -x "$native/bin/llvm-min-tblgen" ]; then
	cmake -S "$src" -B "$native" -G Ninja -DCMAKE_BUILD_TYPE=Release \
		-DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ \
		-DLLVM_TARGETS_TO_BUILD=X86 -DLLVM_INCLUDE_TESTS=OFF -DLLVM_INCLUDE_BENCHMARKS=OFF -DLLVM_INCLUDE_EXAMPLES=OFF
	ninja -j "$jobs" -C "$native" llvm-tblgen llvm-min-tblgen
fi

cmake -S "$src" -B "$build" -G Ninja \
	-DCMAKE_TOOLCHAIN_FILE="$tc/toolchain-uwp.cmake" -DCMAKE_BUILD_TYPE=Release \
	-DCMAKE_INSTALL_PREFIX="$prefix" \
	-DLLVM_HOST_TRIPLE=x86_64-w64-windows-gnu -DLLVM_TARGETS_TO_BUILD=X86 \
	-DLLVM_TABLEGEN="$native/bin/llvm-tblgen" -DLLVM_NATIVE_TOOL_DIR="$native/bin" \
	-DLLVM_BUILD_TOOLS=OFF -DLLVM_INCLUDE_TOOLS=OFF -DLLVM_BUILD_UTILS=OFF -DLLVM_INCLUDE_UTILS=OFF \
	-DLLVM_INCLUDE_TESTS=OFF -DLLVM_INCLUDE_BENCHMARKS=OFF -DLLVM_INCLUDE_EXAMPLES=OFF -DLLVM_INCLUDE_DOCS=OFF \
	-DLLVM_BUILD_RUNTIME=OFF -DLLVM_ENABLE_WARNINGS=OFF -DLLVM_ENABLE_ASSERTIONS=OFF \
	-DLLVM_ENABLE_ZLIB=OFF -DLLVM_ENABLE_ZSTD=OFF -DLLVM_ENABLE_LIBXML2=OFF -DLLVM_ENABLE_LIBEDIT=OFF \
	-DLLVM_ENABLE_LIBPFM=OFF -DLLVM_ENABLE_DIA_SDK=OFF -DLLVM_ENABLE_BINDINGS=OFF \
	-DLLVM_USE_INTEL_JITEVENTS=OFF -DLLVM_ENABLE_PER_TARGET_RUNTIME_DIR=OFF
ninja -j "$jobs" -C "$build" install
