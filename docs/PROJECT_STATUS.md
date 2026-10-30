# XPS3 project status

Evidence date: 2026-10-30. Status is evidence-based; no percentages are used.

| Area | Status | Evidence |
|---|---|---|
| Xbox UWP feasibility probe | Tested on Xbox | JIT RW→X, generated-code execution/patch, D3D12, texture, controller and 5 GiB Game allocation passed on the user's Series S |
| RPCS3-Xbox source integration | 0.1.35.0 compiled; selected 0.1.35.11 safety change integrated in 0.3.4.0 source | Full upstream base is preserved; the PKG free-space preflight from 0.1.35.11 was ported without importing its always-on D3D12 diagnostic overhead |
| XPS3 identity and privacy boundary | Implemented | Own package identity and publisher; inherited telemetry/update endpoints excluded from the target |
| Cell Horizon frontend | Tested on Xbox for launch, navigation and quick-menu entry in 0.3.3.0 | User photos show Biblioteca, Instalar, Configurações and Menu rápido rendered on Series S while an emulation session was active |
| Storage manager 0.3.5.0 | Implemented in source; package/Xbox test pending | Persistent profiles, separate install/library/cache/log/cover paths, legacy migration and a controller-driven folder browser are connected; Xbox filesystem behavior still requires hardware evidence |
| Unified preparation 0.3.5.0 | Implemented in source; package/Xbox test pending | Content verification, real PPU/SPU/shader progress mapping and first-valid-frame gating are covered by Windows CI tests; performance impact is not yet measured on Xbox |
| PPU/SPU LLVM toolchain | Test ELF executed in 0.3.3.0 | `gs_gcm_basic_triangle.elf` passed the visible PPU compilation stage and rendered its expected triangle; `pad_test.elf` reached PPU compilation and produced a 204,629-byte cached PPU object, but its input result is not yet evidenced |
| Mesa Dozen / D3D12 package | Basic RSX workload rendered in 0.3.3.0 | `gs_gcm_basic_triangle.elf` produced a colored triangle on the Xbox display. This validates the isolated test path, not commercial-game rendering |
| Xbox controller in emulated PS3 software | In development | Frontend and quick-menu navigation work; a repeat of `pad_test.elf` reached a black screen. [RPCS3 issue #19095](https://github.com/RPCS3/rpcs3/issues/19095) documents a current upstream regression in that test, so a visually interactive ELF and Trace logs are required before classifying emulated pad input |
| XPS3 gameplay | Not tested | No commercial game may be labelled playable until an identifiable build runs it on the target console with logs |
| Universal PS3 compatibility | Not achieved | Long-term goal; compatibility must be established title by title |

## Evidence boundary

The imported RPCS3-Xbox documentation contains results from that upstream project. Those results are useful engineering references but do not prove an XPS3 build. XPS3 results will be added only from identifiable builds and exported logs.

On 2026-10-29, the user supplied Xbox Series S photos showing PPU compilation, the expected output of `gs_gcm_basic_triangle.elf`, the `pad_test.elf` compilation screen and the in-session quick menu. The USB export also contained a `pad_test.elf` PPU object cache and a Dozen pipeline cache. These artifacts prove that the test reached the integrated core; they do not prove that `pad_test.elf` completed or that all controller APIs work.

Firmware, games, keys and copyrighted cover art are not included in this repository or release.
