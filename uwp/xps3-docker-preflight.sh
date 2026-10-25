#!/bin/sh
# Run inside the XPS3 UWP builder image with this repository mounted at /src.
set -eu

root=$(realpath "$(dirname "$0")/..")
build=${1:-/tmp/xps3-toolchain-preflight}
export XPS3_TOOLCHAIN_CACHE=${XPS3_TOOLCHAIN_CACHE:-$build/xps3-toolchain-cache}

for command in cmake ninja meson python3 nasm clang llvm-nm; do
    command -v "$command" >/dev/null 2>&1 || {
        echo "missing command: $command" >&2
        exit 3
    }
done

windres=
for candidate in llvm-windres llvm-windres-18 llvm-windres-19 llvm-windres-20; do
    if command -v "$candidate" >/dev/null 2>&1; then
        windres=$candidate
        break
    fi
done
[ -n "$windres" ] || { echo "missing llvm-windres" >&2; exit 3; }

python3 -m ziglang version
cmake --version | sed -n '1p'
ninja --version
meson --version
nasm -v
"$windres" --version | sed -n '1p'

cmake -S "$root/uwp/test/toolchain-preflight" -B "$build" -G Ninja \
    -DCMAKE_TOOLCHAIN_FILE="$root/uwp/toolchain/toolchain-uwp.cmake" \
    -DCMAKE_BUILD_TYPE=Release
cmake --build "$build" --parallel 1

object=$(find "$build" -type f \( -name '*.obj' -o -name '*.o' \) | head -n 1)
[ -n "$object" ] || { echo "cross-compiled object not found" >&2; exit 4; }
file "$object"
echo "XPS3_UWP_TOOLCHAIN_PREFLIGHT=PASS"
