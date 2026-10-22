#pragma once

// The system on-screen keyboard (the Xbox keyboard) for the menu's text fields.
// The menu has no XAML text box, so this is a Windows.UI.Text.Core edit context that the
// keyboard types into, shown with InputPane.TryShow. A USB keyboard types into it too.
// All functions run on the CoreWindow thread (uwp_frontend::frame()); the edit context's
// events arrive there during CoreDispatcher::ProcessEvents.

#include "util/types.hpp"

#include <functional>
#include <string>

namespace uwp_textinput
{
	// Picks the keyboard layout (CoreTextInputScope).
	enum class scope
	{
		username,
		password,
		email,
		url,
		code, // letters and digits, e.g. an emailed token
	};

	// Starts editing `text` (UTF-8) and asks the system to show its keyboard.
	// `accept` filters typed characters (UTF-16 code units); `transform` may change one (e.g. to
	// upper case). Returns false when the edit context or the keyboard isn't available.
	bool show(scope sc, const std::string& text, usz max_len, std::function<bool(char16_t)> accept, std::function<char16_t(char16_t)> transform);

	// Replaces the text being edited (after the menu's own keys changed it).
	void set_text(const std::string& text);

	// Stops editing and hides the keyboard.
	void hide();

	// The keyboard is (or was just asked to be) on screen; the menu ignores the gamepad meanwhile.
	bool keyboard_visible();

	// The text after the user's edits, once per change. `submitted`: Enter was typed.
	bool poll(std::string& text, bool& submitted);
}
