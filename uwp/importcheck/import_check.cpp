// Loads every DLL and function rpcs3_uwp.exe imports and writes which ones are missing to
// LocalState/import_check.txt. Run it on a device where the main app fails to start: the
// loader refuses a whole exe when one import is missing and doesn't say which.

#include "../app/winrt_min.h"
#include "import_table.h"

#include <cstdio>
#include <string>

extern "C" HMODULE WINAPI LoadLibraryW(LPCWSTR);

static std::wstring local_state_dir()
{
	using namespace winrt_min;
	IApplicationDataStatics* statics = nullptr;
	IApplicationData* data = nullptr;
	IInspectable* folder = nullptr;
	IStorageItem* item = nullptr;
	HSTRING path = nullptr;
	std::wstring result;

	if (SUCCEEDED(get_factory(L"Windows.Storage.ApplicationData", IID_IApplicationDataStatics, &statics)) &&
		SUCCEEDED(statics->get_Current(&data)) && SUCCEEDED(data->get_LocalFolder(&folder)) &&
		SUCCEEDED(folder->QueryInterface(IID_IStorageItem, reinterpret_cast<void**>(&item))) && SUCCEEDED(item->get_Path(&path)))
	{
		result = WindowsGetStringRawBuffer(path, nullptr);
		result += L'\\';
	}

	if (path) WindowsDeleteString(path);
	if (item) item->Release();
	if (folder) folder->Release();
	if (data) data->Release();
	if (statics) statics->Release();
	return result;
}

int WINAPI wWinMain(HINSTANCE, HINSTANCE, PWSTR, int)
{
	RoInitialize(RO_INIT_MULTITHREADED);
	const std::wstring path = local_state_dir() + L"import_check.txt";
	FILE* f = _wfopen(path.c_str(), L"w");
	if (!f)
		return 1;

	unsigned missing_dlls = 0, missing_funcs = 0;
	const wchar_t* last_dll = nullptr;
	HMODULE module = nullptr;

	for (const auto& imp : g_imports)
	{
		if (!last_dll || wcscmp(last_dll, imp.dll) != 0)
		{
			last_dll = imp.dll;
			module = LoadLibraryW(imp.dll);
			fprintf(f, "%ls: %s (error %lu)\n", imp.dll, module ? "loaded" : "MISSING", module ? 0ul : GetLastError());
			fflush(f);
			if (!module)
				missing_dlls++;
		}

		if (module && !GetProcAddress(module, imp.name))
		{
			fprintf(f, "  MISSING function %s\n", imp.name);
			missing_funcs++;
		}
	}

	fprintf(f, "done: %u missing DLLs, %u missing functions\n", missing_dlls, missing_funcs);
	fclose(f);
	return 0;
}
