/* Stand-ins for desktop-only functions Mesa's Dozen driver imports (Win32 WSI over GDI,
 * the shader disk cache path and the DXIL.dll version query). The Xbox port presents
 * through its own swapchain and never uses those paths; defining them here keeps
 * user32, gdi32, shell32 and version.dll out of vulkan_dzn.dll's import table. */

typedef unsigned long long u64;

#define UWP_STUB(name, value) \
	u64 name(void) { return (u64)(value); } \
	void* __imp_##name = (void*)&name;

UWP_STUB(GetClientRect, 0)
UWP_STUB(GetDC, 0)
UWP_STUB(CreateCompatibleDC, 0)
UWP_STUB(CreateDIBSection, 0)
UWP_STUB(DeleteDC, 0)
UWP_STUB(DeleteObject, 0)
UWP_STUB(GetObjectA, 0)
UWP_STUB(SelectObject, 0)
UWP_STUB(StretchBlt, 0)
UWP_STUB(SHGetFolderPathA, 0x80004005) /* E_FAIL: no disk cache directory */
UWP_STUB(GetFileVersionInfoSizeW, 0)
UWP_STUB(GetFileVersionInfoW, 0)
UWP_STUB(VerQueryValueW, 0)
