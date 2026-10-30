# XPS3 0.3.5.0 Game Preparation Pipeline Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use `superpowers:executing-plans` to implement this plan task by task. Use `superpowers:test-driven-development` for each feature or bug fix and `superpowers:verification-before-completion` before claiming success.

**Goal:** Present one truthful preparation flow before gameplay that verifies content, reports actual PPU/SPU/shader work, keeps the game hidden until its first valid frame, and returns an actionable diagnostic instead of an unexplained black screen.

**Architecture:** Add a small preparation state machine between the library and existing RPCS3 boot path. Pre-boot checks run before `Emu.BootGame`; compile/cache phases observe existing RPCS3 progress counters and messages during boot; XAML remains on a full-screen preparation overlay until the render callback confirms the first valid frame. No fake precompilation or fabricated percentage is allowed.

**Tech Stack:** C++20, RPCS3 system progress globals, C++/WinRT, WinUI/XAML, Direct3D 12 first-frame signal, CMake/MSBuild, Python static integration tests.

**Specification:** `docs/superpowers/specs/2026-10-29-game-preparation-pipeline-design.md`

## Global constraints

- Do not run the firmware-wide `ppu_precompile` path as part of every game launch; existing measurements show it can take tens of minutes and is not title preparation.
- Do not report shaders as compiled before RPCS3 actually discovers/compiles them.
- Do not promise stutter-free play, 120 FPS or compatibility with a title from preparation success alone.
- Keep the selected game hidden until a render callback confirms a valid frame.
- Treat cancellation as cooperative and safe. Never terminate worker threads or abandon partially written caches.
- Keep diagnostic logs even when boot fails or produces no frame.

---

## Task 1: Add a pure preparation state machine

**Files:**

- Create: `uwp/app/uwp_preparation.h`
- Create: `uwp/app/uwp_preparation.cpp`
- Modify: `uwp/app/CMakeLists.txt`
- Modify: `uwp/test_xps3_overlay.py`

**Step 1: Write failing checks**

Require:

- explicit phases for verify, analyse, PPU, SPU, shaders/pipelines, start, first frame and terminal state;
- determinate or indeterminate progress per phase;
- cancellation and timeout state;
- no hard-coded fake percentage increments.

**Step 2: Define the API**

```cpp
namespace xps3::preparation
{
enum class phase
{
    verify_content,
    analyse_executables,
    compile_ppu,
    prepare_spu,
    load_shader_cache,
    prepare_pipelines,
    start_emulation,
    wait_first_frame,
    complete,
    failed,
    cancelled
};

enum class phase_state { pending, active, complete, failed, skipped, cancelled };
enum class mode { quick, balanced, smoothness };

struct phase_progress
{
    phase id = phase::verify_content;
    phase_state state = phase_state::pending;
    u64 done = 0;
    u64 total = 0;
    bool indeterminate = true;
    std::string label;
    std::string detail;
};

struct request
{
    std::string title;
    std::string title_id;
    std::string content_path;
    mode preparation_mode = mode::balanced;
};

struct snapshot
{
    u64 serial = 0;
    bool active = false;
    request game;
    std::vector<phase_progress> phases;
    phase current = phase::verify_content;
    u64 elapsed_ms = 0;
    bool can_cancel = false;
    std::string error_code;
    std::string error_message;
};

class controller
{
public:
    void begin(request value);
    void cancel();
    void activate(phase value, std::string detail = {});
    void update(phase value, u64 done, u64 total, std::string detail = {});
    void complete(phase value, std::string detail = {});
    void skip(phase value, std::string reason);
    void fail(std::string code, std::string message);
    void first_frame_presented();
    snapshot get_snapshot() const;
};
}
```

Make transitions monotonic and thread-safe. Reject updates from stale launch serials.

**Step 3: Implement transition rules**

- A phase can move pending → active → complete/failed/cancelled.
- Skipped is allowed only with an explicit reason.
- Percentage exists only when `total > 0` and `done <= total`.
- Terminal states ignore later progress except diagnostic logging.
- First frame may complete only after start-emulation is complete.

**Step 4: Register, test and commit**

```powershell
python uwp/test_xps3_overlay.py
git add uwp/app/uwp_preparation.* uwp/app/CMakeLists.txt uwp/test_xps3_overlay.py
git commit -m "feat: add game preparation state machine"
```

---

## Task 2: Add real pre-boot verification and content analysis

**Files:**

- Modify: `uwp/app/uwp_preparation.h`
- Modify: `uwp/app/uwp_preparation.cpp`
- Modify: `uwp/app/uwp_frontend.cpp`
- Modify: `uwp/test_xps3_overlay.py`

**Step 1: Add failing checks**

Require content-specific validation for ELF, ISO and extracted folder, and require explicit errors for missing/unreadable/unsupported/encrypted content.

**Step 2: Add verifier results**

Add:

```cpp
enum class content_kind { elf, iso, extracted_game, installed_title, package, unsupported };
struct verification_result
{
    bool ok = false;
    content_kind kind = content_kind::unsupported;
    u64 readable_bytes = 0;
    std::string title_id;
    std::vector<std::string> executable_candidates;
    std::string error_code;
    std::string message;
};
verification_result verify_content(std::string_view path);
```

