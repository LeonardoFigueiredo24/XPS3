#pragma once

// Xbox controller input for the UWP shell. LoadLibrary is not available to UWP apps, so
// RPCS3's XInput pad handler gets these Windows.Gaming.Input backed functions instead of
// the ones from xinput1_x.dll. Signatures match XInputGetState/SetState/GetBatteryInformation.

#include <windows.h>
#include <xinput.h>

#include <string>

DWORD WINAPI uwp_xinput_get_state(DWORD user_index, XINPUT_STATE* state);
DWORD WINAPI uwp_xinput_set_state(DWORD user_index, XINPUT_VIBRATION* vibration);
DWORD WINAPI uwp_xinput_get_battery_information(DWORD user_index, BYTE dev_type, XINPUT_BATTERY_INFORMATION* info);

// Marks gamepad B / back as handled so the console doesn't leave the app. Call on the view thread.
void uwp_input_attach_view();

// Writes an input config with the first four controllers on players 1-4 if none exists yet.
void uwp_input_default_config();

// Scripted button presses on controller 1 for remote test runs: reads LocalState/autopress.
// Lines: "<seconds> <button>[+button] [hold ms]" or "repeat <from s> <to s> <every s> <button> [hold ms]",
// buttons cross circle square triangle start select up down left right l1 r1 l3 r3. Seconds count
// from when the game starts running. Controller 1 counts as connected while a script is loaded.
void uwp_input_load_script(const std::string& path);
