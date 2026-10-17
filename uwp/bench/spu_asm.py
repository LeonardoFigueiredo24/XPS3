#!/usr/bin/env python3
# Tiny SPU assembler for the bench kernels (no SPU toolchain exists any more).
# Encodings and opcodes follow rpcs3/Emu/Cell/SPUOpcodes.h. Writes a flat LS image.
#
# Kernel ABI (sys_spu_thread_argument, value in the high word = preferred slot):
#   r3 = kernel id, r4 = iterations, r5 = effective address, r6 = SPU index.
# Each kernel PUTs a 16-byte result to r5 + 0x100000 + index*16, then exits (stop 0x102).
import struct
import sys

RR = {'a': 0x0C0, 'sf': 0x040, 'and': 0x0C1, 'or': 0x041, 'xor': 0x241, 'fa': 0x2C4, 'fs': 0x2C5,
      'fm': 0x2C6, 'mpyu': 0x3CC, 'wrch': 0x10D, 'rdch': 0x00D, 'lqx': 0x1C4, 'stqx': 0x144,
      'nop': 0x201, 'lnop': 0x001, 'stop': 0x000}
RI7 = {'shli': 0x07B, 'rotmi': 0x079, 'roti': 0x078, 'rotqbyi': 0x1FC}
RI10 = {'ai': 0x1C, 'andi': 0x14, 'ori': 0x04, 'xori': 0x44, 'ceqi': 0x7C, 'cgti': 0x4C,
        'lqd': 0x34, 'stqd': 0x24}
RI16 = {'il': 0x081, 'ilhu': 0x082, 'iohl': 0x0C1}
BR16 = {'br': 0x064, 'brnz': 0x042, 'brz': 0x040}
RI18 = {'ila': 0x21}
RRR = {'fma': 0xE, 'mpya': 0xC, 'shufb': 0xB}


class Asm:
    def __init__(self):
        self.ops = []
        self.labels = {}

    def label(self, name):
        self.labels[name] = len(self.ops) * 4

    def __getattr__(self, name):
        name = name.rstrip('_')
        return lambda *args: self.ops.append((name, args))

    def li32(self, rt, value):
        value &= 0xFFFFFFFF
        self.ilhu(rt, value >> 16)
        self.iohl(rt, value & 0xFFFF)

    def assemble(self):
        out = bytearray()
        for pc, (name, a) in enumerate(self.ops):
            pc *= 4
            if name in RR:
                if name == 'stop':
                    w = (RR[name] << 21) | (a[0] & 0x3FFF)
                elif name in ('nop', 'lnop'):
                    w = RR[name] << 21
                elif name == 'wrch':  # wrch channel, rt
                    w = (RR[name] << 21) | (a[0] << 7) | a[1]
                elif name == 'rdch':  # rdch rt, channel
                    w = (RR[name] << 21) | (a[1] << 7) | a[0]
                else:  # op rt, ra, rb
                    w = (RR[name] << 21) | (a[2] << 14) | (a[1] << 7) | a[0]
            elif name in RI7:
                w = (RI7[name] << 21) | ((a[2] & 0x7F) << 14) | (a[1] << 7) | a[0]
            elif name in RI10:
                imm = a[2]
                if name in ('lqd', 'stqd'):
                    assert imm % 16 == 0
                    imm //= 16
                assert -512 <= imm < 512, (name, a)
                w = (RI10[name] << 24) | ((imm & 0x3FF) << 14) | (a[1] << 7) | a[0]
            elif name in RI16:
                w = (RI16[name] << 23) | ((a[1] & 0xFFFF) << 7) | a[0]
            elif name in BR16:
                rt, target = (0, a[0]) if name == 'br' else a
                off = (self.labels[target] - pc) // 4
                assert -32768 <= off < 32768
                w = (BR16[name] << 23) | ((off & 0xFFFF) << 7) | rt
            elif name in RI18:
                assert 0 <= a[1] < (1 << 18)
                w = (RI18[name] << 25) | (a[1] << 7) | a[0]
            elif name in RRR:  # op rt, ra, rb, rc
                w = (RRR[name] << 28) | (a[0] << 21) | (a[2] << 14) | (a[1] << 7) | a[3]
            else:
                raise ValueError(name)
            out += struct.pack('>I', w)
        return bytes(out)


MFC_LSA, MFC_EAH, MFC_EAL, MFC_Size, MFC_TagID, MFC_Cmd = 16, 17, 18, 19, 20, 21
MFC_WrTagMask, MFC_WrTagUpdate, MFC_RdTagStat, MFC_RdAtomicStat, SPU_WrOutMbox = 22, 23, 24, 27, 28
GET, PUT, GETLLAR, PUTLLC = 0x40, 0x20, 0xD0, 0xB4

LS_BUF, LS_LINE, LS_RESULT = 0x10000, 0x20000, 0x30000

s = Asm()
Z = 40  # always zero

