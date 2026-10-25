#!/bin/sh
# Cross-build the static FFmpeg libraries for the UWP port with Zig, using the same
# component set as upstream RPCS3 (3rdparty/ffmpeg/ffmpeg.patch).
# Usage: build-ffmpeg.sh <ffmpeg source dir (n8.1)> <build dir> <install prefix>
# Then configure RPCS3 with -DRPCS3_UWP_FFMPEG_LIB_DIR=<install prefix>/lib
set -e
src=$(realpath "$1"); build=$2; prefix=$(realpath -m "$3")
tc=$(dirname "$(realpath "$0")")
jobs=${XPS3_JOBS:-$(nproc)}
case "$jobs" in *[!0-9]*|'') echo "XPS3_JOBS must be a positive integer" >&2; exit 2;; esac
[ "$jobs" -gt 0 ] || { echo "XPS3_JOBS must be greater than zero" >&2; exit 2; }
mkdir -p "$build"
cd "$build"

"$src/configure" \
	--prefix="$prefix" \
	--enable-cross-compile --target-os=mingw32 --arch=x86_64 \
	--cc="$tc/zig-cc-ffmpeg" --cxx="$tc/zig-c++" --ar="$tc/zig-ar" --ranlib="$tc/zig-ranlib" --nm=llvm-nm \
	--pkg-config=false --x86asmexe=nasm \
	--enable-static --disable-shared --enable-pic --disable-doc --disable-programs \
	--enable-runtime-cpudetect --disable-autodetect --enable-w32threads \
	--disable-avdevice --disable-avfilter --disable-debug \
	--disable-everything --disable-network \
	--enable-decoder=aac --enable-decoder=aac_latm --enable-decoder=atrac3 --enable-decoder=atrac3p --enable-decoder=atrac9 --enable-decoder=mp3 --enable-decoder=pcm_s16le --enable-decoder=pcm_s8 \
	--enable-decoder=mov --enable-decoder=h264 --enable-decoder=mpeg4 --enable-decoder=mpeg2video --enable-decoder=mjpeg --enable-decoder=mjpegb \
	--enable-encoder=pcm_s16le --enable-encoder=mp3 --enable-encoder=ac3 --enable-encoder=aac \
	--enable-encoder=ffv1 --enable-encoder=mpeg4 --enable-encoder=mjpeg --enable-encoder=h264 \
	--enable-muxer=avi --enable-muxer=h264 --enable-muxer=mjpeg --enable-muxer=mp4 \
	--enable-demuxer=h264 --enable-demuxer=m4v --enable-demuxer=mp3 --enable-demuxer=mpegvideo --enable-demuxer=mpegps --enable-demuxer=mjpeg --enable-demuxer=mov \
	--enable-demuxer=avi --enable-demuxer=aac --enable-demuxer=pmp --enable-demuxer=oma --enable-demuxer=pcm_s16le --enable-demuxer=pcm_s8 --enable-demuxer=wav \
	--enable-parser=h264 --enable-parser=mpeg4video --enable-parser=mpegaudio --enable-parser=mpegvideo --enable-parser=mjpeg --enable-parser=aac --enable-parser=aac_latm \
	--enable-protocol=file \
	--enable-bsf=mjpeg2jpeg

make -j"$jobs"
make install
