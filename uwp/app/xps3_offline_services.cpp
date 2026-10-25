// XPS3 privacy boundary for the imported RPCS3 UWP frontend.
//
// The source fork contains opt-in telemetry and an automatic GitHub release check tied to the
// fork author's infrastructure. XPS3 does not inherit either service. These implementations keep
// the existing frontend ABI intact while making every telemetry callback and update request a
// deterministic offline no-op.

#include "stdafx.h"
#include "uwp_telemetry.h"
#include "uwp_update.h"

#include "Emu/system_config.h"

namespace uwp_telemetry
{
	namespace
	{
		struct telemetry_cfg : cfg::node
		{
			cfg::_bool enabled{this, "Send logs to the developer", false};
		};

		telemetry_cfg g_cfg_tel;
		const std::string g_empty;
	}

	void init(const std::string&) {}
	const std::string& install_id() { return g_empty; }
	consent get_consent() { return consent::no; }
	void set_consent(bool) { g_cfg_tel.enabled.from_string("false"); }
	cfg::_bool& setting() { return g_cfg_tel.enabled; }
	void on_game_started() {}
	void on_game_stopped() {}
	void on_suspend() {}
	void on_resume() {}
	void on_fatal(std::string_view) {}
	std::string_view tls_roots() { return {}; }
	std::string consent_title() { return "Telemetry disabled"; }
	std::string consent_text() { return "XPS3 does not send logs or diagnostics over the network."; }
}

namespace uwp_update
{
	namespace
	{
		const std::string g_app_version = XPS3_APP_VERSION;
	}

	const std::string& app_version() { return g_app_version; }
	void start(std::function<void()>) {}
	status get_status() { return status::unknown; }
	std::string latest_version() { return {}; }

	int compare(const std::string& a, const std::string& b)
	{
		usz i = 0, j = 0;
		while (i < a.size() || j < b.size())
		{
			u64 x = 0, y = 0;
			for (; i < a.size() && a[i] != '.'; i++)
				x = x * 10 + (a[i] - '0');
			for (; j < b.size() && b[j] != '.'; j++)
				y = y * 10 + (b[j] - '0');
			if (x != y)
				return x < y ? -1 : 1;
			i++;
			j++;
		}
		return 0;
	}
}
