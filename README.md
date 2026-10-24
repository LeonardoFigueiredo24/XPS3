RPCS3 for Xbox (UWP)
====================

> **Proof of concept.** This is an unofficial, experimental port of the [RPCS3](https://rpcs3.net)
> PlayStation 3 emulator to Xbox Series X|S as a UWP app (Developer Mode). It is **not** supported by
> the RPCS3 team, so please don't ask them for help with it. Expect crashes, slow games and rough edges.
>
> The port is about 99% vibecoded: written with AI assistance (Claude), steered and checked with my
> previous programming knowledge. :)

**Video of it running on YouTube** (click the picture):

[![RPCS3 for Xbox running on an Xbox Series X](https://img.youtube.com/vi/VAKe_y5I3iw/maxresdefault.jpg)](https://youtu.be/VAKe_y5I3iw)

Downloads are on the [Releases](../../releases) page. Building, packaging, internals, all settings and
known issues are in [uwp/README.md](uwp/README.md).

## What works (October 2026)

| | |
| --- | --- |
| PS3 system menu (XMB, firmware 4.93) | Steady 60 fps, network and web browser work |
| Portal 2 | Playable, ~28 fps (limited by the emulated PPU main thread) |
| Minecraft PlayStation 3 Edition | Playable, ~57 fps (pick "Continue playing offline" at the PSN prompt) |
| Jetpack Joyride Deluxe | Playable, 60 fps |
| PlayStation Home | Launches and works, ~20 fps; sometimes crashes in the main plaza (runs out of memory) |
| GTA V, Watch Dogs, Plants vs. Zombies Garden Warfare | Boot, but not playable (single-digit fps or out of memory) |

Heavy games are mostly CPU-bound: the Xbox runs the PPU/SPU recompilers fine, but big games need
more CPU and memory than a UWP game app gets (5 GB). The first start of the XMB or a game compiles
its code and shaders and can take several minutes; later starts use the cache.

## Installing on your Xbox

You need an Xbox Series X|S in **Developer Mode**, a PC on the same network, a **USB drive or
external hard drive** (the emulated PS3 hard drive, games and caches live there; the app's own
storage is far too small), and your own PS3 firmware file (`PS3UPDAT.PUP`, 4.93 is tested) and games.

1. Put the console in Developer Mode (Microsoft's "Xbox Dev Mode" app and a developer account) and
   enable the Device Portal in Dev Home. Open it in a browser on your PC: `https://<your-xbox-ip>:11443/`.
2. From the release, download the `.appx` and `Microsoft.UI.Xaml.2.8.appx`.
3. In Device Portal: **Home > Add**, choose the RPCS3 `.appx`, then on the next page add
   `Microsoft.UI.Xaml.2.8.appx` as a dependency, and install.
4. In Dev Home on the Xbox, highlight **RPCS3 Xbox (Beta)**, press View > **View details** and set
   **App type: Game** (gives the app 5 GB of RAM instead of 1 GB). Reinstalling can reset this.
5. Plug in the USB drive and start the app. Copy `PS3UPDAT.PUP` to the root of the USB drive (or
   `PS3/UPDATE/`), open the **Install** tab and install the firmware.
6. Put games on the USB drive: game folders (with `PS3_GAME` inside) or `.iso` files in `GAMES/`,
   `PS3ISO/` or the root, and `.pkg`/`.rap` files in `PKG/` or the root (install those from the
   Install tab). Press Y in the menu to look again. Boot the XMB or a game from the **Games** tab.

### Controls

| Xbox | PS3 |
| --- | --- |
| A / B / X / Y | Cross / Circle / Square / Triangle |
| Menu / View | Start / Select |
| Tap View + Menu | PS button |
| Hold View + Menu ~1 s | Pause menu (resume, stop the game, live settings) |

In the app menu: D-pad or left stick to move, A select, B back, LB/RB switch tabs, Y rescan.

## Contributing

Pull requests are very welcome, and so are compatibility reports: if you try a game, open an issue
with the game's name and title ID, what happened (fps, crashes, glitches) and the app version.

## License and credits

This port is based on [RPCS3](https://github.com/RPCS3/rpcs3) and keeps its license (GPL-2.0, see
[LICENSE](LICENSE)); all emulation credit goes to the RPCS3 team and contributors. Graphics run on
[Mesa](https://mesa3d.org)'s Dozen driver (Vulkan on D3D12, MIT license) with Microsoft's DXIL
signer. The menu uses Microsoft's WinUI 2 (MIT). The source of everything in a release is in this
repository.

The upstream RPCS3 README follows.

---

RPCS3
=====

[![GitHub Actions](https://img.shields.io/github/actions/workflow/status/RPCS3/rpcs3/rpcs3.yml?branch=master&logo=github&label=Actions)](https://github.com/RPCS3/rpcs3/actions/workflows/rpcs3.yml)
[![RPCS3 Discord Server](https://img.shields.io/discord/272035812277878785?color=5865F2&label=RPCS3%20Discord&logo=discord&logoColor=white)](https://discord.gg/rpcs3)

The world's first free and open-source PlayStation 3 emulator/debugger, written in C++ for Windows, Linux, macOS and FreeBSD.

You can find some basic information on our [**website**](https://rpcs3.net/). Game info is being populated on the [**Wiki**](https://wiki.rpcs3.net/).
For discussion about this emulator, PS3 emulation, and game compatibility reports, please visit our [**forums**](https://forums.rpcs3.net) and our [**Discord server**](https://discord.gg/RPCS3).

[**Support the Lead Developers on Patreon**](https://rpcs3.net/patreon)

## Contributing

If you want to help the project but do not code, the best way to help out is to test games and make bug reports. See:
* [Quickstart](https://rpcs3.net/quickstart)

If you want to contribute as a developer, please take a look at the following pages:

* [Coding Style](https://github.com/RPCS3/rpcs3/wiki/Coding-Style)
* [Developer Information](https://github.com/RPCS3/rpcs3/wiki/Developer-Information)

You should also contact any of the developers in the forums or in the Discord server to learn more about the current state of the emulator.

### AI Use

Use of AI tools for research and reverse engineering purposes is permitted. However, contributors are expected to fully own and understand all code they submit. Any communication with the team — including code, code comments, and GitHub comments — must come from the human contributor, not an AI agent acting autonomously.

We have unfortunately seen a rise in untested and unverified AI-generated slop being submitted to this project. This wastes maintainer time and, in worse cases, such changes get merged and break functionality for all users. Repeated violations will result in a ban from the repository. Please be respectful of everyone's time.

**Pull requests opened by AI agents or automated tools must include a disclosure in the PR description** stating the scope of AI involvement — which parts were AI-generated and what human testing or review was performed prior to submission. PRs that omit this disclosure may be closed without review.

If you are unsure about your work, open a discussion issue to talk it through with the team, or reach out to a maintainer on [Discord](https://discord.gg/RPCS3).

## Building

See [BUILDING.md](BUILDING.md) for more information about how to setup an environment to build RPCS3.

## Running

Check our friendly [quickstart](https://rpcs3.net/quickstart) guide to make sure your computer meets the minimum system requirements to run RPCS3.

Don't forget to have your graphics driver up to date and to install the [Visual C++ Redistributable Packages for Visual Studio 2022](https://aka.ms/vs/17/release/VC_redist.x64.exe) if you are a Windows user.

## License

Most files are licensed under the terms of GNU GPL-2.0-only License; see LICENSE file for details. Some files may be licensed differently; check appropriate file headers for details.
