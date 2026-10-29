# Xbox Series S test plan for XPS3 0.3.4.0

Run tests in Xbox Developer Mode with **App type: Game**. Keep the first run focused on infrastructure; do not begin with a heavy commercial title.

## Installation evidence

1. Verify the APPX and dependency hashes against `dist/checksums.sha256`.
2. Install `Microsoft.UI.Xaml.2.8.appx` only if Device Portal reports it missing.
3. Install the XPS3 APPX and set App type to Game.
4. Photograph the first screen and record whether long game titles wrap fully and cover art appears for a game with `ICON0.PNG`.

## Functional order

1. Confirm the app reports an approximately 5120 MiB memory limit.
2. Confirm controller navigation, A selection, B return, Y rescan and LB/RB tabs.
3. Confirm JIT initialization and that no executable-memory error appears in the log.
4. Run `gs_gcm_basic_triangle.elf`, then `pad_test.elf`, and record the visible result.
5. Confirm firmware selection and installation using a legitimate `PS3UPDAT.PUP`.
6. Confirm USB discovery for a `PS3_GAME` folder, ISO, PKG/RAP and ELF/SELF/EBOOT.BIN; confirm they are listed under **Adicionar e instalar** with the correct action.
7. Confirm a deliberately oversized PKG is refused before writing when the target USB lacks space.
8. Confirm safe removal handling and log export.
9. Boot the PS3 system menu or a freely distributable homebrew test.
10. Only then test a legally obtained game already known to boot in the upstream port.

## Required report

- Package version and SHA-256
- Xbox model and OS version
- App type shown in Dev Home
- Title ID and exact content format/path
- Cold-boot and warm-boot times
- FPS after caches stabilize, minimum FPS and visible stutter
- Peak application memory
- Audio, controller and rendering result
- Crash/error text and exported RPCS3 log
- Photo or short video showing the result

Recognizing a game or reaching a loading screen is not enough to classify it as playable.
