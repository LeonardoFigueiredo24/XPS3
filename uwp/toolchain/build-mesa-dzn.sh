#!/bin/sh
# Cross-build Mesa's Dozen Vulkan driver (vulkan_dzn.dll) for the UWP port.
# Usage: build-mesa-dzn.sh <mesa source (tested: 26.2.5)> <build dir>
# Run after the RPCS3 CMake configure step, which creates uwp-implibs (see meson-cross-uwp.ini).
set -e
tc=$(dirname "$(realpath "$0")")
# Port patch: expose VK_EXT_shader_uniform_buffer_unsized_array (RPCS3 shader interpreter needs it).
git -C "$1" apply --check "$tc/mesa-dzn-uwp.patch" 2>/dev/null && git -C "$1" apply "$tc/mesa-dzn-uwp.patch"
meson setup --wipe "$2" "$1" --cross-file "$tc/meson-cross-uwp.ini" --buildtype=release \
	-Dvulkan-drivers=microsoft-experimental -Dgallium-drivers= -Dopengl=false -Dgles1=disabled -Dgles2=disabled \
	-Degl=disabled -Dglx=disabled -Dplatforms=windows -Dllvm=disabled -Dzlib=disabled -Dzstd=disabled \
	-Dexpat=disabled -Dbuild-tests=false -Dmicrosoft-clc=disabled -Dspirv-to-dxil=false -Dvideo-codecs=
ninja -C "$2"
