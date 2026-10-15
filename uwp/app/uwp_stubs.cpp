// Symbols the core expects from the desktop frontend (Qt UI) or from libraries the
// UWP build leaves out.

#include "stdafx.h"
#include <functional>
#include <thread>

// Desktop RPCS3 pumps Qt events while waiting; the UWP shell has no Qt event loop.
void qt_events_aware_op(int repeat_duration_ms, std::function<bool()> wrapped_op)
{
	while (!wrapped_op())
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(repeat_duration_ms ? repeat_duration_ms : 1));
	}
}

extern "C"
{
	// The microphone code calls alcGetString for logging even when built without OpenAL.
	const char* alcGetString(void*, int)
	{
		return "";
	}

	// MSVC intrinsic used by some code paths; MinGW doesn't provide an out-of-line copy.
	void __movsb(unsigned char* dst, const unsigned char* src, size_t n)
	{
		__asm__ __volatile__("rep movsb" : "+D"(dst), "+S"(src), "+c"(n) : : "memory");
	}
}

// Input configuration globals live in the Qt frontend on desktop.
#include "Emu/Io/pad_config.h"
cfg_input_configurations g_cfg_input_configs;
std::string g_input_config_override;

// Mouse-driven gyro emulation needs Qt mouse events; the UWP shell never enables it.
#include "Input/mouse_gyro_handler.h"
void mouse_gyro_handler::set_enabled(bool enabled)
{
	m_enabled = enabled;
}

void mouse_gyro_handler::apply_gyro(const std::shared_ptr<Pad>&)
{
}
