#!/bin/sh
# Build the locked Mesa Dozen source without modifying the verified source checkout.
set -eu

root=${XPS3_SOURCE_ROOT:-/src}
state=${XPS3_BUILD_ROOT:-/build}
locked=$state/sources/mesa
source_dir=$state/mesa-source
build_dir=$state/mesa-build
expected=ddaf85e353442d9d7f192fdcf1823e652e3f2ee3

actual=$(git -C "$locked" rev-parse HEAD^{commit})
[ "$actual" = "$expected" ] || { echo "Mesa revision mismatch: expected $expected, got $actual" >&2; exit 1; }
[ -z "$(git -C "$locked" status --porcelain)" ] || { echo "locked Mesa checkout is dirty" >&2; exit 1; }

marker=$source_dir/.xps3-complete-$expected
if [ ! -f "$marker" ]; then
    rm -rf "$source_dir" "$build_dir"
    mkdir -p "$source_dir"
    git -C "$locked" archive HEAD | tar -x -C "$source_dir"
    printf '%s\n' "$expected" >"$marker"
fi

export XPS3_JOBS=${XPS3_JOBS:-1}
exec "$root/uwp/toolchain/build-mesa-dzn.sh" "$source_dir" "$build_dir"
