#!/bin/sh
# Build import libraries Zig can't resolve by itself: mixed-case library names used by
# RPCS3's dependencies (e.g. -lShlwapi) and runtimeobject (WinRT activation APIs).
set -e
zig_dir=$1; out=$2
defs=$zig_dir/lib/libc/mingw/lib-common
dlltool=$(command -v llvm-dlltool || command -v llvm-dlltool-18 || command -v llvm-dlltool-20)
mkdir -p "$out"
for pair in Shlwapi:shlwapi Iphlpapi:iphlpapi Winmm:winmm Psapi:psapi; do
	"$dlltool" -m i386:x86-64 -d "$defs/${pair#*:}.def" -l "$out/lib${pair%%:*}.a"
done
"$dlltool" -m i386:x86-64 -d "$defs/api-ms-win-core-winrt-l1-1-0.def" -l "$out/libwinrt_activation.a"
"$dlltool" -m i386:x86-64 -d "$defs/api-ms-win-core-winrt-string-l1-1-0.def" -l "$out/libwinrt_string.a"
# UWP memory/file APIs (VirtualAllocFromApp, MapViewOfFile3FromApp, CreateFile2FromAppW, ...)
for v in 1 3 5 6; do
	"$dlltool" -m i386:x86-64 -d "$defs/api-ms-win-core-memory-l1-1-$v.def" -l "$out/libuwp_memory_$v.a"
done
"$dlltool" -m i386:x86-64 -d "$defs/api-ms-win-core-file-fromapp-l1-1-0.def" -l "$out/libuwp_file_fromapp.a"
