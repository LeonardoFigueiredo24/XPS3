// Xbox controller input for the UWP shell, through Windows.Gaming.Input.
//
// RPCS3's XInput pad handler does the button mapping, deadzones and rumble; on UWP it
// calls the XInput-shaped functions below instead of xinput1_x.dll (see uwp_input.h).
// Controllers keep the slot (XInput Pad #1-4) they got when they first appeared.

#include "stdafx.h"
#include "uwp_input.h"

#include "Emu/Io/pad_config.h"
#include "Emu/system_utils.hpp"
#include "Utilities/File.h"
#include "util/logs.hpp"

#include <inspectable.h>
#include <roapi.h>
#include <winstring.h>

#include <array>
#include <cmath>
#include <mutex>

LOG_CHANNEL(uwp_input_log, "UWPInput");

namespace
{
	// ABI declarations from the Windows SDK (winrt/windows.gaming.input.h, windows.ui.core.h).
	enum gamepad_buttons : UINT32
	{
		gp_menu = 0x1,
		gp_view = 0x2,
		gp_a = 0x4,
		gp_b = 0x8,
		gp_x = 0x10,
		gp_y = 0x20,
		gp_dpad_up = 0x40,
		gp_dpad_down = 0x80,
		gp_dpad_left = 0x100,
		gp_dpad_right = 0x200,
		gp_left_shoulder = 0x400,
		gp_right_shoulder = 0x800,
		gp_left_thumb = 0x1000,
		gp_right_thumb = 0x2000,
	};

	struct gamepad_reading
	{
		UINT64 timestamp;
		UINT32 buttons;
		double left_trigger;
		double right_trigger;
		double left_x;
		double left_y;
		double right_x;
		double right_y;
	};

	struct gamepad_vibration
	{
		double left_motor;
		double right_motor;
		double left_trigger;
		double right_trigger;
	};

	struct IGamepad : IInspectable
	{
		virtual HRESULT STDMETHODCALLTYPE get_Vibration(gamepad_vibration* value) = 0;
		virtual HRESULT STDMETHODCALLTYPE put_Vibration(gamepad_vibration value) = 0;
		virtual HRESULT STDMETHODCALLTYPE GetCurrentReading(gamepad_reading* value) = 0;
	};

	// Windows.Foundation.Collections.IVectorView<Gamepad*>
	struct IGamepadVectorView : IInspectable
	{
		virtual HRESULT STDMETHODCALLTYPE GetAt(UINT32 index, IGamepad** item) = 0;
		virtual HRESULT STDMETHODCALLTYPE get_Size(UINT32* size) = 0;
	};

	struct IGamepadStatics : IInspectable
	{
		virtual HRESULT STDMETHODCALLTYPE add_GamepadAdded(IUnknown*, INT64*) = 0;
		virtual HRESULT STDMETHODCALLTYPE remove_GamepadAdded(INT64) = 0;
		virtual HRESULT STDMETHODCALLTYPE add_GamepadRemoved(IUnknown*, INT64*) = 0;
		virtual HRESULT STDMETHODCALLTYPE remove_GamepadRemoved(INT64) = 0;
		virtual HRESULT STDMETHODCALLTYPE get_Gamepads(IGamepadVectorView** value) = 0;
	};

	struct IBackRequestedEventArgs : IInspectable
	{
		virtual HRESULT STDMETHODCALLTYPE get_Handled(boolean* value) = 0;
		virtual HRESULT STDMETHODCALLTYPE put_Handled(boolean value) = 0;
	};

	// Windows.Foundation.EventHandler<BackRequestedEventArgs*>
	struct IBackRequestedHandler : IUnknown
	{
		virtual HRESULT STDMETHODCALLTYPE Invoke(IInspectable* sender, IBackRequestedEventArgs* args) = 0;
	};

	struct ISystemNavigationManager : IInspectable
	{
		virtual HRESULT STDMETHODCALLTYPE add_BackRequested(IBackRequestedHandler* handler, INT64* token) = 0;
		virtual HRESULT STDMETHODCALLTYPE remove_BackRequested(INT64 token) = 0;
	};