**Step 3: Implement bounded verification**

- Confirm existence and readability.
- Read only necessary headers/metadata.
- Detect extracted `PS3_GAME/USRDIR/EBOOT.BIN`.
- Detect ELF magic and supported machine metadata.
- Use the existing ISO reader for filesystem metadata; do not decrypt protected images.
- Recognize `.pkg` as installation input, not direct boot input.
- Gather executable candidates for diagnostic display.

**Step 4: Integrate before boot**

In `start_game_entry`, begin preparation, run verify/analyse on the frontend worker and call the main-thread boot callback only after success. Failure returns to the library with a diagnostic dialog and retained log.

**Step 5: Verify and commit**

```powershell
python uwp/test_xps3_overlay.py
git add uwp/app/uwp_preparation.* uwp/app/uwp_frontend.cpp uwp/test_xps3_overlay.py
git commit -m "feat: verify PS3 content before boot"
```

---

## Task 3: Map RPCS3 compilation progress into preparation phases

**Files:**

- Modify: `uwp/app/uwp_main.cpp`
- Modify: `uwp/app/uwp_frontend.h`
- Modify: `uwp/app/uwp_frontend.cpp`
- Modify: `uwp/test_xps3_overlay.py`

**Step 1: Add failing checks**

Require use of existing `system_progress.hpp` counters/messages, stale-serial protection and indeterminate fallback.

**Step 2: Add progress bridge callbacks**

Extend `host_info` with callbacks/events that report:

- preparation started;
- system progress message plus file/program totals;
- emulation boot accepted;
- boot failure;
- first frame.

Do not expose mutable controller internals across threads.

**Step 3: Classify actual RPCS3 messages**

Map existing observed progress strings/counters to phases:

- PPU module analysis/compilation → `compile_ppu`;
- SPU cache/compilation → `prepare_spu`;
- shader cache load/compile → `load_shader_cache`;
- pipeline/program creation → `prepare_pipelines`;
- unknown work → current phase with original message and indeterminate progress.

Do not invent completion for a phase that produced no event. Mark it `skipped` with `Nenhum trabalho necessário nesta inicialização` only after RPCS3 advances beyond that stage.

**Step 4: Keep firmware precompile separate**

Leave the existing firmware-wide `ppu_precompile` diagnostic path behind its explicit developer action. Never invoke it from ordinary game launch.

**Step 5: Verify and commit**

```powershell
python uwp/test_xps3_overlay.py
git add uwp/app/uwp_main.cpp uwp/app/uwp_frontend.* uwp/test_xps3_overlay.py
git commit -m "feat: report real RPCS3 preparation progress"
```

---

## Task 4: Replace the current loading view with one preparation screen

**Files:**

- Modify: `uwp/app/uwp_xaml.cpp`
- Modify: `uwp/app/xaml/shell.xaml`
- Modify: `uwp/app/uwp_frontend.h`
- Modify: `uwp/app/uwp_frontend.cpp`
- Modify: `uwp/test_xps3_overlay.py`

**Step 1: Add failing UI checks**

Require:

- selected game name and content type;
- vertical phase list with pending/active/complete/error state;
- current real counter or indeterminate animation;
- elapsed time;
- cancel action;
- no gameplay surface exposure before first frame.

**Step 2: Extend the frontend snapshot**

Add an immutable preparation view to the main snapshot. Convert enum state to localized display text in one place.

**Step 3: Build the full-screen XAML layer**

Use Cell Horizon visual language and TV-safe sizing. Show one preparation screen for analysis, PPU, SPU and shader/pipeline work instead of separate ambiguous screens. Preserve the original RPCS3 message in a smaller details line.

**Step 4: Implement safe cancellation**

- Before `Emu.BootGame`, cancellation stops queued work.
- During RPCS3 boot, request normal stop through the existing emulator lifecycle.
- Show `Cancelando com segurança…` until workers settle.
- Return to library only after the boot attempt is stopped.

**Step 5: Verify and commit**

```powershell
python uwp/test_xps3_overlay.py
git add uwp/app/uwp_xaml.cpp uwp/app/xaml/shell.xaml uwp/app/uwp_frontend.* uwp/test_xps3_overlay.py
git commit -m "feat: add unified Xbox game preparation screen"
```

---

## Task 5: Gate gameplay on the first valid frame

**Files:**

- Modify: `uwp/app/uwp_main.cpp`
- Modify: `uwp/app/uwp_xaml.cpp`
- Modify: `uwp/app/uwp_frontend.cpp`
- Modify: `uwp/test_xps3_overlay.py`

**Step 1: Add failing checks**

Require explicit first-frame callback, boot timeout diagnostics and no transition triggered solely by `Emu.BootGame` returning success.

**Step 2: Connect the render signal**

Use the existing D3D12/RSX frame-present or valid-frame callback to notify the preparation controller once per launch serial. Ignore stale callbacks from a previous boot.

**Step 3: Add a diagnostic timeout**

After RPCS3 enters running state:

