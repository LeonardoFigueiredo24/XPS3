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

`uwp/app` is a CoreWindow shell with a controller menu (`uwp_frontend.cpp`), drawn on the CPU
and shown through the shell's D3D11 swapchain. RPCS3 writes `RPCS3.log` to `LocalState/rpcs3/`.

- **Games**: game folders (disc layout with `PS3_GAME/`, or HDD layout with `PARAM.SFO` and
  `USRDIR/EBOOT.BIN`) and `.iso` files in `LocalState/games/`, games installed into the emulated
  HDD, `GAMES/` folders on USB drives, loose programs in `LocalState/boot/` and the bundled test
  programs. A starts one; in game, hold View + Menu for a second to stop it and come back.
- **Firmware**: installs `PS3UPDAT.PUP` found in `LocalState/`, `LocalState/firmware/` or on a USB
  drive (root or `PS3/UPDATE/`), the same way desktop RPCS3 does. Without firmware every firmware
  library uses RPCS3's built-in (HLE) version.
- **Settings**: renderer, resolution scale, frame limit, VSync, shader mode and a few others,
  saved to `config.yml` right away.

Controls: D-pad or left stick to move, A to select, B to cancel, LB/RB to switch tabs, Y to rescan.
On a PC keyboard: arrows, Enter, Escape, Q/E (or Tab), F5.

An empty `LocalState/autoboot` file skips the menu and boots the first file in `LocalState/boot/`
(or the bundled GCM triangle test). While a game boots or runs without a frame to show, the screen
is blue; green means it exited cleanly and red means a boot failure or fatal error.

## Known gaps
- PPU/SPU run on the interpreters (no LLVM yet).
- No audio yet; USB drive access depends on broadFileSystemAccess working for plain file APIs.
- Imports pdh.dll, dbghelp.dll and MIDI functions from winmm.dll, which may be missing on Xbox.
