# RPCS3 for Xbox (UWP, Dev Mode)

Unofficial port. Not supported by the RPCS3 team; don't ask them for help with it.

## Building (Linux host)

```sh
pip install ziglang cmake ninja        # also needs nasm, llvm-dlltool, llvm-windres, llvm-nm
git clone --depth 1 --branch n8.1 https://github.com/FFmpeg/FFmpeg.git ffmpeg-src
uwp/toolchain/build-ffmpeg.sh ffmpeg-src ffmpeg-build ffmpeg-install

cmake -S . -B build-uwp -G Ninja \
  -DCMAKE_TOOLCHAIN_FILE=uwp/toolchain/toolchain-uwp.cmake -DCMAKE_BUILD_TYPE=Release \
  -DRPCS3_UWP=ON -DRPCS3_UWP_FFMPEG_LIB_DIR=$PWD/ffmpeg-install/lib \
  -DWITH_LLVM=OFF -DUSE_VULKAN=OFF -DUSE_NATIVE_INSTRUCTIONS=OFF -DUSE_LTO=OFF -DUSE_PRECOMPILED_HEADERS=OFF \
  -DUSE_SDL=OFF -DUSE_FAUDIO=OFF -DUSE_SYSTEM_ZLIB=OFF -DUSE_SYSTEM_CURL=OFF -DUSE_SYSTEM_SDL=OFF \
  -DUSE_SYSTEM_OPENCV=OFF -DUSE_DISCORD_RPC=OFF -DUSE_GAMEMODE=OFF -DUSE_LIBEVDEV=OFF
ninja -C build-uwp rpcs3_uwp

# signed .appx (makemsix from microsoft/msix-packaging, osslsigncode)
MAKEMSIX=/path/to/makemsix uwp/package/package.sh build-uwp RPCS3Xbox.appx cert.pfx <password>
```

`RPCS3_UWP` reuses upstream's Qt-less (Android) build path: no Qt, no OpenGL/Vulkan yet.

## What the test build does

`uwp/app` is a minimal shell with no UI. It boots the first file in
`LocalState/boot/`, or the bundled `test/ppu_thread.elf`, with the null renderer and
null audio, and writes `RPCS3.log` to `LocalState/rpcs3/`. The screen is blue while booting or
running, green when the program exited cleanly and red when the boot failed.
Without firmware, every firmware library uses RPCS3's built-in (HLE) version.

## Known gaps
- PPU/SPU run on the interpreters (no LLVM yet).
- No graphics, audio, input UI or firmware installer yet.
- Imports pdh.dll, dbghelp.dll and MIDI functions from winmm.dll, which may be missing on Xbox.
