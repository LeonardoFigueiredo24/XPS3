#!/bin/sh
# Continue a release after the long LLVM build has completed. Run inside the builder container.
set -eu

root=${XPS3_SOURCE_ROOT:-/src}
state=${XPS3_BUILD_ROOT:-/build}
export XPS3_JOBS=${XPS3_JOBS:-1}

[ -f "$state/llvm-install/lib/cmake/llvm/LLVMConfig.cmake" ] || {
    echo "LLVM is not complete; run xps3-docker-build-llvm.sh first" >&2
    exit 1
}

"$root/uwp/xps3-fetch-sources.sh" "$state/sources"
"$root/uwp/xps3-fetch-packages.sh" "$state/packages"
"$root/uwp/xps3-docker-build-ffmpeg.sh"
"$root/uwp/xps3-configure-build.sh" \
    "$state/app-build" \
    "$state/ffmpeg-install/lib" \
    "$state/llvm-install/lib/cmake/llvm" \
    "$state/sources/Vulkan-Headers/include" \
    "$state/evidence/xps3-build.log"
"$root/uwp/xps3-docker-build-mesa.sh"
"$root/uwp/xps3-docker-build-msix.sh"
"$root/uwp/xps3-docker-package.sh"
