# XPS3 0.3.5.0 storage manager design

Date: 2026-10-29

## Purpose

Give Xbox Series X|S Developer Mode users explicit, controller-friendly control over where XPS3 discovers playable content and where RPCS3 writes installed packages, firmware data, licenses, saves, caches, logs and captures. The feature must work within UWP permissions, must not claim arbitrary access to the Xbox operating-system drive, and must prevent writes when a removable device is missing or full.

## Platform boundary

The Xbox internal option means the XPS3 package's private `ApplicationData.LocalFolder` (`LocalState`). It does not mean an arbitrary folder on the console SSD. The UI must describe this as **Armazenamento interno do XPS3** and show measured free space before enabling an install.

USB and external drives are discovered through the file APIs already used by the port. A user-selected folder is accepted only after it passes read, create, write, rename and delete probes. Where the UWP folder picker works, the selected folder is retained through `FutureAccessList`; the Xbox-oriented in-app volume/folder browser remains available because picker support must be proven on hardware rather than assumed.

## Storage model

XPS3 exposes five independent categories:

1. **Bibliotecas de jogos** — zero or more folders containing `PS3_GAME`, HDD-layout games, ISO, ELF, SELF or `EBOOT.BIN`. These paths are scanned and combined into one library. Library content is opened in place and is not copied automatically.
2. **Sistema PS3 ativo** — one writable profile containing `dev_hdd0` plus the profile's `dev_flash*` firmware directories: installed PKG games, updates, DLC, licenses, game data, users, saves and installed PS3 firmware. RPCS3 requires one coherent active virtual system, so only one profile can be mounted for writes and launches at a time.
3. **Cache e shaders** — Automatic, internal XPS3 storage, same location as the active PS3 profile, or a custom writable folder.
4. **Logs e capturas** — internal XPS3 storage or a custom writable folder. A failed external destination falls back to internal logs without redirecting package or save writes.
5. **Capas manuais** — `LocalState/XPS3/COVERS` by default or a custom authorized folder. XPS3 does not download artwork. The user copies legally obtained `.png` or `.jpg` files into this folder and may change the destination between internal storage and USB.

Each external PS3-system location is a named storage profile. The initial built-in profiles are **Interno do XPS3** and one profile per writable external drive. A custom profile may point at a user-selected folder. Profile data is stored in `LocalState/storage_profiles.json`; sensitive account data is not stored there.

## Default behavior

- On a first start with a writable external drive, XPS3 recommends `<drive>:\XPS3` but asks for confirmation before making it the active PS3 system.
- Without an external drive, XPS3 uses its private internal folder only when measured free space is sufficient. Otherwise package installation is disabled with a clear explanation.
- The last valid active profile is restored on launch.
- If the active external profile is absent, XPS3 does not silently redirect `dev_hdd0` to another device. It enters a read-only/missing-storage state and offers **Reconectar**, **Escolher outro perfil** or **Usar armazenamento interno**.
- Existing installations are never moved automatically. Migration is a separate confirmed operation.

## User interface

Settings gains an **Armazenamento** section with these cards:

- **Sistema PS3 ativo** — profile name, device, full path, installed firmware version, available/total space and connection state.
- **Pastas da biblioteca** — add, remove, enable/disable and rescan multiple folders.
- **Cache e shaders** — destination and current size.
- **Logs e capturas** — destination and an export action.
- **Capas manuais** — destination, current size, rescan action and the accepted filename for the selected title.
- **Gerenciar perfis** — create, rename, validate, switch or remove a profile. Removing a profile removes only its XPS3 reference, never its files.
- **Migrar dados** — copy the active PS3 system to another profile using partial files and post-copy verification. The source remains intact until the destination is verified.

All actions are navigable with the Xbox controller. Paths wrap across multiple lines. Device cards show **Interno**, **USB/externo**, **Conectado**, **Ausente**, **Somente leitura** or **Espaço insuficiente** rather than relying on drive letters alone.

## Folder selection

The in-app browser lists only storage roots XPS3 can actually enumerate. It supports:

- selecting the current folder;
- entering an existing child folder;
- returning to the parent within the authorized root;
- creating a new child folder;
- showing free space and write status;
- cancelling without changing configuration.

The browser never accepts an unresolved path, a package-install directory, the app installation directory, or a path outside the authorized root. A Windows UWP `FolderPicker` action may be offered as an additional option and its access retained, but failure or cancellation leaves the previous configuration unchanged.

## Manual cover resolution

The central cover folder is searched without modifying the original game. A valid PS3 `Title ID` uses `<TITLE_ID>.png` or `<TITLE_ID>.jpg`; homebrew and entries without a title ID use a stable content hash. Matching is case-insensitive, while duplicate names prefer `.png`. The lookup priority is:

