#pragma once

// Opt-in telemetry: with the user's consent (asked once, on the first start; Settings can turn it
// off) the app sends events and its logs to the developer's server, rpcs3.emelinhabibovic.de, when a
// game or the XMB starts or stops, after a fatal error and after a run that ended while a game ran,
// plus a performance report every 10 s while a game runs (sent once a minute).
// Logs go through uwp_telemetry_scrub first (no account names, passwords, tokens, emails, IPs or
// user folders). Reports carry a random anonymous ID made on the first start, nothing that names
// the user or console. The upload API is described in uwp/TELEMETRY_API.md.

#include <string>
#include <string_view>

namespace cfg
{
	class _bool;
}

namespace uwp_telemetry
{
	enum class consent
	{
		unknown, // not asked yet: the first start shows the question
		yes,
		no,
	};

	// Emulator thread, once the log file is set up (after RPCS3.log moved to RPCS3.prev.log):
	// loads or creates the anonymous ID, looks for a run that ended during a game, starts the upload thread.
	void init(const std::string& local_dir);

	// The anonymous install ID (UUID v4), empty before init().
	const std::string& install_id();
	consent get_consent();
	// The user's answer to the first-start question or the Settings switch. Yes sends the "install"
	// event (one per install, so the server can count consoles) and this start's events; No drops
	// whatever is queued and tells the server once that telemetry was turned off.
	void set_consent(bool yes);

	// The Settings switch's value (true = sending), kept in step with get_consent().
	cfg::_bool& setting();

	// Emulator callbacks (Emu.on_run / on_stop): a game, homebrew or the XMB started or stopped.
	void on_game_started();
	void on_game_stopped();
	// A fatal log line or report_fatal_error. Safe to call from the log listener: only queues work.
	void on_fatal(std::string_view text);

	// The first-start question.
	std::string consent_title();
	std::string consent_text();
}
