/* Lookup half of uwp_lazyimports.c: find a function in a DLL the process already loaded. */
#include <windows.h>
#include <winternl.h>

/* GetModuleHandle isn't available to UWP apps, so walk the loader's module list. */
static HMODULE lazy_find_module(const wchar_t* dll)
{
	const size_t want = wcslen(dll);
	const PEB* peb = NtCurrentTeb()->ProcessEnvironmentBlock;
	const LIST_ENTRY* head = &peb->Ldr->InMemoryOrderModuleList;
	for (const LIST_ENTRY* it = head->Flink; it != head; it = it->Flink)
	{
		const LDR_DATA_TABLE_ENTRY* e = CONTAINING_RECORD(it, LDR_DATA_TABLE_ENTRY, InMemoryOrderLinks);
		const size_t len = e->FullDllName.Length / sizeof(wchar_t);
		const wchar_t* path = e->FullDllName.Buffer;
		if (len >= want && _wcsicmp(path + len - want, dll) == 0 && (len == want || path[len - want - 1] == L'\\'))
			return (HMODULE)e->DllBase;
	}
	return NULL;
}

void* uwp_lazy_lookup(const wchar_t* dll, const char* name)
{
	HMODULE m = lazy_find_module(dll);
	void* fn = m ? (void*)GetProcAddress(m, name) : NULL;
	if (!fn)
	{
		/* Most kernel32/advapi32 functions live in kernelbase.dll on current Windows. */
		m = lazy_find_module(L"kernelbase.dll");
		fn = m ? (void*)GetProcAddress(m, name) : NULL;
	}
	return fn;
}