# Constants used by the DMA helpers.
s.il(Z, 0)
s.il(41, 1)   # tag mask
s.il(42, 2)   # MFC_TAG_UPDATE_ALL
s.ila(43, LS_BUF)
s.ila(44, LS_LINE)
s.ila(45, LS_RESULT)
s.il(46, 0x4000)  # DMA size
s.il(47, 16)
s.il(21, 0)   # result


def dma(cmd_reg, lsa, ea, size):
    s.wrch(MFC_LSA, lsa)
    s.wrch(MFC_EAH, Z)
    s.wrch(MFC_EAL, ea)
    s.wrch(MFC_Size, size)
    s.wrch(MFC_TagID, Z)
    s.wrch(MFC_Cmd, cmd_reg)


def wait_tag():
    s.wrch(MFC_WrTagMask, 41)
    s.wrch(MFC_WrTagUpdate, 42)
    s.rdch(23, MFC_RdTagStat)


for kid, lab in enumerate(['k_fp', 'k_int', 'k_dma', 'k_atomic', 'k_pingpong']):
    s.ceqi(10, 3, kid)
    s.brnz(10, lab)
s.br('finish')

# --- k_fp: 8 independent FMA chains of 4 floats each ---
s.label('k_fp')
s.li32(12, 0x3A83126F)  # 0.001
for r in range(13, 21):
    s.li32(r, 0x3F800000 + (r << 12))
    s.li32(r + 9, 0x3F7FFF58 - (r << 4))  # ~0.99999, a different factor per chain (r22-r29)
s.label('fp_loop')
for r in range(13, 21):
    s.fma(r, r, r + 9, 12)
s.ai(4, 4, -1)
s.brnz(4, 'fp_loop')
for r in range(13, 21):
    s.xor(21, 21, r)
s.br('finish')

# --- k_int: xorshift + 16-bit multiplies + rotates ---
s.label('k_int')
s.li32(13, 0x9E3779B9)
s.li32(14, 0x12345678)
s.il(18, 0)
s.label('int_loop')
s.shli(15, 13, 13)
s.xor(13, 13, 15)
s.rotmi(15, 13, -17)
s.xor(13, 13, 15)
s.shli(15, 13, 5)
s.xor(13, 13, 15)
s.mpyu(16, 13, 14)
s.a(14, 14, 16)
s.a(14, 14, 13)
s.roti(17, 14, 7)
s.xor(18, 18, 17)
s.ai(4, 4, -1)
s.brnz(4, 'int_loop')
s.xor(21, 18, 14)
s.br('finish')

# --- k_dma: 16 KB GET then PUT, each SPU in its own 32 KB window at EA + index*32K ---
s.label('k_dma')
s.shli(30, 6, 15)
s.a(30, 30, 5)       # src EA
s.a(31, 30, 46)      # dst EA = src + 16 KB
s.il(32, GET)
s.il(33, PUT)
s.label('dma_loop')
dma(32, 43, 30, 46)
wait_tag()
dma(33, 43, 31, 46)
wait_tag()
s.ai(4, 4, -1)
s.brnz(4, 'dma_loop')
s.lqd(21, 43, 0)
s.br('finish')

# --- k_atomic: GETLLAR/PUTLLC increment of one shared 128-byte line at EA ---
s.label('k_atomic')
s.il(32, GETLLAR)
s.il(33, PUTLLC)
s.label('at_loop')
dma(32, 44, 5, 47)
s.rdch(23, MFC_RdAtomicStat)
s.lqd(15, 44, 0)
s.ai(15, 15, 1)
s.stqd(15, 44, 0)
dma(33, 44, 5, 47)
s.rdch(23, MFC_RdAtomicStat)
s.andi(22, 23, 1)
s.brnz(22, 'at_loop')  # reservation lost: retry
s.ai(4, 4, -1)
s.brnz(4, 'at_loop')
s.br('finish')

# --- k_pingpong: wait for the PPU to make word 0 odd, make it even again; r4 rounds ---
s.label('k_pingpong')
s.il(32, GETLLAR)
s.il(33, PUTLLC)
s.label('pp_loop')
dma(32, 44, 5, 47)
s.rdch(23, MFC_RdAtomicStat)
s.lqd(15, 44, 0)
s.andi(22, 15, 1)
s.brz(22, 'pp_loop')  # still even: PPU's turn
s.ai(15, 15, 1)
s.stqd(15, 44, 0)
dma(33, 44, 5, 47)
s.rdch(23, MFC_RdAtomicStat)
s.andi(22, 23, 1)
s.brnz(22, 'pp_loop')
s.ai(4, 4, -1)
s.brnz(4, 'pp_loop')
s.br('finish')

# --- finish: PUT the result quadword to EA + 0x100000 + index*16, exit(0) ---
s.label('finish')
s.stqd(21, 45, 0)
s.shli(30, 6, 4)
s.ilhu(31, 0x10)  # 0x100000
s.a(30, 30, 31)
s.a(30, 30, 5)
s.il(33, PUT)
dma(33, 45, 30, 47)
wait_tag()
s.wrch(SPU_WrOutMbox, Z)
s.stop(0x102)

code = s.assemble()
code += b'\0' * (-len(code) % 128)
open(sys.argv[1], 'wb').write(code)
