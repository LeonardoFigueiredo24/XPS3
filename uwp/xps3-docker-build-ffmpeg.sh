#!/bin/sh
# Build the locked FFmpeg source in the persistent XPS3 Docker build volume.
set -eu

root=${XPS3_SOURCE_ROOT:-/src}
source_dir=${XPS3_FFMPEG_SOURCE:-/build/sources/ffmpeg}
build_dir=${XPS3_FFMPEG_BUILD:-/build/ffmpeg-build}
install_dir=${XPS3_FFMPEG_INSTALL:-/build/ffmpeg-install}
expected=9047fa1b084f76b1b4d065af2d743df1b40dfb56

if [ ! -x "$source_dir/configure" ]; then
    echo "locked FFmpeg source is missing; run xps3-fetch-sources.sh first" >&2
    exit 1
fi

actual=$(git -C "$source_dir" rev-parse HEAD^{commit})
if [ "$actual" != "$expected" ]; then
    echo "FFmpeg revision mismatch: expected $expected, got $actual" >&2
    exit 1
fi
if [ -n "$(git -C "$source_dir" status --porcelain)" ]; then
    echo "FFmpeg checkout has local changes; refusing to build an unrecorded source" >&2
    exit 1
fi

echo "XPS3_FFMPEG_SOURCE=$actual"
exec "$root/uwp/toolchain/build-ffmpeg.sh" "$source_dir" "$build_dir" "$install_dir"
