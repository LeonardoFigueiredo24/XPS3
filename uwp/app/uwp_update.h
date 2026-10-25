#pragma once

// XPS3 deliberately disables the inherited automatic network update check. The API remains so
// the RPCS3-derived frontend can report the package version without contacting a third party.

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
	inline constexpr const char* releases_page = "update service not configured";

	// Compares two dotted version numbers ("0.1.34.75"): <0, 0 or >0.
	int compare(const std::string& a, const std::string& b);
}
