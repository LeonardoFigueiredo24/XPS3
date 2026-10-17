// Emulator stress/benchmark ELF for the RPCS3 Xbox port. Freestanding PPU code (no SDK):
// raw lv2 syscalls, built with clang (see build.sh). Each test prints one line:
//   BENCH <name> <microseconds> us <checksum>
// The checksum must match between runs and builds (wrong code = wrong checksum).
// SPU tests load the SPU programs embedded from spu_*.bin (see spu.S).

#include <stdint.h>
#include <stddef.h>

typedef uint64_t u64;
typedef uint32_t u32;
typedef int64_t s64;

static inline u64 sc6(u64 n, u64 a1, u64 a2, u64 a3, u64 a4, u64 a5, u64 a6)
{
	register u64 r11 __asm__("r11") = n;
	register u64 r3 __asm__("r3") = a1;
	register u64 r4 __asm__("r4") = a2;
	register u64 r5 __asm__("r5") = a3;
	register u64 r6 __asm__("r6") = a4;
	register u64 r7 __asm__("r7") = a5;
	register u64 r8 __asm__("r8") = a6;
	__asm__ volatile("sc"
		: "+r"(r3), "+r"(r4), "+r"(r5), "+r"(r6), "+r"(r7), "+r"(r8), "+r"(r11)
		:
		: "r0", "r9", "r10", "r12", "cr0", "ctr", "xer", "memory");
	return r3;
}

u64 bench_sc8(u64 n, u64 a1, u64 a2, u64 a3, u64 a4, u64 a5, u64 a6, u64 a7, u64 a8)
{
	register u64 r11 __asm__("r11") = n;
	register u64 r3 __asm__("r3") = a1;
	register u64 r4 __asm__("r4") = a2;
	register u64 r5 __asm__("r5") = a3;
	register u64 r6 __asm__("r6") = a4;
	register u64 r7 __asm__("r7") = a5;
	register u64 r8 __asm__("r8") = a6;
	register u64 r9 __asm__("r9") = a7;
	register u64 r10 __asm__("r10") = a8;
	__asm__ volatile("sc"
		: "+r"(r3), "+r"(r4), "+r"(r5), "+r"(r6), "+r"(r7), "+r"(r8), "+r"(r9), "+r"(r10), "+r"(r11)
		:
		: "r0", "r12", "cr0", "ctr", "xer", "memory");
	return r3;
}

#define SYS(...) SYS_(__VA_ARGS__, 0, 0, 0, 0, 0, 0)
#define SYS_(n, a, b, c, d, e, f, ...) sc6(n, (u64)(a), (u64)(b), (u64)(c), (u64)(d), (u64)(e), (u64)(f))

static inline u64 mftb(void)
{
	u64 tb;
	__asm__ volatile("mftb %0" : "=r"(tb));
	return tb;
}

static u64 g_tb_freq;

// ---- output ----

static void tty(const char* s, u32 len)
{
	static u32 written;
	sc6(403, 0, (u64)s, len, (u64)&written, 0, 0);
}

static u32 slen(const char* s)
{
	u32 n = 0;
	while (s[n]) n++;
	return n;
}

static char* put_str(char* p, const char* s)
{
	while (*s) *p++ = *s++;
	return p;
}

static char* put_dec(char* p, u64 v)
{
	char tmp[24];
	int n = 0;
	do { tmp[n++] = '0' + (char)(v % 10); v /= 10; } while (v);
	while (n) *p++ = tmp[--n];
	return p;
}

static char* put_hex(char* p, u64 v)
{
	for (int i = 60; i >= 0; i -= 4)
		*p++ = "0123456789abcdef"[(v >> i) & 15];
	return p;
}

static void print(const char* s)
{
	tty(s, slen(s));
}

static void report(const char* name, u64 t0, u64 t1, u64 sum)
{
	char buf[128];
	char* p = put_str(buf, "BENCH ");
	p = put_str(p, name);
	*p++ = ' ';
	p = put_dec(p, (t1 - t0) * 1000000 / g_tb_freq);
	p = put_str(p, " us ");
	p = put_hex(p, sum);
	*p++ = '\n';
	tty(buf, (u32)(p - buf));
}

// ---- lv2 wrappers ----

