# XPS3 UWP cross-toolchain preflight — 2026-10-25

Result: **PASS**

The XPS3 Docker builder was created from `uwp/toolchain/Dockerfile.xps3-uwp` and the test in
`uwp/test/toolchain-preflight` was configured and compiled through the actual UWP cross-toolchain.
The output was an Intel amd64 COFF object, not a Linux object.

## Environment

- Docker Desktop client/server: `29.9.2` / `29.9.2`
- Docker engine allocation: 4 CPUs, 1,908,006,912 bytes RAM
- Builder image: `xps3-uwp-builder:2026-10-25`
- Builder image ID: `sha256:2034e2e975dbfb38c1e0a27b7427c47ac8e3892f73076107b99fbabbed3e8c01`
- Ubuntu base: `ubuntu:24.04`
- Zig: `0.17.0`
- Cross compiler reported by CMake: Clang `22.1.8`
- CMake: `4.4.4`
- Ninja: `1.13.2`
- Meson: `1.12.1`
- NASM: `2.16.01`
- Resource policy used by the test: 1,700 MiB, four CPUs, one build job

## Evidence excerpt

```text
case links: 30
-- The C compiler identification is Clang 22.1.8
-- The CXX compiler identification is Clang 22.1.8
-- Detecting C compiler ABI info - done
-- Detecting CXX compiler ABI info - done
[1/1] Building CXX object CMakeFiles/xps3_uwp_toolchain_preflight.dir/main.cpp.obj
main.cpp.obj: Intel amd64 COFF object file, not stripped, 12 sections
XPS3_UWP_TOOLCHAIN_PREFLIGHT=PASS
```

## Corrections proven during the test

- The repository now forces LF for Linux scripts and the extensionless Zig wrappers. A Windows
  checkout with CRLF previously made `/bin/sh` report valid wrappers as `not found`.
- Case-insensitive Windows-header aliases and generated import libraries now share one cache across
  CMake `TryCompile` directories. Before this change, each compiler probe repeated a multi-minute
  source scan through the Windows bind mount.
- Build wrappers accept `XPS3_JOBS`; the low-memory builder uses `XPS3_JOBS=1`.

This preflight validates the host-to-Windows cross compiler only. It is not evidence that the full
RPCS3-derived executable, Mesa Dozen, APPX package, PS3 software, or Xbox runtime has passed.
