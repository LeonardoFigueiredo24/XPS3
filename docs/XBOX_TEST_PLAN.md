# Xbox Series S test plan for XPS3 0.3.5.0

Run tests in Xbox Developer Mode with **App type: Game**. Keep the first run focused on infrastructure; do not begin with a heavy commercial title.

## Installation evidence

1. Verify the APPX and dependency hashes against `dist/checksums.sha256`.
2. Install `Microsoft.UI.Xaml.2.8.appx` only if Device Portal reports it missing.
3. Install the XPS3 APPX and set App type to Game.
4. Photograph the first screen and confirm that the package displays version `0.3.5.0`.

## Functional order

1. Confirm the app reports an approximately 5120 MiB memory limit.
2. Confirm controller navigation, A selection, B return, Y rescan and LB/RB tabs.
3. Confirm JIT initialization and that no executable-memory error appears in the log.
4. Re-run `gs_gcm_basic_triangle.elf` to confirm the already observed colored-triangle output. Then run `pad_test.elf`, press every Xbox control shown by the test for at least one second, and photograph the final input report rather than the PPU compilation screen.
5. Confirm firmware selection and installation using a legitimate `PS3UPDAT.PUP`.
6. Open **Configurações > Armazenamento** and select one internal folder and one USB folder. Restart after changing install/cache/log destinations and confirm the choices persist.
7. Add a configured library folder containing a `PS3_GAME` folder, ISO and ELF; confirm the rescan lists only real content and does not duplicate entries.
8. Place a legal PNG/JPG cover in the configured cover folder using the title ID as the filename; confirm it appears. Also test `cover.png` inside an extracted game folder.
9. Start `gs_gcm_basic_triangle.elf`; photograph the unified preparation phases and confirm the emulated frame only replaces that screen after the triangle is rendered.
10. Confirm USB discovery for PKG/RAP and verify the install confirmation shows the configured destination. Confirm a deliberately oversized PKG is refused before writing when the target lacks space.
11. Disconnect the USB only while no install or game is active, restart, and record the exact warning without writing new data.
12. Confirm log export, then boot the PS3 system menu or a freely distributable homebrew test.
13. Only then test a legally obtained game already known to boot in the upstream port.

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