	struct ISystemNavigationManagerStatics : IInspectable
	{
		virtual HRESULT STDMETHODCALLTYPE GetForCurrentView(ISystemNavigationManager** value) = 0;
	};

	// {8bbce529-d49c-39e9-9560-e47dde96b7c8}
	constexpr GUID IID_IGamepadStatics{0x8bbce529, 0xd49c, 0x39e9, {0x95, 0x60, 0xe4, 0x7d, 0xde, 0x96, 0xb7, 0xc8}};
	// {dc52b5ce-bee0-4305-8c54-68228ed683b5}
	constexpr GUID IID_ISystemNavigationManagerStatics{0xdc52b5ce, 0xbee0, 0x4305, {0x8c, 0x54, 0x68, 0x22, 0x8e, 0xd6, 0x83, 0xb5}};
	// {ca821060-002b-526d-8122-982630d7cdbe}
	constexpr GUID IID_IBackRequestedHandler{0xca821060, 0x002b, 0x526d, {0x81, 0x22, 0x98, 0x26, 0x30, 0xd7, 0xcd, 0xbe}};

	template <typename T>
	HRESULT get_factory(const wchar_t* class_name, const GUID& iid, T** out)
	{
		HSTRING_HEADER header;
		HSTRING name;
		if (const HRESULT hr = WindowsCreateStringReference(class_name, static_cast<UINT32>(wcslen(class_name)), &header, &name); FAILED(hr))
			return hr;
		return RoGetActivationFactory(name, iid, reinterpret_cast<void**>(out));
	}

	IUnknown* identity(IGamepad* pad)
	{
		IUnknown* unk = nullptr;
		pad->QueryInterface(__uuidof(IUnknown), reinterpret_cast<void**>(&unk));
		if (unk)
			unk->Release(); // The pad keeps it alive; only the pointer value is used.
		return unk;
	}

	struct gamepads
	{
		std::mutex mutex;
		IGamepadStatics* statics = nullptr;
		bool init_done = false;
		std::array<IGamepad*, XUSER_MAX_COUNT> slots{};
		std::array<DWORD, XUSER_MAX_COUNT> packet{};
		std::array<UINT64, XUSER_MAX_COUNT> last_timestamp{};
		u64 last_refresh = 0;

		// Matches the controller list to the slots, at most every 250 ms. Caller holds the mutex.
		void refresh()
		{
			if (!init_done)
			{
				init_done = true;
				// The pad thread may not be in an apartment yet; joining the MTA is harmless if it is.
				RoInitialize(RO_INIT_MULTITHREADED);
				if (const HRESULT hr = get_factory(L"Windows.Gaming.Input.Gamepad", IID_IGamepadStatics, &statics); FAILED(hr))
				{
					uwp_input_log.error("Gamepad statics unavailable: 0x%08x", static_cast<u32>(hr));
					statics = nullptr;
				}
				else
				{
					uwp_input_log.notice("Windows.Gaming.Input ready");
				}
			}

			if (!statics)
				return;

			const u64 now = GetTickCount64();
			if (last_refresh && now - last_refresh < 250)
				return;
			last_refresh = now;

			IGamepadVectorView* list = nullptr;
			if (FAILED(statics->get_Gamepads(&list)) || !list)
				return;

			UINT32 count = 0;
			list->get_Size(&count);

			std::array<IGamepad*, 8> current{};
			const UINT32 n = std::min<UINT32>(count, static_cast<UINT32>(current.size()));
			for (UINT32 i = 0; i < n; i++)
				list->GetAt(i, &current[i]);
			list->Release();

			// Drop controllers that are gone.
			for (u32 s = 0; s < slots.size(); s++)
			{
				if (!slots[s])
					continue;

				bool found = false;
				for (UINT32 i = 0; i < n; i++)
					found = found || (current[i] && identity(current[i]) == identity(slots[s]));

				if (!found)
				{
					uwp_input_log.notice("Controller %u disconnected", s + 1);
					slots[s]->Release();
					slots[s] = nullptr;
				}
			}

			// Give new controllers the first free slot.
			for (UINT32 i = 0; i < n; i++)
			{
				if (!current[i])
					continue;

				bool known = false;
				for (IGamepad* slot : slots)
					known = known || (slot && identity(slot) == identity(current[i]));

				if (!known)
				{
					for (u32 s = 0; s < slots.size(); s++)
					{
						if (!slots[s])
						{
							slots[s] = current[i];
							current[i] = nullptr;
							uwp_input_log.notice("Controller %u connected", s + 1);
							break;
						}
					}
				}

				if (current[i])
					current[i]->Release();
			}
		}

