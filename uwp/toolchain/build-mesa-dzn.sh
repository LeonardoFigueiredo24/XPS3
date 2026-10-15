#!/bin/sh
# Cross-build Mesa's Dozen Vulkan driver (vulkan_dzn.dll) for the UWP port.
# Usage: build-mesa-dzn.sh <mesa source (tested: 26.2.5)> <build dir>
# Run after the RPCS3 CMake configure step, which creates uwp-implibs (see meson-cross-uwp.ini).
set -e
tc=$(dirname "$(realpath "$0")")
meson setup --wipe "$2" "$1" --cross-file "$tc/meson-cross-uwp.ini" --buildtype=release \
	-Dvulkan-drivers=microsoft-experimental -Dgallium-drivers= -Dopengl=false -Dgles1=disabled -Dgles2=disabled \
	-Degl=disabled -Dglx=disabled -Dplatforms=windows -Dllvm=disabled -Dzlib=disabled -Dzstd=disabled \
	-Dexpat=disabled -Dbuild-tests=false -Dmicrosoft-clc=disabled -Dspirv-to-dxil=false -Dvideo-codecs=
ninja -C "$2"
