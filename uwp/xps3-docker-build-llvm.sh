#!/bin/sh
# Run inside the XPS3 builder container. The Windows checkout can contain CRLF conversion, so the
# pinned LLVM tree is materialized from Git objects into the Linux build volume before compiling.
set -eu

root=$(realpath "$(dirname "$0")/..")
state=${XPS3_BUILD_ROOT:-/build}
llvm_source=$state/llvm-source
git config --global --add safe.directory "$root/3rdparty/llvm/llvm"
llvm_commit=$(git -C "$root/3rdparty/llvm/llvm" rev-parse HEAD)
llvm_marker=$llvm_source/.xps3-complete-$llvm_commit

if [ ! -f "$llvm_marker" ]; then
    rm -rf "$llvm_source"
    mkdir -p "$llvm_source"
    git -C "$root/3rdparty/llvm/llvm" archive HEAD llvm cmake third-party | tar -x -C "$llvm_source"
    printf '%s\n' "$llvm_commit" >"$llvm_marker"
fi

export XPS3_LLVM_SOURCE=$llvm_source/llvm
export XPS3_JOBS=${XPS3_JOBS:-1}
exec "$root/uwp/toolchain/build-llvm.sh" \
    "$state/llvm-cross-lfs" \
    "$state/llvm-install" \
    "$state/llvm-native-lfs"
