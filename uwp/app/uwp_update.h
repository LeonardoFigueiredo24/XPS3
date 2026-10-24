#pragma once

// The update check: once per app start, the app asks GitHub for the port's releases
// (github.com/emelin2004/rpcs3-xbox, pre-releases included) and compares the highest
// vX.Y.Z.W tag with its own package version. The menu shows the version in green when it is
// current and in orange, with a line naming the newer release, when it is outdated. Without an
// answer (offline, GitHub down) the version is shown without either. Not telemetry: nothing
// is sent but the request itself, whatever the user chose there.

#include <functional>
#include <string>

namespace uwp_update
{
	enum class status
	{
		unknown,  // not checked yet, or no answer
		current,
		outdated,
	};

	// This package's version (uwp/package/AppxManifest.xml), like "0.1.34.75".
	const std::string& app_version();

	// Starts the check on its own thread; on_done runs there once it has a result.
	void start(std::function<void()> on_done);

	status get_status();
	// The newest release's version (without the "v"), empty before an answer.
	std::string latest_version();
	// Where to get it.
	inline constexpr const char* releases_page = "github.com/emelin2004/rpcs3-xbox/releases";

	// Compares two dotted version numbers ("0.1.34.75"): <0, 0 or >0.
	int compare(const std::string& a, const std::string& b);
}
