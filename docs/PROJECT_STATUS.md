# XPS3 project status

Evidence date: 2026-10-27. Status is evidence-based; no percentages are used.

| Area | Status | Evidence |
|---|---|---|
| Xbox UWP feasibility probe | Tested on Xbox | JIT RW→X, generated-code execution/patch, D3D12, texture, controller and 5 GiB Game allocation passed on the user's Series S |
| RPCS3-Xbox 0.1.35.0 source integration | Compiled upstream / XPS3 build in progress | Full upstream commit merged; static integration tests pass |
| XPS3 identity and privacy boundary | Implemented | Own package identity and publisher; inherited telemetry/update endpoints excluded from the target |
| Cell Horizon frontend | Implemented in source | Biblioteca, instalação, configurações e menu rápido atualizados; XAML e testes estáticos passam; validação visual 0.3.3.0 no console pendente |
| PPU/SPU LLVM toolchain | Compiled in 0.3.2.0 / 0.3.3.0 pending | O pipeline 0.3.2.0 produziu um APPX validado; a versão 0.3.3.0 recompila o núcleo antes da publicação |
| Mesa Dozen / D3D12 package | Compiled in 0.3.2.0 / 0.3.3.0 pending | Dependências gráficas foram incluídas no APPX 0.3.2.0; a nova versão ainda precisa concluir o workflow |
| XPS3 gameplay | Not tested in 0.3.3.0 | No commercial game may be labelled playable until this package runs on the target console |
| Universal PS3 compatibility | Not achieved | Long-term goal; compatibility must be established title by title |

## Evidence boundary

The imported RPCS3-Xbox documentation contains results from that upstream project. Those results are useful engineering references but do not prove an XPS3 build. XPS3 results will be added only from identifiable builds and exported logs.

Firmware, games, keys and copyrighted cover art are not included in this repository or release.
