# XPS3 project status

Evidence date: 2026-10-29. Status is evidence-based; no percentages are used.

| Area | Status | Evidence |
|---|---|---|
| Xbox UWP feasibility probe | Tested on Xbox | JIT RW→X, generated-code execution/patch, D3D12, texture, controller and 5 GiB Game allocation passed on the user's Series S |
| RPCS3-Xbox source integration | 0.1.35.0 compiled; selected 0.1.35.11 safety change integrated in 0.3.4.0 source | Full upstream base is preserved; the PKG free-space preflight from 0.1.35.11 was ported without importing its always-on D3D12 diagnostic overhead |
| XPS3 identity and privacy boundary | Implemented | Own package identity and publisher; inherited telemetry/update endpoints excluded from the target |
| Cell Horizon frontend | Tested on Xbox for launch and navigation in 0.3.3.0 | User photos show Biblioteca, Instalar and Configurações rendered on Series S and bundled ELF entries discovered; no ELF execution result was supplied yet |
| PPU/SPU LLVM toolchain | Compiled in 0.3.3.0; execution pending evidence | The signed 0.3.3.0 package was installed, but a bundled ELF has not yet been shown executing |
| Mesa Dozen / D3D12 package | Compiled in 0.3.3.0; game rendering pending evidence | The frontend rendered on Xbox; this is not evidence that an RSX workload or game rendered correctly |
| XPS3 gameplay | Not tested | No commercial game may be labelled playable until an identifiable build runs it on the target console with logs |
| Universal PS3 compatibility | Not achieved | Long-term goal; compatibility must be established title by title |

## Evidence boundary

The imported RPCS3-Xbox documentation contains results from that upstream project. Those results are useful engineering references but do not prove an XPS3 build. XPS3 results will be added only from identifiable builds and exported logs.

Firmware, games, keys and copyrighted cover art are not included in this repository or release.
