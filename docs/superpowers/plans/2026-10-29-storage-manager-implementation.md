# XPS3 0.3.5.0 Storage Manager Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use `superpowers:executing-plans` to implement this plan task by task. Use `superpowers:test-driven-development` for each feature or bug fix and `superpowers:verification-before-completion` before claiming success.

**Goal:** Replace hard-coded Xbox storage discovery with a persistent, controller-friendly storage manager that lets the user select authorized library, installation, cache, log and manual-cover folders while retaining safe internal defaults.

**Architecture:** Add a platform-independent storage configuration/model layer, a UWP/Xbox discovery and permission adapter, and thin frontend/XAML bindings. The emulator core continues to receive ordinary filesystem paths through the existing VFS APIs. No online artwork service is added: covers are user-supplied files resolved centrally and cached safely.

**Tech Stack:** C++20, C++/WinRT, Windows.Storage, WinUI/XAML, RPCS3 `fs` utilities, CMake/MSBuild, Python static integration tests, GitHub Actions.

**Specification:** `docs/superpowers/specs/2026-10-29-storage-manager-design.md`

## Global constraints

- Do not copy package identities, certificates or branding from third-party emulators.
- Do not claim arbitrary Xbox filesystem access. Every external folder must be selected or otherwise exposed by Xbox UWP policy.
- Never move, delete or overwrite a game, save or firmware directory automatically.
- Preserve the current internal path as a working fallback when an external device is unavailable.
- Reject unsupported or encrypted content clearly; do not implement DRM circumvention.
- Treat `.pkg`, `.iso`, extracted game folders and `.elf` as different content types. A listed format is not evidence that it boots.
- All status labels must describe observed state. Do not show fake free space, fake compatibility or invented progress.

---

## Task 1: Add the pure storage configuration model

**Files:**

- Create: `uwp/app/uwp_storage.h`
- Create: `uwp/app/uwp_storage.cpp`
- Modify: `uwp/app/CMakeLists.txt`
- Modify: `uwp/test_xps3_overlay.py`

**Step 1: Write the failing static integration checks**

Extend `uwp/test_xps3_overlay.py` to require:

- `uwp_storage.h` and `uwp_storage.cpp` in the UWP target;
- schema version `1`;
- five configurable categories: libraries, install destination, cache, logs and covers;
- an explicit internal fallback profile;
- atomic persistence markers (`.tmp` plus replace/rename);
- no URL or network artwork provider.

Run:

```powershell
python uwp/test_xps3_overlay.py
```

Expected: FAIL because the storage model does not exist.

**Step 2: Define the model API**

In `uwp/app/uwp_storage.h`, define `namespace xps3::storage` with:

```cpp
enum class device_kind { internal, external, custom };
enum class validation_state { unknown, ready, missing, read_only, insufficient_space };

struct profile
{
    std::string id;
    std::string name;
    std::string root;
    std::string volume_id;
    device_kind device = device_kind::custom;
    validation_state state = validation_state::unknown;
    u64 free_bytes = 0;
    u64 total_bytes = 0;
};

struct library_root
{
    std::string path;
    std::string profile_id;
    bool enabled = true;
};

struct config
{
    u32 schema = 1;
    std::string active_profile_id;
    std::string install_profile_id;
    std::vector<profile> profiles;
    std::vector<library_root> libraries;
    std::string cache_root;
    std::string log_root;
    std::string cover_root;
};

struct validation_result
{
    validation_state state = validation_state::unknown;
    u64 free_bytes = 0;
    u64 total_bytes = 0;
    std::string message;
};

config make_default(std::string_view local_state_root);
bool load(std::string_view file, config& value, std::string& warning);
bool save(std::string_view file, const config& value, std::string& error);
validation_result validate_profile(const profile& value, u64 required_bytes);
std::string normalize_root(std::string_view value);
std::string stable_content_key(std::string_view title_id, std::string_view content_path);
```

Keep Windows.Storage types out of this header so the logic remains testable on Windows desktop and CI.

**Step 3: Implement safe defaults and persistence**

Implement:

- default internal profile rooted at `<LocalState>/XPS3`;
- default subdirectories `GAMES`, `INSTALL`, `CACHE`, `LOGS` and `COVERS`;
- strict schema validation with fallback to defaults and a warning on malformed files;
- atomic write to `storage.json.tmp`, flush/close, then replace `storage.json`;
- normalization that removes duplicate separators and trailing separators without allowing an empty root;
- stable content key using normalized title ID, falling back to a deterministic path hash.

