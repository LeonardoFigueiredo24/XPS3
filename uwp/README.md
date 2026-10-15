# RPCS3 for Xbox (UWP, Dev Mode)

Unofficial port. Not supported by the RPCS3 team; don't ask them for help with it.

## Building the core (Linux host)

```sh
pip install ziglang cmake ninja
cmake -S . -B build-uwp -G Ninja \
  -DCMAKE_TOOLCHAIN_FILE=uwp/toolchain/toolchain-uwp.cmake -DCMAKE_BUILD_TYPE=Release \
  -DRPCS3_UWP=ON -DWITH_LLVM=OFF -DUSE_VULKAN=OFF -DUSE_NATIVE_INSTRUCTIONS=OFF -DUSE_LTO=OFF \
  -DUSE_SDL=OFF -DUSE_FAUDIO=OFF -DUSE_SYSTEM_ZLIB=OFF -DUSE_SYSTEM_CURL=OFF -DUSE_SYSTEM_SDL=OFF \
  -DUSE_SYSTEM_OPENCV=OFF -DUSE_DISCORD_RPC=OFF -DUSE_GAMEMODE=OFF -DUSE_LIBEVDEV=OFF
ninja -C build-uwp rpcs3_emu
```

`RPCS3_UWP` reuses upstream's Qt-less (Android) build path: no Qt, no OpenGL/Vulkan yet.

## Status
See the plan in the project files. Current step: compiling the core, then a UWP shell that boots homebrew with the null renderer.
