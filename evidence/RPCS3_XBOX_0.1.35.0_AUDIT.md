# RPCS3 Xbox 0.1.35.0 update audit — 2026-10-25

The user supplied the release source in GitHub ZIP, release ZIP and release tarball forms, plus the
compiled AppX and its WinUI dependency. All inspection was static; no supplied executable or DLL was
run.

## Supplied artifact hashes

```text
0cd9ee332747076def638c89de48eef9ddf3a436926dac15db6e919ab883de77  rpcs3-xbox-main.zip
296324c7f87ed3f62d5699caedbd80aec4ad6abfb396d8457a1cfafdffa5a5e4  rpcs3-xbox-0.1.35.0.tar.gz
b813c0c5922762b1de4603bab35b7cd91b29e357af1c5c643f4e108423c9f9a2  rpcs3-xbox-0.1.35.0.zip
6b5ae5832a32806145a4e86225258f61abfe4788b19dfca4504511e706e9371d  RPCS3Xbox-0.1.35.0.appx
1421dba0b7f1da87e6ad52e17c25d38bcd2159bd7f91c85034cfa837238abaaa  Microsoft.UI.Xaml.2.8.appx
```

The three source archives contain the same 2,118 files with no content differences. Comparison with
the official `emelin2004/rpcs3-xbox` tag `v0.1.35.0`, commit
`44587f2cfc0eaca8ffe1f37ace8ce16cae2f1bc3`, found 358 exact byte matches and 1,760 text files that
match after CRLF/LF normalization. No file had a substantive difference.

## Package inspection

- Identity: `RPCS3Xbox`, version `0.1.35.0`, x64.
- Publisher/signing subject: `CN=RPCS3XboxProbe`; Windows reports the chain ending in an untrusted
  root, as expected for that project's development certificate. XPS3 does not reuse it.
- Executables: `rpcs3_uwp.exe` and `rpcs3_uwp_importcheck.exe`.
- Graphics payload: `vulkan_dzn.dll`, `dxil.dll`, `dxcompiler.dll`.
- Capabilities: `codeGeneration`, internet client/server, private network, broad filesystem and
  removable storage. Their presence is not treated as proof that each operation works on Xbox.
- The supplied `Microsoft.UI.Xaml.2.8.appx` has a valid Microsoft signature and manifest identity
  version `8.2501.31001.0`, matching the package dependency.

## Changes integrated into XPS3

The complete upstream commit was merged so the RPCS3 core remains coherent rather than copying
isolated files. Notable changes relevant to Xbox include:

- RPCS3 core update to upstream `8b9ee6056`, including PPU/SPU, LV2, RSX, filesystem and decoder
  fixes.
- SPU scheduling, mailbox, DMA and group lifecycle corrections.
- RSX semaphore waits that yield instead of consuming a CPU core on long UWP waits.
- UWP suspend/resume now pauses and restores the running emulation state.
- Device-loss handling avoids a tight `Present` failure loop and reports the D3D failure once.
- Longer, instrumented shutdown handling for slow cache/storage completion.
- ISO repeated-delimiter handling and expanded ISO tests.
- Input blob detection split from OpenCV; the obsolete OpenCV build dependency was removed.

XPS3 retains its own identity, executable names, storage root, assets and offline telemetry/update
implementations. The imported certificate, publisher identity, executable names and telemetry upload
endpoint are not used.

Status: source integrated, not yet compiled after this merge and not tested on Xbox hardware.