		IGamepad* get(DWORD index)
		{
			if (index >= slots.size())
				return nullptr;
			refresh();
			return slots[index];
		}
	};

	gamepads& get_gamepads()
	{
		static gamepads s_pads;
		return s_pads;
	}

	SHORT to_thumb(double v)
	{
		return static_cast<SHORT>(std::clamp(v, -1.0, 1.0) * 32767.0);
	}

	BYTE to_trigger(double v)
	{
		return static_cast<BYTE>(std::lround(std::clamp(v, 0.0, 1.0) * 255.0));
	}

	struct back_handler final : IBackRequestedHandler
	{
		LONG refs = 1;

		HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** out) override
		{
			if (riid == __uuidof(IUnknown) || riid == IID_IBackRequestedHandler || riid == __uuidof(IAgileObject))
			{
				*out = static_cast<IBackRequestedHandler*>(this);
				AddRef();
				return S_OK;
			}
			*out = nullptr;
			return E_NOINTERFACE;
		}
		ULONG STDMETHODCALLTYPE AddRef() override { return InterlockedIncrement(&refs); }
		ULONG STDMETHODCALLTYPE Release() override
		{
			const LONG r = InterlockedDecrement(&refs);
			if (!r)
				delete this;
			return r;
		}
		HRESULT STDMETHODCALLTYPE Invoke(IInspectable*, IBackRequestedEventArgs* args) override
		{
			// B is the PS3's circle button; without this the console leaves the app.
			if (args)
				args->put_Handled(true);
			return S_OK;
		}
	};
}

DWORD WINAPI uwp_xinput_get_state(DWORD user_index, XINPUT_STATE* state)
{
	gamepads& pads = get_gamepads();
	std::lock_guard lock(pads.mutex);

	IGamepad* pad = pads.get(user_index);
	if (!pad || !state)
		return ERROR_DEVICE_NOT_CONNECTED;

	gamepad_reading r{};
	if (FAILED(pad->GetCurrentReading(&r)))
		return ERROR_DEVICE_NOT_CONNECTED;

	if (r.timestamp != pads.last_timestamp[user_index])
	{
		pads.last_timestamp[user_index] = r.timestamp;
		pads.packet[user_index]++;
	}

	XINPUT_GAMEPAD& g = state->Gamepad;
	g = {};
	const auto map = [&](UINT32 from, WORD to) { if (r.buttons & from) g.wButtons |= to; };
	map(gp_dpad_up, XINPUT_GAMEPAD_DPAD_UP);
	map(gp_dpad_down, XINPUT_GAMEPAD_DPAD_DOWN);
	map(gp_dpad_left, XINPUT_GAMEPAD_DPAD_LEFT);
	map(gp_dpad_right, XINPUT_GAMEPAD_DPAD_RIGHT);
	map(gp_menu, XINPUT_GAMEPAD_START);
	map(gp_view, XINPUT_GAMEPAD_BACK);
	map(gp_left_thumb, XINPUT_GAMEPAD_LEFT_THUMB);
	map(gp_right_thumb, XINPUT_GAMEPAD_RIGHT_THUMB);
	map(gp_left_shoulder, XINPUT_GAMEPAD_LEFT_SHOULDER);
	map(gp_right_shoulder, XINPUT_GAMEPAD_RIGHT_SHOULDER);
	map(gp_a, XINPUT_GAMEPAD_A);
	map(gp_b, XINPUT_GAMEPAD_B);
	map(gp_x, XINPUT_GAMEPAD_X);
	map(gp_y, XINPUT_GAMEPAD_Y);
	// The Xbox button belongs to the console; PS is View + Menu (the XInput handler's default combo).
	g.bLeftTrigger = to_trigger(r.left_trigger);
	g.bRightTrigger = to_trigger(r.right_trigger);
	g.sThumbLX = to_thumb(r.left_x);
	g.sThumbLY = to_thumb(r.left_y);
	g.sThumbRX = to_thumb(r.right_x);
	g.sThumbRY = to_thumb(r.right_y);
	state->dwPacketNumber = pads.packet[user_index];
	return ERROR_SUCCESS;
}

