#pragma once

// Controller-driven menu for the UWP shell: game list, PS3 firmware installer and a few
// settings, drawn in software into a BGRA canvas the shell shows while no game runs.

#include "util/types.hpp"

#include <functional>
#include <string>

struct IInspectable;

namespace uwp_frontend
{
	constexpr u32 canvas_width = 1920;
	constexpr u32 canvas_height = 1080;

	struct host_info
	{
		std::string local_dir;   // LocalState, with trailing slash
		std::string package_dir; // installed package folder, with trailing slash
		// Runs a function on the emulator ("main") thread, which owns Emu state changes.
		std::function<void(std::function<void()>)> run_on_main;
	};

	// Emulator thread, after Emu.Init() and the shell's fixed settings.
	void init(const host_info& host);

	// Emulator thread: boot a game folder, ISO or program the way the menu does
	// (picks built-in libraries when no firmware is installed). Returns false on failure.
	bool boot(const std::string& path);

	// UI thread, once per frame. Polls the gamepad (and keyboard through the CoreWindow).
	// Returns true while the menu is shown; then `pixels` is the canvas_width x canvas_height
	// BGRA canvas and `changed` says whether it differs from the previous frame.
	bool frame(IInspectable* core_window, const u32*& pixels, bool& changed);

	// UI thread, while a game boots without a frame to show: a loading screen with the
	// emulator's progress text (e.g. PPU modules compiling). Same canvas as frame().
	bool loading_frame(const std::string& title, const std::string& detail, u32 done, u32 total, const u32*& pixels);

	// Emulator callbacks.
	void on_emulation_started();
	void on_emulation_stopped();
}
