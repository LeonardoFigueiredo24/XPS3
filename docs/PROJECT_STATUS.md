# XPS3 project status

Evidence date: 2026-10-25. Status is evidence-based; no percentages are used.

| Area | Status | Evidence |
|---|---|---|
| Xbox UWP feasibility probe | Tested on Xbox | JIT RW→X, generated-code execution/patch, D3D12, texture, controller and 5 GiB Game allocation passed on the user's Series S |
| RPCS3-Xbox 0.1.35.0 source integration | Compiled upstream / XPS3 build in progress | Full upstream commit merged; static integration tests pass |
| XPS3 identity and privacy boundary | Implemented | Own package identity and publisher; inherited telemetry/update endpoints excluded from the target |
| Cell Horizon frontend | Implemented in source | XAML bindings and static structure tests pass; target-console visual validation pending |
| PPU/SPU LLVM toolchain | In development | Reproducible Docker cross-build active; installable 0.3.2.0 package not produced yet |
| Mesa Dozen / D3D12 package | Not yet compiled for 0.3.2.0 | Release pipeline is prepared and runs after LLVM |
| XPS3 gameplay | Not tested | No commercial game may be labelled playable until the 0.3.2.0 package runs on the target console |
| Universal PS3 compatibility | Not achieved | Long-term goal; compatibility must be established title by title |

## Evidence boundary

The imported RPCS3-Xbox documentation contains results from that upstream project. Those results are useful engineering references but do not prove the XPS3 0.3.2.0 build. XPS3 results will be added only from identifiable builds and exported logs.

Firmware, games, keys and copyrighted cover art are not included in this repository or release.