1. central manual cover;
2. the game's embedded `ICON0.PNG`;
3. a same-name `.png`, `cover.png` or `ICON0.PNG` beside an ISO or folder, preserving the existing compatibility behavior;
4. the generated placeholder.

Images are decoded off the XAML thread, cached by path, size and modification time, and rejected when the encoded file is larger than 16 MiB or either decoded dimension exceeds 4096 pixels. An invalid image produces a warning for that title and falls back to the next source. If an external cover folder is absent, the library remains usable and shows embedded artwork or placeholders. Changing the cover folder or selecting **Atualizar capas** invalidates only the artwork cache and does not rescan or alter game data.

## PKG installation flow

Selecting a PKG opens a destination confirmation screen containing:

- package title, ID and source path;
- active PS3 profile and destination path;
- available space and estimated required space;
- **Instalar no destino padrão**;
- **Escolher armazenamento**;
- **Cancelar**.

Choosing another destination selects or creates a PS3 storage profile. XPS3 switches the active VFS only while the emulator is stopped, reinitializes the core, verifies the new `dev_hdd0`, then starts extraction. The selected profile becomes the default only when the user enables **Usar como padrão**.

PKG data is always installed into `<profile>/dev_hdd0/game` using the existing RPCS3 package installer. RAP files beside the package are copied into the same profile's user `exdata` folder. Firmware installation writes the RPCS3 `dev_flash*` directories inside the selected active profile, so switching profiles also switches the installed firmware view. A profile without firmware is labelled accordingly and prompts for a legitimate `PS3UPDAT.PUP` when firmware-dependent content is started.

The preflight check uses the target profile, not the source PKG device. It requires package size plus 64 MiB of headroom at minimum; if a more accurate unpacked-size estimate is available, the larger value is used. Extraction failure never reports success and leaves enough information in the log to identify the destination.

## Launch behavior across profiles

Library entries record their source path and, for installed games, their owning PS3 profile. Starting an installed title from a profile other than the active one prompts the user to switch profiles. Switching is allowed only when emulation and installation jobs are stopped. XPS3 reinitializes the VFS, refreshes installed content and then starts the selected title.

Disc folders, ISO and homebrew stored in library folders remain launchable in place; their saves and game data are written to the active PS3 profile.

## USB removal and data integrity

- Every write job retains the expected volume identity and validates it before each stage.
- Temporary files use `.xps3part`; final names appear only after a successful rename.
- Removing the active device during an install or migration cancels the job, retains recoverable partial data and shows a device-removed error.
- Save writes remain RPCS3-managed inside the active `dev_hdd0`. XPS3 does not move individual save folders while a title is running.
- A profile may not be switched or removed while emulation, package installation, firmware installation or migration is active.
- Migration copies first, validates file count and byte count, writes the new profile marker, then switches. Source deletion is a separate explicit action.

## Persistence format

`LocalState/storage_profiles.json` contains a schema version, active profile identifier, default install profile identifier, library folders, cache policy, log policy, manual-cover policy and profile records. Each record contains an opaque ID, display name, device kind, authorized path/token when available, last known volume identity and last validation result. Unknown future fields are ignored; invalid files are preserved as `.invalid` and defaults are reconstructed without deleting user content.

## Error handling

The UI distinguishes these cases:

- destination disconnected;
- permission denied or authorization expired;
- destination read-only;
- insufficient space;
- source PKG unreadable or damaged;
- destination changed during the operation;
- VFS switch failed;
- migration partially completed.

Errors include the affected profile and path. XPS3 never labels an install complete unless RPCS3 returns success and the installed content is discoverable under the selected profile.

## Test strategy

Static integration tests first assert that the storage manager, profile persistence, destination selector, target-based free-space check, manual-cover folder and localized UI markers exist. Unit-level path/profile tests cover valid internal and external profiles, missing devices, read-only destinations, duplicate library roots, path normalization and malformed persistence data. Cover tests verify Title-ID naming, stable homebrew keys, `.png` preference, invalid/oversized-image fallback and a missing removable cover folder.

Xbox Series S validation then covers:

1. internal profile creation and measured space;
2. external profile creation in a custom folder;
3. persistence across app restart;
4. multiple library folders on different devices;
5. PKG destination selection and RAP placement;
6. refusal on insufficient target space;
7. safe behavior when USB is removed before and during installation;
8. profile switching before launching installed content;
9. cache and log destination selection;
10. migration with source preservation.
11. manual covers loaded from internal and USB folders, including fallback after USB removal.

No commercial title is classified as playable solely because its package installs or its cover appears.

## Release scope

Version 0.3.5.0 targets the storage manager, destination selection, persisted profiles, multiple library folders, safe PKG placement and a configurable manual-cover folder. Artwork downloading, arbitrary access to protected Xbox folders, automatic deletion of old installations, cloud synchronization and per-title save redirection are explicitly out of scope.