struct mutex_attr { u32 protocol, recursive, pshared, adaptive; u64 ipc_key; int32_t flags; u32 pad; char name[8]; };
struct cond_attr { u32 pshared; int32_t flags; u64 ipc_key; char name[8]; };
struct thread_param { u32 entry; u32 tls; };

extern char _thread_opd[];

static u32 mutex_create(void)
{
	static struct mutex_attr a;
	a.protocol = 2; // SYS_SYNC_PRIORITY
	a.recursive = 0x20; // not recursive
	a.pshared = 0x200;
	a.adaptive = 0x2000;
	u32 id = 0;
	SYS(100, (u64)&id, (u64)&a);
	return id;
}

static u32 cond_create(u32 mutex)
{
	static struct cond_attr a;
	a.pshared = 0x200;
	u32 id = 0;
	SYS(105, (u64)&id, mutex, (u64)&a);
	return id;
}

#define mutex_lock(m) SYS(102, m, 0)
#define mutex_unlock(m) SYS(104, m)
#define cond_wait(c) SYS(107, c, 0)
#define cond_signal(c) SYS(108, c)

static u64 thread_start(void* ctx)
{
	static struct thread_param p;
	p.entry = (u32)(u64)_thread_opd;
	p.tls = 0;
	u64 tid = 0;
	// _sys_ppu_thread_create(&tid, &param, arg, 0, prio, stacksize, flags=JOINABLE, name)
	bench_sc8(52, (u64)&tid, (u64)&p, (u64)ctx, 0, 1000, 0x10000, 1, (u64)"bench");
	SYS(53, (u32)tid);
	return tid;
}

static void thread_join(u64 tid)
{
	u64 v;
	SYS(44, (u32)tid, (u64)&v);
}

// ---- PPU tests ----

__attribute__((noinline)) static u64 t_int(u64 n)
{
	u64 x = 0x9E3779B97F4A7C15ull, y = 1, s = 0;
	for (u64 i = 0; i < n; i++)
	{
		x ^= x << 13; x ^= x >> 7; x ^= x << 17;
		y = y * 0x5851F42D4C957F2Dull + x;
		s += (y >> 29) ^ __builtin_rotateleft64(x, (u32)i & 63);
		s += (u32)x / ((u32)(y | 1) & 0xffff | 1);
	}
	return s;
}

__attribute__((noinline)) static u64 t_fp(u64 n)
{
	double a = 1.0, b = 0.5, c = 0.25, d = 0.125;
	float f = 1.0f;
	for (u64 i = 0; i < n; i++)
	{
		a = a * 0.999999 + 0.000001;
		b = b * 1.000001 - 0.0000005;
		c = __builtin_fma(c, 0.9999, d * 0.0001);
		d = d + (a - b) * 0.001;
		f = f * 0.99999f + 0.00001f;
		if (i % 64 == 0) d = d / (1.0 + c * c);
	}
	union { double d; u64 u; } r = { a + b + c + d + (double)f };
	return r.u;
}

typedef float v4f __attribute__((vector_size(16)));
typedef uint32_t v4u __attribute__((vector_size(16)));

__attribute__((noinline)) static u64 t_vmx(u64 n)
{
	v4f a = {1.0f, 2.0f, 3.0f, 4.0f}, b = {0.5f, 0.25f, 0.125f, 0.0625f}, c = {0.999f, 0.998f, 0.997f, 0.996f};
	v4u u = {1, 2, 3, 4}, k = {0x9E3779B9u, 0x85EBCA6Bu, 0xC2B2AE35u, 0x27D4EB2Fu};
	for (u64 i = 0; i < n; i++)
	{
		a = a * c + b;
		b = b * c + a * (v4f){0.001f, 0.001f, 0.001f, 0.001f};
		u = (u ^ (u >> 7)) + k;
		u = __builtin_shufflevector(u, u, 1, 2, 3, 0) ^ (u << 3);
	}
	union { v4f f; v4u u; } r = { a + b };
	return ((u64)(r.u[0] ^ r.u[2] ^ u[0] ^ u[2]) << 32) | (r.u[1] ^ r.u[3] ^ u[1] ^ u[3]);
}

