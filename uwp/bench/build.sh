#!/bin/sh
# Builds bench.elf (freestanding PPU code + SPU kernels). Needs clang and ld.lld only.
set -e
cd "$(dirname "$0")"
OUT=${1:-build}
mkdir -p "$OUT"
python3 spu_asm.py "$OUT/spu_kernels.bin"
CFLAGS="--target=powerpc64-unknown-linux-gnu -mabi=elfv2 -mcpu=970 -maltivec -O2 -ffreestanding -fno-builtin -nostdlib -fno-stack-protector -fno-pic -mno-crbits"
clang $CFLAGS -c bench.c -o "$OUT/bench.o"
clang $CFLAGS -c spu.c -o "$OUT/spu.o"
clang $CFLAGS -c start.S -Wa,-I"$OUT" -I"$OUT" -o "$OUT/start.o"
ld.lld -m elf64ppc -T link.ld -static -o "$OUT/bench.elf" "$OUT/start.o" "$OUT/bench.o" "$OUT/spu.o"
echo "$OUT/bench.elf"
