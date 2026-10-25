# XPS3 UWP external source lock

These revisions are inputs to the experimental Xbox UWP build. The fetch script
verifies the resolved commit before checking out or building any source.

| Component | Upstream ref | Commit | Purpose |
|---|---|---|---|
| FFmpeg | `n8.1` | `9047fa1b084f76b1b4d065af2d743df1b40dfb56` | RPCS3 audio/video codecs |
| Mesa | `mesa-26.2.5` | `ddaf85e353442d9d7f192fdcf1823e652e3f2ee3` | Dozen Vulkan-to-D3D12 driver |
| Vulkan-Headers | `v1.4.335` | `2fa203425eb4af9dfc6b03f97ef72b0b5bcb8350` | Vulkan API headers |

Run in the XPS3 builder container:

```sh
/src/uwp/xps3-fetch-sources.sh /build/sources
```

## Microsoft redistributable package lock

| Package | Version | NuGet SHA-256 | Files used |
|---|---|---|---|
| Microsoft.Direct3D.DXC | `1.9.2609.5` | `73cfa082245918c2cc1ec6740250eb7377e83a757b3b3a3e16e0d71535bfbfc3` | x64 `dxil.dll`, `dxcompiler.dll`, license files |
| Microsoft.UI.Xaml | `2.8.7` | `79207b10fe243eb1a8dcdc29beceba2f472f145ed31f147f7ae3f43b0659c9f7` | x64 framework `.appx`, license |

`uwp/xps3-fetch-packages.sh` downloads the archives only from NuGet's official API, verifies the
complete archive before extraction and stages only the documented x64 redistributables. No package
publisher identity, application identity or signing certificate is reused.
