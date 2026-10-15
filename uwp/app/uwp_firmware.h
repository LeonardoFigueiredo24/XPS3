#pragma once

#include "util/atomic.hpp"

#include <string>

namespace uwp_firmware
{
	struct progress
	{
		atomic_t<u32> done{0};
		atomic_t<u32> total{0};
		std::string version; // set once the PUP has been validated
	};

	// Install a PS3UPDAT.PUP into dev_flash. Blocks; run it on a worker thread while no
	// game is running, then call Emu.Init() on the main thread to unmount.
	// Returns an empty string on success, else a short message for the user.
	std::string install(const std::string& path, progress& prog);
}
