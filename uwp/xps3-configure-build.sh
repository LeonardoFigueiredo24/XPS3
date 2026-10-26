#!/bin/sh
# Configure and build the XPS3 UWP executables from already prepared, pinned dependencies.
# This script performs no downloads and does not package or sign anything.
set -eu

if [ "$#" -ne 5 ]; then
    echo "usage: $0 <build-dir> <ffmpeg-lib-dir> <llvm-cmake-dir> <vulkan-include-dir> <evidence-log>" >&2
    exit 2
fi

build=$1
ffmpeg_lib=$(realpath "$2")
llvm_cmake=$(realpath "$3")
vulkan_include=$(realpath "$4")
evidence=$(realpath -m "$5")
root=$(realpath "$(dirname "$0")/..")
jobs=${XPS3_JOBS:-$(nproc)}
case "$jobs" in *[!0-9]*|'') echo "XPS3_JOBS must be a positive integer" >&2; exit 2;; esac
[ "$jobs" -gt 0 ] || { echo "XPS3_JOBS must be greater than zero" >&2; exit 2; }
export XPS3_TOOLCHAIN_CACHE=${XPS3_TOOLCHAIN_CACHE:-$build/xps3-toolchain-cache}

for command in cmake ninja python3; do
    command -v "$command" >/dev/null 2>&1 || { echo "missing command: $command" >&2; exit 3; }
done
for path in "$ffmpeg_lib" "$llvm_cmake" "$vulkan_include"; do
    [ -d "$path" ] || { echo "missing dependency directory: $path" >&2; exit 4; }
done

mkdir -p "$build" "$(dirname "$evidence")"
submodules=$(git -C "$root" submodule status --recursive)
if printf '%s\n' "$submodules" | grep -Eq '^[-+U]'; then
    echo "submodule checkout is incomplete or differs from the recorded gitlink" >&2
    printf '%s\n' "$submodules" >&2
    exit 5
fi
{
    echo "xps3_build_started=$(date -u +%Y-%m-%dT%H:%M:%SZ)"
    echo "source_commit=$(git -C "$root" rev-parse HEAD)"
    echo "source_tree=$(git -C "$root" rev-parse HEAD^{tree})"
    echo "submodule_status_begin"
    printf '%s\n' "$submodules"
    echo "submodule_status_end"
} >"$evidence"

configure_log="$build/xps3-cmake-configure.log"
if ! cmake -S "$root" -B "$build" -G Ninja \
    -DCMAKE_TOOLCHAIN_FILE="$root/uwp/toolchain/toolchain-uwp.cmake" \
    -DCMAKE_BUILD_TYPE=Release \
    -DRPCS3_UWP=ON \
    -DRPCS3_UWP_FFMPEG_LIB_DIR="$ffmpeg_lib" \
    -DUSE_VULKAN=ON \
    -DRPCS3_UWP_VULKAN_INCLUDE_DIR="$vulkan_include" \
    -DWITH_LLVM=ON \
    -DBUILD_LLVM=OFF \
    -DSTATIC_LINK_LLVM=ON \
    -DLLVM_DIR="$llvm_cmake" \
    -DUSE_NATIVE_INSTRUCTIONS=OFF \
    -DUSE_PRECOMPILED_HEADERS=OFF \
    -DUSE_SDL=OFF \
    -DUSE_FAUDIO=OFF \
    -DUSE_SYSTEM_ZLIB=OFF \
    -DUSE_SYSTEM_CURL=OFF \
    -DUSE_SYSTEM_SDL=OFF \
    -DUSE_SYSTEM_OPENCV=OFF \
    -DUSE_DISCORD_RPC=OFF \
    -DUSE_GAMEMODE=OFF \
    -DUSE_LIBEVDEV=OFF >"$configure_log" 2>&1; then
    tee -a "$evidence" <"$configure_log"
    exit 6
fi
tee -a "$evidence" <"$configure_log"

# The source checkout is shared with Windows and can contain CRLF line endings. libpng executes
# generated awk/DFA inputs inside the Linux builder, where a trailing carriage return corrupts
# tokens (for example "com\r"). Normalize only the generated build-tree copies so the verified
# source checkout remains untouched.
png_scripts="$build/3rdparty/libpng/libpng/scripts"
if [ -d "$png_scripts" ]; then
    find "$png_scripts" -type f \( -name '*.awk' -o -name '*.dfa' -o -name '*.dfn' -o -name '*.cmake' \) \
        -exec sed -i 's/\r$//' {} +
fi

build_log="$build/xps3-ninja-build.log"
if ! ninja -j "$jobs" -C "$build" xps3_uwp xps3_uwp_importcheck >"$build_log" 2>&1; then
    tee -a "$evidence" <"$build_log"
    exit 7
fi
tee -a "$evidence" <"$build_log"
echo "xps3_build_finished=$(date -u +%Y-%m-%dT%H:%M:%SZ)" >>"$evidence"
