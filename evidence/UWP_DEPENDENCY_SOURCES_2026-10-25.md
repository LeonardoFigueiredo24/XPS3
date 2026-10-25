# UWP dependency source acquisition — 2026-10-25

Environment: Docker image `xps3-uwp-builder:2026-10-25`, persistent build volume
`xps3-build`.

Command:

```sh
/src/uwp/xps3-fetch-sources.sh /build/sources
```

Observed result:

```text
ffmpeg refs/tags/n8.1 9047fa1b084f76b1b4d065af2d743df1b40dfb56
mesa refs/tags/mesa-26.2.5 ddaf85e353442d9d7f192fdcf1823e652e3f2ee3
Vulkan-Headers refs/tags/v1.4.335 2fa203425eb4af9dfc6b03f97ef72b0b5bcb8350
XPS3_SOURCE_FETCH=PASS
```

The first FFmpeg attempt correctly failed because the initial lock used the annotated tag object's
SHA rather than its peeled commit. The lock was corrected for every annotated tag and the command
then passed twice. Existing checkouts with a different HEAD or local changes are rejected.

Status: source acquisition verified. This is not evidence that FFmpeg, Mesa, LLVM, RPCS3 or an AppX
has compiled, and is not evidence of execution on Xbox hardware.

## Microsoft package inputs

The official NuGet archives were downloaded by exact version, hashed and inspected without running
their executables:

```text
73cfa082245918c2cc1ec6740250eb7377e83a757b3b3a3e16e0d71535bfbfc3  Microsoft.Direct3D.DXC 1.9.2609.5
79207b10fe243eb1a8dcdc29beceba2f472f145ed31f147f7ae3f43b0659c9f7  Microsoft.UI.Xaml 2.8.7
```

`dxil.dll` and `dxcompiler.dll` were identified as x86-64 PE DLLs. The WinUI archive contains the
x64 `Microsoft.UI.Xaml.2.8.appx`; its signed manifest identity is `Microsoft.UI.Xaml.2.8`, version
`8.2501.31001.0`, which matches the XPS3 manifest dependency's name and minimum version.