DWORD WINAPI uwp_xinput_set_state(DWORD user_index, XINPUT_VIBRATION* vibration)
{
	gamepads& pads = get_gamepads();
	std::lock_guard lock(pads.mutex);

	IGamepad* pad = pads.get(user_index);
	if (!pad || !vibration)
		return ERROR_DEVICE_NOT_CONNECTED;

	gamepad_vibration v{};
	v.left_motor = vibration->wLeftMotorSpeed / 65535.0;
	v.right_motor = vibration->wRightMotorSpeed / 65535.0;
	return SUCCEEDED(pad->put_Vibration(v)) ? ERROR_SUCCESS : ERROR_DEVICE_NOT_CONNECTED;
}

DWORD WINAPI uwp_xinput_get_battery_information(DWORD user_index, BYTE, XINPUT_BATTERY_INFORMATION* info)
{
	gamepads& pads = get_gamepads();
	std::lock_guard lock(pads.mutex);

	if (!info)
		return ERROR_BAD_ARGUMENTS;

	// Battery level isn't read yet: report connected pads as wired (full).
	const bool connected = pads.get(user_index) != nullptr;
	info->BatteryType = connected ? BATTERY_TYPE_WIRED : BATTERY_TYPE_DISCONNECTED;
	info->BatteryLevel = connected ? BATTERY_LEVEL_FULL : BATTERY_LEVEL_EMPTY;
	return connected ? ERROR_SUCCESS : ERROR_DEVICE_NOT_CONNECTED;
}

void uwp_input_attach_view()
{
	ISystemNavigationManagerStatics* statics = nullptr;
	if (const HRESULT hr = get_factory(L"Windows.UI.Core.SystemNavigationManager", IID_ISystemNavigationManagerStatics, &statics); FAILED(hr))
	{
		uwp_input_log.error("SystemNavigationManager unavailable: 0x%08x", static_cast<u32>(hr));
		return;
	}

	ISystemNavigationManager* nav = nullptr;
	if (const HRESULT hr = statics->GetForCurrentView(&nav); FAILED(hr) || !nav)
	{
		uwp_input_log.error("GetForCurrentView failed: 0x%08x", static_cast<u32>(hr));
		statics->Release();
		return;
	}

	auto* handler = new back_handler();
	INT64 token = 0;
	const HRESULT hr = nav->add_BackRequested(handler, &token);
	uwp_input_log.notice("Back button handler: 0x%08x", static_cast<u32>(hr));
	handler->Release();
	nav->Release();
	statics->Release();
}

void uwp_input_default_config()
{
	const std::string path = rpcs3::utils::get_input_config_dir() + g_cfg_input_configs.default_config + ".yml";
	// Keep a config that already maps controllers (an earlier run wrote it, or a user edited it).
	// Builds without controller support ran with every pad on Null; replace those.
	if (fs::file f(path); f && f.to_string().find("Handler: XInput") != umax)
		return;

	g_cfg_input.from_default();
	for (u32 i = 0; i < XUSER_MAX_COUNT; i++)
	{
		g_cfg_input.player[i]->handler.set(pad_handler::xinput);
		g_cfg_input.player[i]->device.from_string(fmt::format("XInput Pad #%u", i + 1));
		g_cfg_input.player[i]->buddy_device.from_string("");
	}
	g_cfg_input.save("", g_cfg_input_configs.default_config);
	uwp_input_log.notice("Wrote default input config with controllers 1-4: %s", path);
}