Use the JSON facility already available in the RPCS3 tree. If no parser is usable in the UWP target, implement a small dedicated serializer/parser that accepts only this schema; do not add a large dependency solely for configuration.

**Step 4: Register sources and rerun checks**

Add both sources to the UWP target in `uwp/app/CMakeLists.txt`.

Run:

```powershell
python uwp/test_xps3_overlay.py
```

Expected: storage-model checks PASS; later UI checks may still fail.

**Step 5: Commit**

```powershell
git add uwp/app/uwp_storage.h uwp/app/uwp_storage.cpp uwp/app/CMakeLists.txt uwp/test_xps3_overlay.py
git commit -m "feat: add persistent Xbox storage model"
```

---

## Task 2: Add the UWP storage platform adapter

**Files:**

- Create: `uwp/app/uwp_storage_platform.h`
- Create: `uwp/app/uwp_storage_platform.cpp`
- Modify: `uwp/app/uwp_main.cpp`
- Modify: `uwp/app/CMakeLists.txt`
- Modify: `uwp/test_xps3_overlay.py`

**Step 1: Add failing checks**

Require markers for:

- internal `ApplicationData::Current().LocalFolder()`;
- `FolderPicker`/authorized-folder persistence;
- removable-volume discovery as best effort only;
- non-destructive fallback when a selected volume disappears;
- free-space validation before installation.

Run the static test and confirm failure.

**Step 2: Define the adapter API**

Define:

```cpp
struct platform_snapshot
{
    storage::config config;
    std::vector<storage::profile> available_profiles;
    std::string config_file;
    std::string warning;
};

bool initialize(std::string_view local_state_root, platform_snapshot& value);
bool refresh(platform_snapshot& value);
bool apply_active_profile(const platform_snapshot& value, std::string& error);
concurrency::task<std::optional<storage::profile>> pick_folder_async(std::string purpose);
bool remember_authorized_folder(std::string_view token, std::string_view purpose, storage::profile& result);
```

The adapter owns Windows.Storage and FutureAccessList use. The pure model must not.

**Step 3: Replace hard-coded cache/HDD selection**

Refactor `choose_cache_dir()` and `place_hdd()` in `uwp_main.cpp` to call the adapter:

- initialize config before `Emu.Init()`;
- set `EmuCallbacks.get_cache_path` from the selected cache root;
- set `g_cfg_vfs` paths from the active profile;
- keep the existing internal path if configuration fails;
- record the exact fallback reason in logs.

Do not enumerate `D:` through `Z:` as proof of accessibility. Drive probing may supplement but never replace explicit authorization.

**Step 4: Handle missing external devices**

At startup and before each boot/install:

- refresh selected profiles;
- mark missing/read-only state;
- keep library metadata visible but disable launch/install for unavailable entries;
- use internal cache/log folders when their configured external root is missing;
- never silently redirect an installation to another destination.

**Step 5: Verify and commit**

Run the static test, configure the UWP project in CI-compatible mode, and commit:

```powershell
git add uwp/app/uwp_storage_platform.* uwp/app/uwp_main.cpp uwp/app/CMakeLists.txt uwp/test_xps3_overlay.py
git commit -m "feat: add UWP storage platform adapter"
```

---

## Task 3: Route library scanning through configured roots

**Files:**

- Modify: `uwp/app/uwp_frontend.h`
- Modify: `uwp/app/uwp_frontend.cpp`
- Modify: `uwp/app/uwp_main.cpp`
- Modify: `uwp/test_xps3_overlay.py`

**Step 1: Add failing checks**

Require the scan worker to consume `config.libraries`, retain the internal default, deduplicate canonical paths and expose unavailable roots rather than hanging.

**Step 2: Extend frontend initialization**

Change `host_info` to provide a storage snapshot callback or immutable storage configuration. Add model types for configured roots and their availability.

**Step 3: Refactor scanning**

Replace broad drive-letter scanning with:

- enabled configured library roots;
- internal default library root;
- optional recognized subdirectories under each root (`GAMES`, `PS3ISO`, extracted-disc folders and homebrew);
- bounded recursive traversal with cancellation;
- canonical-path deduplication;
- per-root error reporting.