// Interpreter-style dispatch: indirect calls through a table (ctr branches) and small functions.
typedef u64 (*op_fn)(u64, u64);
__attribute__((noinline)) static u64 op0(u64 a, u64 b) { return a + b; }
__attribute__((noinline)) static u64 op1(u64 a, u64 b) { return a ^ (b << 1); }
__attribute__((noinline)) static u64 op2(u64 a, u64 b) { return a * 3 + b; }
__attribute__((noinline)) static u64 op3(u64 a, u64 b) { return (a >> 3) | b; }
__attribute__((noinline)) static u64 op4(u64 a, u64 b) { return a - (b & 0xff); }
__attribute__((noinline)) static u64 op5(u64 a, u64 b) { return __builtin_rotateleft64(a, b & 63); }
__attribute__((noinline)) static u64 op6(u64 a, u64 b) { return a + (b >> 2); }
__attribute__((noinline)) static u64 op7(u64 a, u64 b) { return ~a + b; }
static op_fn volatile g_ops[8] = {op0, op1, op2, op3, op4, op5, op6, op7};

__attribute__((noinline)) static u64 t_calls(u64 n)
{
	u64 a = 1, b = 0x1234;
	for (u64 i = 0; i < n; i++)
	{
		a = g_ops[(a ^ i) & 7](a, b);
		b += a >> 5;
	}
	return a ^ b;
}

// Branchy code with data-dependent conditions (no indirect branches).
__attribute__((noinline)) static u64 t_branch(u64 n)
{
	u64 x = 12345, s = 0;
	for (u64 i = 0; i < n; i++)
	{
		x = x * 6364136223846793005ull + 1442695040888963407ull;
		u32 v = (u32)(x >> 33);
		if (v & 1) s += v; else s ^= v;
		if ((v & 6) == 2) s = s * 3;
		else if ((v & 6) == 4) s -= i;
		switch ((v >> 4) & 3)
		{
		case 0: s += 7; break;
		case 1: s ^= 0x55; break;
		case 2: s = __builtin_rotateleft64(s, 3); break;
		default: s >>= 1; break;
		}
	}
	return s;
}

// Memory: copy and checksum a 16 MB buffer (cache misses, big-endian loads/stores).
#define MEM_SIZE (16u << 20)
static u64 g_mem[2][MEM_SIZE / 8] __attribute__((aligned(128)));

__attribute__((noinline)) static u64 t_mem(u64 passes)
{
	u64 s = 0;
	for (u64 p = 0; p < passes; p++)
	{
		u64* src = g_mem[p & 1];
		u64* dst = g_mem[~p & 1];
		for (u32 i = 0; i < MEM_SIZE / 8; i++)
		{
			dst[i] = src[i] + i + p;
			s += dst[i] >> 3;
		}
		// Strided reads (one per 128-byte line).
		for (u32 i = 0; i < MEM_SIZE / 8; i += 16)
			s ^= src[(i * 977) & (MEM_SIZE / 8 - 1)];
	}
	return s;
}

// Atomics: lwarx/stwcx and ldarx/stdcx on one line, single thread.
__attribute__((noinline)) static u64 t_atomic(u64 n)
{
	static volatile u32 w __attribute__((aligned(128)));
	static volatile u64 d __attribute__((aligned(128)));
	for (u64 i = 0; i < n; i++)
	{
		__atomic_fetch_add(&w, 1, __ATOMIC_RELAXED);
		__atomic_fetch_xor(&d, i, __ATOMIC_RELAXED);
	}
	return w ^ d;
}

// Two threads hammering the same reservation granule.
struct atomic_ctx { volatile u32* p; u64 n; };
static volatile u32 g_shared[32] __attribute__((aligned(128)));

// Ping-pong between two PPU threads through an lv2 mutex + cond (host wakeup latency).
struct pp_ctx { u32 mutex, cond; volatile u32 turn; u64 n; };

static int g_mode;
static struct pp_ctx g_pp;
static struct atomic_ctx g_ac;

