# Stress tests and benchmarks

Programs that stress the emulator outside of games, and the tools to run them on the Xbox and on a PC.

## What is in the package

`bin/test/stress/` ships in the app package as `test/stress/`:

- `bench/bench.elf`: our benchmark (source here). Every test prints `BENCH <name> <microseconds> us <checksum>`.
  The checksum must be the same on every build and decoder; a different one means wrong code.
- One folder per [ps3autotests](https://github.com/RPCS3/ps3autotests) program (commit b5cff06, GPLv2,
  see `ps3autotests-LICENSE.txt`): PPU integer/float/vector instructions, SPU instructions, lv2 sync,
  and RSX shader/texture tests. Their `.expected` output stays in that repository.

| bench test | what it measures |
|---|---|
| ppu_int, ppu_fp, ppu_vmx | PPU integer, FPU and VMX throughput (LLVM code quality) |
| ppu_calls | indirect calls through a table and small functions (PPU call/return cost) |
| ppu_branch | data-dependent branches |
| ppu_mem | 16 MB copy and strided reads (guest memory access) |
| ppu_atomic, ppu_atomic2 | lwarx/stwcx and ldarx/stdcx, alone and with two threads on one line |
| ppu_threads3 | three PPU threads of compute at once (do they run in parallel?) |
| lv2_pingpong | two PPU threads handing over a mutex + condition variable (thread wake-up latency) |
| lv2_syscall, lv2_yield | cost of a cheap syscall, and of sys_ppu_thread_yield |
| spu_fp, spu_int (x1, x6) | SPU float and integer throughput on one SPU and on six at once |
| spu_dma (x1, x6) | 16 KB MFC GET + PUT |
| spu_atomic (x1, x6) | GETLLAR/PUTLLC increments of one shared line (reservations, the SPURS hot path) |
| spu_pingpong | an SPU and a PPU thread taking turns on one line |

## Running on the Xbox (no menu needed)

Put a file named `autotest` in LocalState. Its first line is `bench` (the packaged programs, copied
to `LocalState/autotest_programs/` first) or a folder path; an optional second line is the time
limit per program in seconds (default 300). On start the app renames it to `autotest.done`, runs
each program until it exits, and writes `LocalState/autotest_results.txt`:

```
AUTOTEST <folder>: 28 programs, 300 s limit each, PPU Recompiler (LLVM), SPU Recompiler (LLVM)
RUN <path>
<TTY output, then output.txt if the program wrote one>
END <path> ok|timeout|failed|boot-failed <seconds>s
AUTOTEST done in <seconds>s
```

The decoder switches in LocalState (`spu_llvm`, `spu_interp`, ...) apply as for games, so the same
batch can be run per setting. Delete `autotest.done` and the results afterwards like other test files.

## Running on a PC

`rpcs3_bench_runner` is a Qt-less headless build of the same core (Null renderer, no input):

```
cmake -G Ninja <src> -DRPCS3_BENCH_RUNNER=ON -DCMAKE_C_COMPILER=clang-19 -DCMAKE_CXX_COMPILER=clang++-19 \
  -DUSE_NATIVE_INSTRUCTIONS=OFF -DUSE_SDL=OFF -DUSE_FAUDIO=OFF -DUSE_SYSTEM_OPENCV=OFF -DCMAKE_DISABLE_FIND_PACKAGE_Wayland=ON
ninja rpcs3_bench_runner
bin/rpcs3_bench_runner [--ppu llvm|interp] [--spu llvm|asmjit|interp] [--timeout s] prog.elf... > results.txt
```

It prints the same RUN/END format. It needs the PS3 firmware installed in its config folder
(`~/.config/rpcs3/dev_flash`) for the programs that load firmware modules; `bench.elf` does not.

## Reading results

```
report.py results.txt [--tests <ps3autotests checkout>] [--base older_results.txt]
```

lists each program with PASS/DIFF against its `.expected` output, and the BENCH times, with the
speed-up and checksum match against a base run.

## Rebuilding bench.elf

`./build.sh` needs only clang and ld.lld (any recent version): the PPU code is freestanding C with raw
lv2 syscalls, and the SPU kernels are assembled by `spu_asm.py` (no SPU toolchain exists any more).
Copy `build/bench.elf` to `bin/test/stress/bench/`.