Keep recognized content rules separate from successful boot claims.

**Step 4: Verify and commit**

Run static tests and commit:

```powershell
git add uwp/app/uwp_frontend.h uwp/app/uwp_frontend.cpp uwp/app/uwp_main.cpp uwp/test_xps3_overlay.py
git commit -m "feat: scan configured Xbox game folders"
```

---

## Task 4: Add safe manual-cover resolution

**Files:**

- Create: `uwp/app/uwp_cover_store.h`
- Create: `uwp/app/uwp_cover_store.cpp`
- Modify: `uwp/app/uwp_frontend.cpp`
- Modify: `uwp/app/CMakeLists.txt`
- Modify: `uwp/test_xps3_overlay.py`

**Step 1: Write failing checks**

Require the documented priority:

1. central manual `<TITLE_ID>.png`;
2. central manual `<TITLE_ID>.jpg`;
3. embedded `ICON0.PNG`;
4. same-name `.png`, `cover.png`, `ICON0.PNG` beside the game;
5. generated placeholder.

Also require PNG preference, 16 MiB encoded limit, 4096-pixel dimension limit and no network code.

**Step 2: Define resolver API**

```cpp
struct cover_result
{
    std::string source_path;
    icon_data image;
    bool placeholder = false;
    std::string warning;
};

cover_result resolve_manual_cover(
    std::string_view cover_root,
    std::string_view title_id,
    std::string_view content_path,
    const std::function<std::optional<icon_data>(std::string_view)>& embedded_loader);
void invalidate_cover_cache(std::string_view changed_path);
```

**Step 3: Implement bounded decoding and cache invalidation**

- Decode off the XAML thread.
- Cache by canonical path, byte size and modification time.
- Reject oversized encoded files and dimensions.
- Fall back without removing the game from the library.
- Do not download or scrape artwork.

**Step 4: Integrate into ISO/folder/ELF entries**

Replace duplicated cover probing in `add_iso()` and `add_game_folder()` with the resolver. Use stable path hashes for homebrew without a title ID.

**Step 5: Verify and commit**

```powershell
python uwp/test_xps3_overlay.py
git add uwp/app/uwp_cover_store.* uwp/app/uwp_frontend.cpp uwp/app/CMakeLists.txt uwp/test_xps3_overlay.py
git commit -m "feat: resolve configurable manual game covers"
```

---

## Task 5: Add controller-first storage settings UI

**Files:**

- Modify: `uwp/app/uwp_frontend.h`
- Modify: `uwp/app/uwp_frontend.cpp`
- Modify: `uwp/app/uwp_xaml.cpp`
- Modify: `uwp/app/xaml/shell.xaml`
- Modify: `uwp/test_xps3_overlay.py`

**Step 1: Add failing UI checks**

Require a settings section containing:

- active storage profile;
- library folders;
- default install destination;
- cache folder;
- log folder;
- manual-cover folder;
- free space and current availability;
- add/change/remove actions navigable by controller.

**Step 2: Extend the snapshot/command model**

Add immutable view structs for profiles, roots and category paths. Add commands for:

- request folder picker;
- select active/default profile;
- enable/disable/remove a library root;
- choose category folder;
- rescan.

All mutations must serialize through the frontend worker and save atomically.

**Step 3: Build the XAML pages**

Create a storage settings panel consistent with Cell Horizon:

- large TV-safe labels;
- clear focused state;
- explicit `Interno`, `USB` or `Personalizado` badges;
- missing/read-only warnings;
- no path text clipping;
- button legends using Xbox controls.

**Step 4: Verify navigation**

Use keyboard equivalents in the desktop build when Xbox hardware is unavailable. Confirm focus never becomes trapped and long paths wrap or ellipsize with a details view.

**Step 5: Commit**

```powershell
git add uwp/app/uwp_frontend.* uwp/app/uwp_xaml.cpp uwp/app/xaml/shell.xaml uwp/test_xps3_overlay.py
git commit -m "feat: add Xbox storage settings UI"
```

---

## Task 6: Add explicit PKG installation destination selection

**Files:**

- Modify: `uwp/app/uwp_frontend.h`
- Modify: `uwp/app/uwp_frontend.cpp`
- Modify: `uwp/app/uwp_xaml.cpp`
- Modify: `uwp/app/xaml/shell.xaml`
- Modify: `uwp/test_xps3_overlay.py`

