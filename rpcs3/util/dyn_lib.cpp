#include "stdafx.h"
#include "dyn_lib.hpp"

#ifdef _WIN32
	#include <Windows.h>
#ifdef RPCS3_UWP
	#include <winternl.h>
	#include <cwctype>
#endif
#else
	#include <dlfcn.h>
#endif

namespace utils
{
	dynamic_library::dynamic_library(const std::string& path)
	{
		load(path);
	}

	dynamic_library::~dynamic_library()
	{
		close();
	}

	bool dynamic_library::load(const std::string& path)
	{
#ifdef _WIN32
		m_handle = LoadLibraryA(path.c_str());
#else
		m_handle = dlopen(path.c_str(), RTLD_LAZY);
#endif
		return loaded();
	}

#ifdef _WIN32
	bool dynamic_library::load(const std::wstring& path)
	{
		m_handle = LoadLibraryW(path.c_str());
		return loaded();
	}
#endif

	void dynamic_library::close()
	{
#ifdef _WIN32
		FreeLibrary(reinterpret_cast<HMODULE>(m_handle));
#else
		dlclose(m_handle);
#endif
		m_handle = nullptr;
	}

	void* dynamic_library::get_impl(const char* name) const
	{
#ifdef _WIN32
		return reinterpret_cast<void*>(GetProcAddress(reinterpret_cast<HMODULE>(m_handle), name));
#else
		return dlsym(m_handle, name);
#endif
	}

	bool dynamic_library::loaded() const
	{
		return m_handle != nullptr;
	}

	dynamic_library::operator bool() const
	{
		return loaded();
	}

#ifdef RPCS3_UWP
	// GetModuleHandle is not available to UWP apps. System DLLs such as ntdll.dll and
	// KernelBase.dll are always loaded, so find them in the loader's module list.
	static HMODULE find_loaded_module(const char* lib)
	{
		std::wstring wanted;
		for (const char* p = lib; *p; p++)
			wanted += static_cast<wchar_t>(std::towlower(static_cast<unsigned char>(*p)));

		const PEB* peb = NtCurrentTeb()->ProcessEnvironmentBlock;
		const LIST_ENTRY* head = &peb->Ldr->InMemoryOrderModuleList;
		for (const LIST_ENTRY* it = head->Flink; it != head; it = it->Flink)
		{
			const auto* entry = CONTAINING_RECORD(it, LDR_DATA_TABLE_ENTRY, InMemoryOrderLinks);
			const UNICODE_STRING& full = entry->FullDllName;
			std::wstring path(full.Buffer, full.Length / sizeof(wchar_t));
			for (auto& c : path)
				c = std::towlower(c);
			if (path.size() >= wanted.size() && path.compare(path.size() - wanted.size(), wanted.size(), wanted) == 0 &&
				(path.size() == wanted.size() || path[path.size() - wanted.size() - 1] == L'\\'))
			{
				return static_cast<HMODULE>(entry->DllBase);
			}
		}
		return nullptr;
	}
#endif

	void* get_proc_address(const char* lib, const char* name)
	{
#if defined(_WIN32) && defined(RPCS3_UWP)
		const HMODULE module = find_loaded_module(lib);
		return module ? reinterpret_cast<void*>(GetProcAddress(module, name)) : nullptr;
#elif defined(_WIN32)
		return reinterpret_cast<void*>(GetProcAddress(GetModuleHandleA(lib), name));
#else
		return dlsym(dlopen(lib, RTLD_NOLOAD), name);
#endif
	}
}
