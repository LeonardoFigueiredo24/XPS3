#!/bin/sh
# Cross-build Mesa's Dozen Vulkan driver (vulkan_dzn.dll) for the UWP port.
# Usage: build-mesa-dzn.sh <mesa source (tested: 26.2.5)> <build dir>
# Run after the RPCS3 CMake configure step, which creates uwp-implibs (see meson-cross-uwp.ini).
set -e
tc=$(dirname "$(realpath "$0")")
cross_file="$2.xps3-meson-cross-uwp.ini"
implibs=${XPS3_UWP_IMPLIB_DIR:-${XPS3_BUILD_ROOT:-/build}/app-build/xps3-toolchain-cache/uwp-implibs}
jobs=${XPS3_JOBS:-$(nproc)}
case "$jobs" in *[!0-9]*|'') echo "XPS3_JOBS must be a positive integer" >&2; exit 2;; esac
[ "$jobs" -gt 0 ] || { echo "XPS3_JOBS must be greater than zero" >&2; exit 2; }
[ -f "$implibs/mesa_sysstubs.o" ] || { echo "missing UWP import libraries: $implibs" >&2; exit 3; }
sed -e "s|@XPS3_TOOLCHAIN@|$tc|g" -e "s|@XPS3_IMPLIBS@|$implibs|g" \
	"$tc/meson-cross-uwp.ini" >"$cross_file"
# Port patch (also adds dzn_trace.c to meson.build): expose VK_EXT_shader_uniform_buffer_unsized_array (RPCS3 shader interpreter needs it).
git -C "$1" apply --check "$tc/mesa-dzn-uwp.patch" 2>/dev/null && git -C "$1" apply "$tc/mesa-dzn-uwp.patch"
# Sampled depth images (D16/D32 + SAMPLED) as typeless resources when D3D12 has no castable-format list (Xbox).
git -C "$1" apply --check "$tc/mesa-dzn-depth-typeless.patch" 2>/dev/null && git -C "$1" apply "$tc/mesa-dzn-depth-typeless.patch"
# Reuse command buffers' internal D3D12 buffers through a device pool (DZN_NO_BUFPOOL turns it off).
git -C "$1" apply --check "$tc/mesa-dzn-bufpool.patch" 2>/dev/null && git -C "$1" apply "$tc/mesa-dzn-bufpool.patch"
# Keep D3D12 PSOs across runs in an ID3D12PipelineLibrary file named by DZN_PSO_LIBRARY (RPCS3 sets it per game).
git -C "$1" apply --check "$tc/mesa-dzn-psolib.patch" 2>/dev/null && git -C "$1" apply "$tc/mesa-dzn-psolib.patch"
# D3D12 command-list call tracer (logs the last calls when a command list fails to close).
cp "$tc/mesa-dzn_trace.c" "$1/src/microsoft/vulkan/dzn_trace.c"
cp "$tc/mesa-dzn_trace_macros.h" "$1/src/microsoft/vulkan/dzn_trace_macros.h"
if [ -f "$2/meson-private/coredata.dat" ]; then
	setup_mode=--wipe
else
	# A cancelled/cache-saved first run can leave a non-Meson directory behind.
	# Meson refuses both --wipe and a fresh setup in that state.
	rm -rf "$2"
	setup_mode=
fi
meson setup $setup_mode "$2" "$1" --cross-file "$cross_file" --buildtype=release \
	-Dvulkan-drivers=microsoft-experimental -Dgallium-drivers= -Dopengl=false -Dgles1=disabled -Dgles2=disabled \
	-Degl=disabled -Dglx=disabled -Dplatforms=windows -Dllvm=disabled -Dzlib=disabled -Dzstd=disabled \
	-Dexpat=disabled -Dbuild-tests=false -Dmicrosoft-clc=disabled -Dspirv-to-dxil=false -Dvideo-codecs=
ninja -j "$jobs" -C "$2"