**Step 1: Add failing checks**

Require the install confirmation to show source, content type, destination profile/path, required space, available space and cancel/confirm actions.

**Step 2: Extend install commands**

Change the installation API so `start_package_install` receives a destination profile ID. Resolve and validate that profile again immediately before extraction.

**Step 3: Enforce safe installation**

- Require source readability.
- Require destination writability.
- Reserve package size plus documented headroom.
- Install through a staging directory under the chosen destination.
- Commit the completed directory atomically where possible.
- Remove only the staging directory after a failed install.
- Never redirect silently.

**Step 4: Clarify format behavior**

The picker may recognize `.pkg`, `.iso`, extracted folders and `.elf`, but:

- `.pkg` uses the package installer;
- `.iso` and extracted folders are library/boot sources, not copied unless the user explicitly requests a future copy operation;
- `.elf` is homebrew/test content;
- unsupported/encrypted packages produce a clear error.

**Step 5: Verify and commit**

```powershell
python uwp/test_xps3_overlay.py
git add uwp/app/uwp_frontend.* uwp/app/uwp_xaml.cpp uwp/app/xaml/shell.xaml uwp/test_xps3_overlay.py
git commit -m "feat: select safe PKG install destination"
```

---

## Task 7: Migrate legacy paths and document recovery

**Files:**

- Modify: `uwp/app/uwp_storage.cpp`
- Modify: `uwp/app/uwp_storage_platform.cpp`
- Modify: `docs/STORAGE.md`
- Modify: `README.md`
- Modify: `uwp/test_xps3_overlay.py`

**Step 1: Add migration checks**

Require one-time import of existing internal `storage`, `hdd_home`, cache and logs paths without moving data.

**Step 2: Implement non-destructive migration**

- Detect absence of `storage.json`.
- Build configuration referencing existing paths.
- Write the configuration atomically.
- Never relocate large content automatically.
- Keep a log entry describing imported paths.

**Step 3: Document behavior**

Explain defaults, USB authorization, manual covers, missing-device behavior, PKG destination semantics and recovery from a corrupted configuration.

**Step 4: Verify and commit**

```powershell
python uwp/test_xps3_overlay.py
git add uwp/app/uwp_storage.cpp uwp/app/uwp_storage_platform.cpp docs/STORAGE.md README.md uwp/test_xps3_overlay.py
git commit -m "docs: document configurable Xbox storage"
```

---

## Task 8: Integrate version 0.3.5.0 and collect Xbox evidence

**Files:**

- Modify: `uwp/Package.appxmanifest`
- Modify: `uwp/app/uwp_frontend.cpp`
- Modify: `CHANGELOG.md`
- Modify: `docs/STATUS.md`
- Modify: `docs/XBOX_TEST_MATRIX.md`
- Modify: `.github/workflows/xps3-uwp-release.yml`
- Modify: `uwp/test_xps3_overlay.py`

**Step 1: Bump version truthfully**

Set package and visible version to `0.3.5.0`. Changelog entries must distinguish implemented, compiled and hardware-tested status.

**Step 2: Run local checks**

```powershell
python uwp/test_xps3_overlay.py
git diff --check
```

Expected: PASS.

**Step 3: Push and build**

Push to `main`, tag `v0.3.5.0` only after source checks pass, and monitor `XPS3 UWP release`.

**Step 4: Validate release assets**

Require:

- `XPS3_0.3.5.0_x64.appx`;
- `Microsoft.UI.Xaml.2.8.appx`;
- development certificate;
- checksums;
- `INSTALL.md`.

Verify manifest version/architecture, dependency identity, package signature presence, signer/certificate match, file hashes and required static content.

**Step 5: Xbox test protocol**

Ask the hardware tester to record:

- app launches as `Game` with 5120 MiB budget;
- internal default profile works;
- USB folder can be selected and survives restart;
- removal/reinsertion produces correct missing/ready state;
- a manual PNG/JPG cover appears from the selected cover directory;
- library rescan detects legitimate ELF/ISO/folder content;
- PKG destination page reports real free space and can be cancelled safely;
- no install or copy is claimed successful without verifying output;
- logs export successfully.

Do not mark storage features `Tested on Xbox` until that evidence is received.

