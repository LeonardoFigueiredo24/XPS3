#!/bin/sh
# Build the locked Linux makemsix host tool used to assemble the APPX.
set -eu

state=${XPS3_BUILD_ROOT:-/build}
source_dir=$state/sources/msix-packaging
build_dir=$state/msix-build
expected=25a65f5c1690930813bcc10cdf1d59fa865f2bb1

actual=$(git -C "$source_dir" rev-parse HEAD^{commit})
[ "$actual" = "$expected" ] || { echo "MSIX SDK revision mismatch: expected $expected, got $actual" >&2; exit 1; }
[ -z "$(git -C "$source_dir" status --porcelain)" ] || { echo "MSIX SDK checkout is dirty" >&2; exit 1; }

cmake -S "$source_dir" -B "$build_dir" -G Ninja \
    -DCMAKE_BUILD_TYPE=Release \
    -DMSIX_PACK=ON \
    -DUSE_VALIDATION_PARSER=ON \
    -DMSIX_SAMPLES=OFF \
    -DMSIX_TESTS=OFF
cmake --build "$build_dir" --target makemsix --parallel "${XPS3_JOBS:-1}"

tool=$(find "$build_dir" -type f -name makemsix -perm -111 | head -n 1)
[ -n "$tool" ] || { echo "makemsix executable was not produced" >&2; exit 1; }
printf '%s\n' "$tool" >"$build_dir/makemsix.path"
echo "XPS3_MAKEMSIX=$tool"