- keep waiting for the first frame;
- after a conservative diagnostic threshold, show `O núcleo iniciou, mas nenhum quadro válido foi apresentado`;
- offer `Continuar aguardando`, `Exportar diagnóstico` and `Encerrar`;
- do not kill emulation automatically.

**Step 4: Handle pad-test evidence honestly**

The previous `pad_test.elf` black screen becomes a no-frame diagnostic if it produces no render output. Do not label it a graphics failure unless logs prove that cause.

**Step 5: Verify and commit**

```powershell
python uwp/test_xps3_overlay.py
git add uwp/app/uwp_main.cpp uwp/app/uwp_xaml.cpp uwp/app/uwp_frontend.cpp uwp/test_xps3_overlay.py
git commit -m "fix: wait for a valid PS3 frame before gameplay"
```

---

## Task 6: Add preparation modes and per-title cache identity

**Files:**

- Modify: `uwp/app/uwp_preparation.h`
- Modify: `uwp/app/uwp_preparation.cpp`
- Modify: `uwp/app/uwp_frontend.h`
- Modify: `uwp/app/uwp_frontend.cpp`
- Modify: `uwp/app/uwp_xaml.cpp`
- Modify: `uwp/app/xaml/shell.xaml`
- Modify: `uwp/test_xps3_overlay.py`

**Step 1: Add failing checks**

Require modes `Rápido`, `Equilibrado` and `Fluidez`, with accurate explanations and no 120-FPS promise.

**Step 2: Define mode semantics**

- `Rápido`: use existing caches and allow normal asynchronous shader compilation.
- `Equilibrado`: default; validate caches and use RPCS3 defaults.
- `Fluidez`: prefer synchronous shader/pipeline work discovered during startup, accepting longer startup; it cannot precompile shaders for paths never exercised by the game.

Apply mode before boot. Do not mutate shader compilation policy mid-frame.

**Step 3: Persist per-title selection**

Store mode keyed by title ID or stable content hash. Use atomic persistence and fall back to `Equilibrado` on malformed data.

**Step 4: Key cache evidence correctly**

Record title ID, executable/hash, RPCS3-derived cache identity and preparation mode in logs. Never delete a cache automatically after a version mismatch; quarantine or ask the user.

**Step 5: Verify and commit**

```powershell
python uwp/test_xps3_overlay.py
git add uwp/app/uwp_preparation.* uwp/app/uwp_frontend.* uwp/app/uwp_xaml.cpp uwp/app/xaml/shell.xaml uwp/test_xps3_overlay.py
git commit -m "feat: add truthful game preparation modes"
```

---

## Task 7: Add preparation diagnostics and export

**Files:**

- Modify: `uwp/app/uwp_telemetry.h`
- Modify: `uwp/app/uwp_telemetry.cpp`
- Modify: `uwp/app/uwp_frontend.cpp`
- Modify: `docs/DIAGNOSTICS.md`
- Modify: `uwp/test_xps3_overlay.py`

**Step 1: Add failing checks**

Require exported records for phase timestamps, real counters/messages, content kind, mode, boot result, first-frame result and failure code, without private full paths by default.

**Step 2: Emit structured events**

Record one event per state transition plus bounded periodic progress samples. Scrub usernames and removable-volume labels unless the user explicitly chooses a verbose diagnostic.

**Step 3: Include no-frame evidence**

Export:

- elapsed time after running state;
- latest PPU/SPU/RSX message;
- whether D3D12 presented host UI frames;
- whether RSX produced a guest frame;
- memory budget/usage;
- last fatal emulator error.

**Step 4: Verify and commit**

```powershell
python uwp/test_xps3_overlay.py
git add uwp/app/uwp_telemetry.* uwp/app/uwp_frontend.cpp docs/DIAGNOSTICS.md uwp/test_xps3_overlay.py
git commit -m "feat: export game preparation diagnostics"
```

---

## Task 8: Integrate with the 0.3.5.0 release and Xbox test matrix

**Files:**

- Modify: `CHANGELOG.md`
- Modify: `docs/STATUS.md`
- Modify: `docs/XBOX_TEST_MATRIX.md`
- Modify: `uwp/test_xps3_overlay.py`

**Step 1: Update truthful status**

Record each phase as implemented/compiled/tested separately. Do not call the pipeline functional on Xbox until hardware evidence is received.

**Step 2: Run source verification**

```powershell
python uwp/test_xps3_overlay.py
git diff --check
```

**Step 3: Build and validate the APPX**

Use the same release validation described in the storage plan. The version is released once, after both plans are integrated.

**Step 4: Xbox test protocol**

Test in this order:

1. `gs_gcm_basic_triangle.elf`: phase screen, compilation evidence, first frame, triangle.
2. `pad_test.elf`: phase screen, input response or explicit no-frame diagnostic.
3. A small legal homebrew sample with audio/input.
4. Only then a legally obtained commercial title already known to boot in the Xbox RPCS3 fork.

For every run record build, console model, app type, memory limit, preparation phases/times, boot result, first frame, FPS when meaningful, errors, log and photo/video evidence.

Do not begin with The Last of Us or God of War as acceptance tests; they are complex titles and cannot establish basic platform correctness.