u64 bench_thread(u64 arg)
{
	if (g_mode == 1)
	{
		for (u64 i = 0; i < g_ac.n; i++)
			__atomic_fetch_add(g_ac.p, 1, __ATOMIC_RELAXED);
		return 0;
	}
	if (g_mode == 2)
	{
		for (u64 i = 0; i < g_pp.n; i++)
		{
			mutex_lock(g_pp.mutex);
			while (g_pp.turn != 1)
				cond_wait(g_pp.cond);
			g_pp.turn = 0;
			cond_signal(g_pp.cond);
			mutex_unlock(g_pp.mutex);
		}
		return 0;
	}
	if (g_mode == 3)
	{
		// Busy compute thread for the scaling test (arg = thread index).
		return t_int(g_ac.n) + arg;
	}
	return 0;
}

static u64 t_atomic2(u64 n)
{
	g_mode = 1;
	g_ac.p = &g_shared[0];
	g_ac.n = n;
	u64 tid = thread_start(0);
	for (u64 i = 0; i < n; i++)
		__atomic_fetch_add(&g_shared[0], 1, __ATOMIC_RELAXED);
	thread_join(tid);
	return g_shared[0];
}

static u64 t_pingpong(u64 n)
{
	g_mode = 2;
	g_pp.mutex = mutex_create();
	g_pp.cond = cond_create(g_pp.mutex);
	g_pp.turn = 0;
	g_pp.n = n;
	u64 tid = thread_start(0);
	for (u64 i = 0; i < n; i++)
	{
		mutex_lock(g_pp.mutex);
		while (g_pp.turn != 0)
			cond_wait(g_pp.cond);
		g_pp.turn = 1;
		cond_signal(g_pp.cond);
		mutex_unlock(g_pp.mutex);
	}
	thread_join(tid);
	return n;
}

static u64 t_syscall(u64 n)
{
	u64 s = 0;
	s64 sec, nsec;
	for (u64 i = 0; i < n; i++)
	{
		SYS(145, (u64)&sec, (u64)&nsec);
		s += 1;
	}
	return s;
}

static u64 t_yield(u64 n)
{
	for (u64 i = 0; i < n; i++)
		SYS(43);
	return n;
}

// sys_timer_usleep accuracy: N sleeps of a fixed length (the time per sleep shows the host's
// timer granularity; a PS3 wakes after ~the asked time).
static u64 g_usleep_us;
static u64 t_usleep(u64 n)
{
	for (u64 i = 0; i < n; i++)
		SYS(141, g_usleep_us);
	return n;
}

// Two PPU threads of pure compute next to the main one: tests that PPU threads run in parallel.
static u64 t_threads(u64 n)
{
	g_mode = 3;
	g_ac.n = n;
	u64 t1 = thread_start((void*)1);
	u64 t2 = thread_start((void*)2);
	u64 s = t_int(n);
	thread_join(t1);
	thread_join(t2);
	return s;
}

// ---- SPU tests (spu.c) ----
extern void spu_report_all(void (*rep)(const char*, u64, u64, u64), u64 (*tb)(void));

#define RUN(name, fn, n) do { u64 t0 = mftb(); u64 s = fn(n); u64 t1 = mftb(); report(name, t0, t1, s); } while (0)

int bench_main(void)
{
	g_tb_freq = SYS(147);
	if (!g_tb_freq)
		g_tb_freq = 79800000;

	print("BENCH start rpcs3-xbox-bench 1\n");

	RUN("ppu_int", t_int, 50000000);
	RUN("ppu_fp", t_fp, 50000000);
	RUN("ppu_vmx", t_vmx, 50000000);
	RUN("ppu_calls", t_calls, 30000000);
	RUN("ppu_branch", t_branch, 50000000);
	RUN("ppu_mem", t_mem, 8);
	RUN("ppu_atomic", t_atomic, 20000000);
	RUN("ppu_atomic2", t_atomic2, 5000000);
	RUN("ppu_threads3", t_threads, 30000000);
	RUN("lv2_pingpong", t_pingpong, 20000);
	RUN("lv2_syscall", t_syscall, 1000000);
	RUN("lv2_yield", t_yield, 200000);
	g_usleep_us = 100;
	RUN("lv2_usleep100", t_usleep, 2000);
	g_usleep_us = 1000;
	RUN("lv2_usleep1000", t_usleep, 500);

	spu_report_all(report, mftb);

	print("BENCH done\n");
	return 0;
}
