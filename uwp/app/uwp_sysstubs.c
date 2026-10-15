/* Stand-ins for desktop-only Windows DLL functions that the core or its libraries import.
 * Xbox doesn't ship user32 MessageBox, winmm, dbghelp, pdh or the crypt32 system store
 * APIs, and one missing import stops the whole app from loading. Defining the functions
 * (and their __imp_ pointers) here keeps those DLLs out of the import table.
 * On x64 every stub can ignore its arguments; each returns that API's failure value. */

typedef unsigned long long u64;

#define UWP_STUB(name, value) \
	u64 name(void) { return (u64)(value); } \
	void* __imp_##name = (void*)&name;

/* user32 */
UWP_STUB(MessageBoxA, 0)
UWP_STUB(MonitorFromWindow, 0) // fullscreen-exclusive path in the Vulkan swapchain; no monitors on UWP
UWP_STUB(MessageBoxW, 0)
UWP_STUB(GetKeyNameTextW, 0)

/* winmm: no joysticks, no MIDI devices (MMSYSERR_NODRIVER / JOYERR_UNPLUGGED) */
UWP_STUB(joyGetNumDevs, 0)
UWP_STUB(joyGetDevCapsW, 6)
UWP_STUB(joyGetPosEx, 167)
UWP_STUB(midiInGetNumDevs, 0)
UWP_STUB(midiOutGetNumDevs, 0)
UWP_STUB(midiInGetDevCapsA, 6)
UWP_STUB(midiOutGetDevCapsA, 6)
UWP_STUB(midiInOpen, 6)
UWP_STUB(midiOutOpen, 6)
UWP_STUB(midiInClose, 6)
UWP_STUB(midiOutClose, 6)
UWP_STUB(midiInStart, 6)
UWP_STUB(midiInStop, 6)
UWP_STUB(midiInReset, 6)
UWP_STUB(midiInAddBuffer, 6)
UWP_STUB(midiInPrepareHeader, 6)
UWP_STUB(midiInUnprepareHeader, 6)
UWP_STUB(midiOutPrepareHeader, 6)
UWP_STUB(midiOutUnprepareHeader, 6)
UWP_STUB(midiOutShortMsg, 6)
UWP_STUB(midiOutLongMsg, 6)

/* dbghelp: no symbol lookup or stack walking */
UWP_STUB(SymInitialize, 0)
UWP_STUB(SymInitializeW, 0)
UWP_STUB(SymCleanup, 0)
UWP_STUB(SymSetOptions, 0)
UWP_STUB(SymFromAddr, 0)
UWP_STUB(SymFromAddrW, 0)
UWP_STUB(SymGetLineFromAddrW64, 0)
UWP_STUB(SymGetModuleBase64, 0)
UWP_STUB(SymFunctionTableAccess64, 0)
UWP_STUB(StackWalk64, 0)

/* pdh: performance counters unavailable (PDH_CSTATUS_NO_MACHINE) */
UWP_STUB(PdhOpenQueryW, 0x800007D0)
UWP_STUB(PdhAddEnglishCounterW, 0x800007D0)
UWP_STUB(PdhCollectQueryData, 0x800007D0)
UWP_STUB(PdhGetFormattedCounterArrayW, 0x800007D0)
UWP_STUB(PdhCloseQuery, 0)

/* crypt32: no system certificate store */
UWP_STUB(CertOpenSystemStoreA, 0)
UWP_STUB(CertEnumCertificatesInStore, 0)
UWP_STUB(CertCloseStore, 0)
