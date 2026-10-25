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

This lock does not cover the Microsoft binary framework dependencies used only
during packaging (`Microsoft.Direct3D.DXC` and `Microsoft.UI.Xaml`). Those will be
downloaded as versioned NuGet archives and recorded with SHA-256 checksums before
the first package is produced.
