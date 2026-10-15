// Heap guard for the Xbox test builds: the console has no page heap or debugger, and an
// intermittent heap corruption shows up ~1.5 s after launch. Every operator new block gets a
// header and a trailing canary; operator delete checks both and, if either was overwritten,
// writes the details and a stack (exe-relative offsets) to LocalState/heap_guard.log, then
// fails fast so a crash dump is taken at the culprit's free instead of somewhere later.

#include <windows.h>
#include <cstdlib>
#include <cstring>
#include <cstdio>
#include <cstdint>
#include <new>

extern "C" IMAGE_DOS_HEADER __ImageBase;

namespace
{
	constexpr std::uint64_t c_magic_live = 0x4b4f4c4c414d4e57ull; // "WNMALLOK"
	constexpr std::uint64_t c_magic_dead = 0x44414544474e5557ull;
	constexpr unsigned char c_canary = 0xA5;
	constexpr std::size_t c_tail = 16;

	struct alignas(16) header
	{
		std::size_t size;
		void* caller;
		void* raw;       // what malloc returned
		std::uint32_t align; // 0 for plain new
		std::uint32_t pad;
		std::uint64_t magic;
		std::uint64_t magic_inv;
	};
	static_assert(sizeof(header) == 48);

	HANDLE s_log = INVALID_HANDLE_VALUE;

	void write_log(const char* text)
	{
		if (s_log != INVALID_HANDLE_VALUE)
		{
			DWORD written;
			WriteFile(s_log, text, static_cast<DWORD>(std::strlen(text)), &written, nullptr);
			FlushFileBuffers(s_log);
		}
		OutputDebugStringA(text);
	}

	std::uintptr_t rva(const void* p)
	{
		return reinterpret_cast<std::uintptr_t>(p) - reinterpret_cast<std::uintptr_t>(&__ImageBase);
	}

	[[noreturn]] void report(const char* what, header* h, void* user)
	{
		char buf[2048];
		int n = std::snprintf(buf, sizeof(buf), "heap guard: %s: block %p (user %p), size 0x%zx, allocated from exe+0x%llx, freed from exe+0x%llx\n",
			what, static_cast<void*>(h), user, h->size, static_cast<unsigned long long>(rva(h->caller)),
			static_cast<unsigned long long>(rva(__builtin_return_address(0))));

		// Bytes around the block: header and the start of the user data, then the tail.
		const auto* b = reinterpret_cast<const unsigned char*>(h);
		n += std::snprintf(buf + n, sizeof(buf) - n, "  head:");
		for (int i = 0; i < 64 && n < 1200; i++)
			n += std::snprintf(buf + n, sizeof(buf) - n, " %02x", b[i]);
		if (h->magic == c_magic_live && h->size < (1ull << 40))
		{
			const auto* t = static_cast<const unsigned char*>(user) + h->size;
			n += std::snprintf(buf + n, sizeof(buf) - n, "\n  tail:");
			for (int i = -16; i < 48 && n < 1900; i++)
				n += std::snprintf(buf + n, sizeof(buf) - n, " %02x", t[i]);
		}
		n += std::snprintf(buf + n, sizeof(buf) - n, "\n  stack:");
		void* frames[32];
		const USHORT count = RtlCaptureStackBackTrace(0, 32, frames, nullptr);
		for (USHORT i = 0; i < count && n < 2000; i++)
			n += std::snprintf(buf + n, sizeof(buf) - n, " %llx", static_cast<unsigned long long>(rva(frames[i])));
		std::snprintf(buf + n, sizeof(buf) - n, "\n");
		write_log(buf);
		__fastfail(50); // FAST_FAIL_HEAP_METADATA_CORRUPTION
	}

	void* guarded_alloc(std::size_t size, std::size_t align, void* caller)
	{
		const std::size_t a = align > 16 ? align : 16;
		void* raw = std::malloc(sizeof(header) + a + size + c_tail);
		if (!raw)
			return nullptr;
		const std::uintptr_t user_addr = (reinterpret_cast<std::uintptr_t>(raw) + sizeof(header) + a - 1) & ~(a - 1);
		auto* user = reinterpret_cast<unsigned char*>(user_addr);
		auto* h = reinterpret_cast<header*>(user) - 1;
		h->size = size;
		h->caller = caller;
		h->raw = raw;
		h->align = static_cast<std::uint32_t>(align); // 0 only for plain new (aligned new may ask for <= 16)
		h->pad = 0;
		h->magic = c_magic_live;
		h->magic_inv = ~c_magic_live;
		std::memset(user + size, c_canary, c_tail);
		return user;
	}

