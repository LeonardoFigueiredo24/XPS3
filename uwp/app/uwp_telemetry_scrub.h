#pragma once

// Removes personal data from log lines before the telemetry upload (uwp_telemetry.cpp).
// Plain C++ without Windows or RPCS3 headers, so it can be tested on the build host
// (uwp/app/test/telemetry_scrub_test.cpp).

#include <string>
#include <string_view>
#include <vector>

namespace uwp_telemetry
{
	struct scrubber
	{
		// Exact strings that identify the user (RPCN name, password hash, token, XMB user names),
		// each replaced by its label wherever it appears. Strings shorter than 3 characters are ignored.
		struct known
		{
			std::string text, label;
		};
		std::vector<known> known_strings;

		// Returns the line with personal data replaced by <labels>:
		// - the known strings above
		// - user folder names in paths (C:\Users\<name>\..., also with /)
		// - email addresses
		// - IPv4 addresses (not 0.x, 127.x or 224+, which carry nothing personal and look like versions)
		// - IPv6 and MAC addresses
		// - values after password / passwd / token / secret / ticket / authorization = or :
		// - in RPCN and NP lines (other players' names): quoted text, (...) groups and the name after from / as
		std::string scrub(std::string_view line) const;
	};
}
