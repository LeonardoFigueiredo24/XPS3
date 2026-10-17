// SPU side of the bench: runs the kernels from spu_asm.py on 1..6 SPU threads.

#include <stdint.h>

typedef uint64_t u64;
typedef uint32_t u32;

u64 bench_sc8(u64 n, u64 a1, u64 a2, u64 a3, u64 a4, u64 a5, u64 a6, u64 a7, u64 a8);

extern const char spu_image[], spu_image_end[];

struct spu_segment { int32_t type; u32 ls, size, addr; };
struct spu_image { u32 type, entry_point, segs; int32_t nsegs; };
struct group_attr { u32 nsize, name; int32_t type; };
struct thread_attr { u32 name, name_len, option; };
struct thread_arg { u64 arg1, arg2, arg3, arg4; };

// Line, DMA windows (6 x 32 KB) and results (+1 MB).
static char g_area[0x110000] __attribute__((aligned(128)));

static u64 sys(u64 n, u64 a1, u64 a2, u64 a3, u64 a4, u64 a5, u64 a6)
{
	return bench_sc8(n, a1, a2, a3, a4, a5, a6, 0, 0);
}

static u32 g_group;

static int spu_start(u32 kernel, u32 nspu, u64 iters)
{
	static struct spu_segment seg;
	static struct spu_image img;
	static struct group_attr ga;
	static struct thread_attr ta;
	static struct thread_arg arg;
	static int inited;

	if (!inited)
	{
		sys(169, 6, 0, 0, 0, 0, 0); // sys_spu_initialize(6, 0)
		inited = 1;
	}

	seg.type = 1; // copy
	seg.ls = 0;
	seg.size = (u32)(spu_image_end - spu_image);
	seg.addr = (u32)(u64)spu_image;
	img.type = 0; // user
	img.entry_point = 0;
	img.segs = (u32)(u64)&seg;
	img.nsegs = 1;

	ga.nsize = 6;
	ga.name = (u32)(u64)"bench";
	ga.type = 0;
	u32 group = 0;
	if (sys(170, (u64)&group, nspu, 100, (u64)&ga, 0, 0))
		return -1;

	for (u32 i = 0; i < nspu; i++)
	{
		ta.name = (u32)(u64)"bench";
		ta.name_len = 6;
		ta.option = 0;
		arg.arg1 = (u64)kernel << 32;
		arg.arg2 = iters << 32;
		arg.arg3 = (u64)(u32)(u64)g_area << 32;
		arg.arg4 = (u64)i << 32;
		u32 tid = 0;
		if (sys(172, (u64)&tid, group, i, (u64)&img, (u64)&ta, (u64)&arg))
			return -2;
	}

	if (sys(173, group, 0, 0, 0, 0, 0))
		return -3;
	g_group = group;
	return 0;
}

static u64 spu_join(u32 nspu)
{
	u32 cause = 0, status = 0;
	sys(178, g_group, (u64)&cause, (u64)&status, 0, 0, 0);
	sys(171, g_group, 0, 0, 0, 0, 0); // sys_spu_thread_group_destroy
	u64 s = 0;
	for (u32 i = 0; i < nspu; i++)
	{
		const u32* r = (const u32*)(g_area + 0x100000 + i * 16);
		s = (s << 7 | s >> 57) ^ r[0] ^ ((u64)r[1] << 32);
	}
	return s;
}

static u64 run(u32 kernel, u32 nspu, u64 iters, u64* sum)
{
	for (u32 i = 0; i < 0x100000 / 8; i++)
		((u64*)g_area)[i] = i * 0x9E3779B97F4A7C15ull;
	if (spu_start(kernel, nspu, iters))
		return 0;

	if (kernel == 4)
	{
		// Ping-pong with the SPU: make word 0 odd when it is even.
		volatile u32* w = (volatile u32*)g_area;
		*w = 0;
		for (u64 i = 0; i < iters; i++)
		{
			while (*w & 1)
				;
			__atomic_fetch_add(w, 1, __ATOMIC_RELAXED);
		}
	}

	*sum = spu_join(nspu);
	if (kernel == 3 || kernel == 4)
		*sum = *(volatile u32*)g_area;
	return 1;
}

void spu_report_all(void (*rep)(const char*, u64, u64, u64), u64 (*tb)(void))
{
	static const struct { const char* name; u32 kernel, nspu; u64 iters; } tests[] =
	{
		{"spu_fp", 0, 1, 50000000},
		{"spu_fp_x6", 0, 6, 50000000},
		{"spu_int", 1, 1, 50000000},
		{"spu_int_x6", 1, 6, 50000000},
		{"spu_dma", 2, 1, 100000},
		{"spu_dma_x6", 2, 6, 100000},
		{"spu_atomic", 3, 1, 2000000},
		{"spu_atomic_x6", 3, 6, 300000},
		{"spu_pingpong", 4, 1, 200000},
	};

	for (u32 i = 0; i < sizeof(tests) / sizeof(tests[0]); i++)
	{
		u64 sum = 0;
		u64 t0 = tb();
		u64 ok = run(tests[i].kernel, tests[i].nspu, tests[i].iters, &sum);
		u64 t1 = tb();
		rep(tests[i].name, t0, ok ? t1 : t0, ok ? sum : 0xdeadull);
	}
}