	void guarded_free(void* p, bool aligned)
	{
		if (!p)
			return;
		auto* h = static_cast<header*>(p) - 1;
		if (h->magic == c_magic_dead && h->magic_inv == ~c_magic_dead)
			report("double delete", h, p);
		if (h->magic != c_magic_live || h->magic_inv != ~c_magic_live)
			report("header overwritten (or memory not from operator new)", h, p);
		if (aligned != (h->align != 0))
			report(aligned ? "plain new freed by aligned delete" : "aligned new freed by plain delete", h, p);
		const auto* tail = static_cast<const unsigned char*>(p) + h->size;
		for (std::size_t i = 0; i < c_tail; i++)
		{
			if (tail[i] != c_canary)
				report("block overrun", h, p);
		}
		h->magic = c_magic_dead;
		h->magic_inv = ~c_magic_dead;
		std::free(h->raw);
	}

	void* alloc_or_throw(std::size_t size, std::size_t align, void* caller)
	{
		if (size == 0)
			size = 1;
		for (;;)
		{
			if (void* p = guarded_alloc(size, align, caller))
				return p;
			std::new_handler handler = std::get_new_handler();
			if (!handler)
				__fastfail(7); // FAST_FAIL_FATAL_APP_EXIT: out of memory, built without exceptions
			handler();
		}
	}
}

extern "C" void uwp_heap_guard_open_log(const wchar_t* path)
{
	s_log = CreateFile2(path, GENERIC_WRITE, FILE_SHARE_READ, CREATE_ALWAYS, nullptr);
}

#define RA __builtin_return_address(0)
void* operator new(std::size_t size) { return alloc_or_throw(size, 0, RA); }
void* operator new[](std::size_t size) { return alloc_or_throw(size, 0, RA); }
void* operator new(std::size_t size, const std::nothrow_t&) noexcept
{
	return guarded_alloc(size ? size : 1, 0, RA);
}
void* operator new[](std::size_t size, const std::nothrow_t&) noexcept
{
	return guarded_alloc(size ? size : 1, 0, RA);
}
void operator delete(void* p) noexcept { guarded_free(p, false); }
void operator delete[](void* p) noexcept { guarded_free(p, false); }
void operator delete(void* p, const std::nothrow_t&) noexcept { guarded_free(p, false); }
void operator delete[](void* p, const std::nothrow_t&) noexcept { guarded_free(p, false); }
void operator delete(void* p, std::size_t) noexcept { guarded_free(p, false); }
void operator delete[](void* p, std::size_t) noexcept { guarded_free(p, false); }

// Over-aligned allocations are guarded the same way; a mismatched new/delete pair is reported.
void* operator new(std::size_t size, std::align_val_t al) { return alloc_or_throw(size, static_cast<std::size_t>(al), RA); }
void* operator new[](std::size_t size, std::align_val_t al) { return alloc_or_throw(size, static_cast<std::size_t>(al), RA); }
void* operator new(std::size_t size, std::align_val_t al, const std::nothrow_t&) noexcept
{
	return guarded_alloc(size ? size : 1, static_cast<std::size_t>(al), RA);
}
void* operator new[](std::size_t size, std::align_val_t al, const std::nothrow_t&) noexcept
{
	return guarded_alloc(size ? size : 1, static_cast<std::size_t>(al), RA);
}
void operator delete(void* p, std::align_val_t) noexcept { guarded_free(p, true); }
void operator delete[](void* p, std::align_val_t) noexcept { guarded_free(p, true); }
void operator delete(void* p, std::align_val_t, const std::nothrow_t&) noexcept { guarded_free(p, true); }
void operator delete[](void* p, std::align_val_t, const std::nothrow_t&) noexcept { guarded_free(p, true); }
void operator delete(void* p, std::size_t, std::align_val_t) noexcept { guarded_free(p, true); }
void operator delete[](void* p, std::size_t, std::align_val_t) noexcept { guarded_free(p, true); }
