# XPS3 0.3.5.0 game preparation pipeline design

Date: 2026-10-29

## Purpose

Replace ambiguous black screens and scattered compilation notices with one controller-friendly **Preparando jogo** flow. XPS3 prepares every CPU module and cached graphics resource that can be known safely before launch, then starts the selected title automatically. Resources that can only be discovered while the title executes remain handled by an explicit runtime policy rather than being falsely reported as precompiled.

This subsystem is independent from the 0.3.5.0 storage manager. It consumes the storage manager's selected cache root but owns compilation scheduling, progress reporting and launch gating.

## Evidence and constraints

- `gs_gcm_basic_triangle.elf` has been shown compiling PPU modules and rendering its expected triangle on the user's Xbox Series S.
- `pad_test.elf` reaches PPU compilation but then shows a black screen. [RPCS3 issue #19095](https://github.com/RPCS3/rpcs3/issues/19095) describes a current upstream regression in that same bundled test and requests `cellPad` and `sys_event` Trace logs. XPS3 must not treat this ELF as the sole controller acceptance test.
- Enabling full LLVM precompilation for every module in `dev_flash` has already been measured by the port at more than 36 minutes on a first Xbox boot. The normal launch path must not repeat that work for every title.
- An RSX title can generate or reveal shaders only when particular command streams, scenes and effects execute. First-run shader completeness cannot be guaranteed before emulation begins.
- The Dozen D3D12 pipeline library remains disabled by default because prior Xbox testing removed the D3D12 device when it was enabled. The pipeline must use the validated on-disk shader and pipeline caches rather than enabling that experimental path silently.

## User flow

Selecting **Jogar** opens a full-screen preparation view with the title name, cover, elapsed time, current phase, phase progress, overall progress and **Cancelar**. The phases are:

1. **Verificando conteúdo** — validate source path, active PS3 profile, firmware requirement, cache destination and available space.
2. **Analisando executáveis** — discover the title's executable and loadable PPU/SPU modules without recursively compiling unrelated firmware.
3. **Compilando módulos PPU** — compile missing title-specific PPU objects and reuse compatible objects already on disk.
4. **Preparando blocos SPU** — load and compile discoverable SPU cache entries using the selected policy.
5. **Carregando shaders conhecidos** — validate and load the title's existing shader cache.
6. **Preparando pipelines gráficos conhecidos** — compile or load only pipeline objects represented by compatible cached shader data.
7. **Iniciando jogo** — initialize input, audio and RSX, then reveal the game surface only after the core reports a valid running state or first frame.

The screen does not return to the library after a successful preparation. It starts the selected title automatically. Cancellation returns to the library without deleting valid cache entries already completed.

## Compilation modes

XPS3 exposes three per-title modes, with a global default:

### Equilibrado — default

Compile discoverable PPU/SPU modules and preload all compatible known graphics caches before launch. New shaders compile asynchronously during execution using the validated multi-threaded compiler. A small non-blocking notification may report background compilation. This provides the best default balance for Xbox Series S.

### Fluidez

Perform the same preparation as Equilibrado. When the running title encounters a shader or pipeline that is not ready, pause emulation at a safe frame boundary, show **Compilando novo shader**, wait for the job, then resume. This avoids presenting incomplete draws at the cost of visible first-run pauses. It must never hold an RPCS3 or D3D12 lock while the XAML UI waits.

### Inicialização rápida

Validate the title and load compatible existing caches, but do not perform the optional full scan of title modules. Missing CPU modules and graphics resources compile on demand. This is opt-in and may stutter more.

Changing the mode affects subsequent launches. The pause menu may display the active mode, but it may not switch compilation policy in the middle of a pending compiler job.

## What “prepare everything” means

The preparation contract is explicit:

- all title PPU modules discoverable without executing the title;
- discoverable SPU blocks supported by the existing RPCS3 precompiler;
- all compatible shaders and pipelines already recorded in that title's cache;
- required firmware modules known to be loaded by the title's boot path.

It does not claim to include shaders, SPU code, overlays or dynamically loaded modules that have never been observed or cannot be reached until the title executes. The UI says **Recursos conhecidos preparados** rather than **100% dos shaders do jogo**.

## Cache identity and storage

The storage manager supplies one cache root. Under it, the preparation pipeline retains isolated cache namespaces keyed by:

- title ID or stable content hash for homebrew;
- title/version identity;
- XPS3/RPCS3 cache ABI version;
- renderer and Dozen/D3D12 compatibility identity;
- CPU and relevant compilation settings.

Changing an incompatible key starts a new namespace. XPS3 does not delete the previous cache automatically. A settings action can display sizes and remove a selected title's cache after confirmation.

## Progress model

Every phase reports a state: **Aguardando**, **Em andamento**, **Concluído**, **Ignorado**, **Cancelado** or **Falhou**. Phases with known item counts report completed/total. Phases without a trustworthy total use an indeterminate bar and do not invent percentages.

Overall progress is based only on phases with known work and advances monotonically. The preparation view shows meaningful counters such as `PPU 4/12`, `SPU 18/63` or `Pipelines 220/481`. It never freezes the UI thread while polling compiler state.

## Launch gating and black-screen handling

The XAML game surface stays hidden behind the preparation view until one of these occurs:

- the renderer presents the first valid frame;
- the title explicitly runs without graphics and the core reports a stable running state;
- a configurable diagnostic timeout is reached.

If the title is running without presenting a frame, XPS3 shows **O programa está executando, mas ainda não apresentou imagem** with actions for **Continuar aguardando**, **Abrir diagnóstico** or **Encerrar**. It does not mislabel that state as shader compilation.

For test programs such as `pad_test.elf`, diagnostics record the last completed preparation phase, core state, presented-frame count, controller connection state and relevant log locations. A dedicated input acceptance test uses a visually interactive test such as `gs_gcm_tetris.elf` in addition to upstream `pad_test.elf`.

## Failure and cancellation

- Source removal, missing active storage or cache write failure stops before boot and names the affected path/profile.
- A compiler failure identifies the phase and module/resource when available and preserves the full RPCS3 log.
- Cancellation requests a cooperative stop; it does not terminate compiler threads or corrupt object files.
- Cache writes use existing atomic/temporary-file behavior. Incomplete cache files are rejected on the next launch.
- If a runtime shader job fails in Fluidez mode, XPS3 reports the error and lets the user end the title; it does not resume with a known invalid pipeline.

## Background and batch preparation

An action named **Preparar cache** for one selected title is a possible post-0.3.5.0 feature, not part of this release. Version 0.3.5.0 does not automatically batch-compile the entire library: on Xbox this could take many hours, consume substantial storage and compile resources for titles the user never launches. The normal entry point remains **Jogar**, which prepares the selected title and starts it.

## Settings and interface

Settings gains **Preparação de jogos** with:

- default mode: Equilibrado, Fluidez or Inicialização rápida;
- per-title override from the title options screen;
- show compiler details: on/off;
- cache location and usage, read from the storage manager;
- clear selected title cache;
- open/export compilation diagnostics.

All new user-facing strings are Portuguese-Brazilian first. Existing untranslated pause-menu labels are corrected as part of this UI work without changing emulator behavior.

## Test strategy

Static tests first assert the phase model, localized strings, compilation modes, cache identity and launch gate. Unit-level state-machine tests cover phase transitions, unknown totals, cancellation, errors, cache compatibility and the no-first-frame diagnostic state.

Host/integration tests cover:

1. cached and uncached basic triangle launches;
2. cache reuse on a second launch;
3. cancellation during PPU preparation;
4. incompatible cache identity isolation;
5. Equilibrado handling of a newly discovered shader;
6. Fluidez pause/compile/resume without blocking the UI thread;
7. a running program that presents no frame;
8. missing cache storage during preparation;
9. Portuguese labels in preparation and pause screens.

Xbox Series S evidence records cold and warm preparation time, each phase duration, cache size, first-frame time, memory peak, shader/pipeline counts, runtime pauses and exported logs. `gs_gcm_basic_triangle.elf` validates basic rendering; `gs_gcm_tetris.elf` is used as the next visual controller test; `pad_test.elf` remains diagnostic until the upstream regression is understood or corrected.

## Release scope

Version 0.3.5.0 includes the unified preparation view, per-title modes, known-cache preload, truthful first-frame gating, diagnostics and Portuguese localization. Predicting every never-seen shader, recompiling the entire firmware on every boot, automatic full-library batch compilation and claiming stutter-free behavior for all games are explicitly out of scope.
