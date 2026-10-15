// RPCS3 UWP shell for Xbox Series X|S Dev Mode.
//
// Milestone 1: no UI and no graphics. On launch it sets up RPCS3's folders in the
// app's LocalState, boots the first program found in LocalState/boot/ (or the bundled
// test ELF) with the null renderer, and writes RPCS3.log next to it.
// The screen shows the state: blue = booting/running, green = program exited cleanly,
// red = boot failed or fatal error.

#include "stdafx.h"
#include "winrt_min.h"

#include "Emu/System.h"
#include "Emu/system_config.h"
#include "Emu/system_utils.hpp"
#include "Emu/vfs_config.h"
#include "Emu/emu_callbacks.h"
#include "Emu/IdManager.h"
#include "Emu/RSX/Null/NullGSRender.h"
#include "Emu/Cell/Modules/cellMsgDialog.h"
#include "Emu/Cell/Modules/cellOskDialog.h"
#include "Emu/Cell/Modules/cellSaveData.h"
#include "Emu/Cell/Modules/sceNpTrophy.h"
#include "Emu/Io/Null/null_camera_handler.h"
#include "Emu/Io/Null/null_music_handler.h"
#include "Emu/Io/Null/NullKeyboardHandler.h"
#include "Emu/Io/Null/NullMouseHandler.h"
#include "Emu/Io/KeyboardHandler.h"
#include "Emu/Io/MouseHandler.h"
#include "Emu/Audio/AudioBackend.h"
#include "Emu/Audio/Null/NullAudioBackend.h"
#include "Emu/Audio/Null/null_enumerator.h"
#include "Input/pad_thread.h"
#include "Utilities/File.h"
#include "Utilities/Thread.h"
#include "util/logs.hpp"
#include "util/sysinfo.hpp"
#include "util/video_source.h"

#include <map>
#include <set>
#include <d3d11_1.h>
#include <dxgi1_2.h>

// Firmware library list (Emu/Cell/PPUModule.cpp): true = HLE by default.
extern const std::map<std::string_view, int> g_prx_list;

LOG_CHANNEL(sys_log, "SYS");

extern std::string g_android_executable_dir;
extern std::string g_android_config_dir;
extern std::string g_android_cache_dir;

namespace
{
	enum class shell_state : u32
	{
		booting,
		running,
		finished,
		failed,
	};

	atomic_t<shell_state> g_state{shell_state::booting};
	std::string g_local_dir; // LocalState, with trailing slash

	std::string wide_to_utf8(const wchar_t* s)
	{
		const int n = WideCharToMultiByte(CP_UTF8, 0, s, -1, nullptr, 0, nullptr, nullptr);
		std::string out(std::max(n, 1) - 1, '\0');
		WideCharToMultiByte(CP_UTF8, 0, s, -1, out.data(), n, nullptr, nullptr);
		return out;
	}

	std::string get_local_state_dir()
	{
		using namespace winrt_min;
		IApplicationDataStatics* statics = nullptr;
		IApplicationData* data = nullptr;
		IInspectable* folder = nullptr;
		IStorageItem* item = nullptr;
		HSTRING path = nullptr;
		std::string result;

		if (SUCCEEDED(get_factory(L"Windows.Storage.ApplicationData", IID_IApplicationDataStatics, &statics)) &&
			SUCCEEDED(statics->get_Current(&data)) && SUCCEEDED(data->get_LocalFolder(&folder)) &&
			SUCCEEDED(folder->QueryInterface(IID_IStorageItem, reinterpret_cast<void**>(&item))) && SUCCEEDED(item->get_Path(&path)))
		{
			result = wide_to_utf8(WindowsGetStringRawBuffer(path, nullptr));
			std::replace(result.begin(), result.end(), '\\', '/');
			result += '/';
		}

		if (path) WindowsDeleteString(path);
		if (item) item->Release();
		if (folder) folder->Release();
		if (data) data->Release();
		if (statics) statics->Release();
		return result;
	}

	std::string get_package_dir()
	{
		std::wstring buf(MAX_PATH * 4, L'\0');
		const DWORD n = GetModuleFileNameW(nullptr, buf.data(), static_cast<DWORD>(buf.size()));
		buf.resize(n);
		std::string dir = wide_to_utf8(buf.c_str());
		std::replace(dir.begin(), dir.end(), '\\', '/');
		dir.resize(dir.rfind('/') + 1);
		return dir;
	}

	void log_app_memory(std::string_view when)
	{
		winrt_min::IMemoryManagerStatics* mm = nullptr;
		if (SUCCEEDED(winrt_min::get_factory(L"Windows.System.MemoryManager", winrt_min::IID_IMemoryManagerStatics, &mm)))
		{
			UINT64 usage = 0, limit = 0;
			mm->get_AppMemoryUsage(&usage);
			mm->get_AppMemoryUsageLimit(&limit);
			sys_log.notice("UWP: app memory %.3f GB of %.3f GB (%s)", usage / 1073741824.0, limit / 1073741824.0, when);
			mm->Release();
		}
	}

	// Pick what to boot: first file in LocalState/boot/, else the bundled test ELF.
	std::string find_boot_target()
	{
		const std::string boot_dir = g_local_dir + "boot/";
		fs::create_dir(boot_dir);

		for (const auto& entry : fs::dir(boot_dir))
		{
			if (!entry.is_directory && entry.name != "." && entry.name != "..")
			{
				return boot_dir + entry.name;
			}
		}

		return get_package_dir() + "test/ppu_thread.elf";
	}

	void create_callbacks()
	{
		g_emu_callbacks.update_emu_settings = []() {};
		g_emu_callbacks.save_emu_settings = []()
		{
			Emulator::SaveSettings(g_cfg.to_string(), Emu.GetTitleID());
		};

		g_emu_callbacks.try_to_quit = [](bool force_quit, std::function<void()> on_exit) -> bool
		{
			if (force_quit)
			{
				if (on_exit)
					on_exit();
				g_state = shell_state::finished;
				return true;
			}
			return false;
		};

		// The emulator thread acts as the "main thread": run requests inline.
		g_emu_callbacks.call_from_main_thread = [](std::function<void()> func, atomic_t<u32>* wake_up)
		{
			func();
			if (wake_up)
			{
				*wake_up = true;
				wake_up->notify_one();
			}
		};

		g_emu_callbacks.init_kb_handler = []()
		{
			ensure(g_fxo->init<KeyboardHandlerBase, NullKeyboardHandler>(Emu.DeserialManager()));
		};
		g_emu_callbacks.init_mouse_handler = []()
		{
			ensure(g_fxo->init<MouseHandlerBase, NullMouseHandler>(Emu.DeserialManager()));
		};
		g_emu_callbacks.init_pad_handler = [](std::string_view title_id)
		{
			ensure(g_fxo->init<named_thread<pad_thread>>(nullptr, nullptr, title_id));
		};

		g_emu_callbacks.get_audio = []() -> std::shared_ptr<AudioBackend> { return std::make_shared<NullAudioBackend>(); };
		g_emu_callbacks.get_audio_enumerator = [](u64) -> std::shared_ptr<audio_device_enumerator> { return std::make_shared<null_enumerator>(); };

		g_emu_callbacks.init_gs_render = [](utils::serial* ar)
		{
			g_fxo->init<rsx::thread, named_thread<NullGSRender>>(ar);
		};
		g_emu_callbacks.close_gs_frame = []() {};
		g_emu_callbacks.get_gs_frame = []() -> std::unique_ptr<GSFrameBase> { return {}; };
		g_emu_callbacks.get_camera_handler = []() -> std::shared_ptr<camera_handler_base> { return std::make_shared<null_camera_handler>(); };
		g_emu_callbacks.get_music_handler = []() -> std::shared_ptr<music_handler_base> { return std::make_shared<null_music_handler>(); };

		g_emu_callbacks.get_msg_dialog = []() -> std::shared_ptr<MsgDialogBase> { return {}; };
		g_emu_callbacks.get_osk_dialog = []() -> std::shared_ptr<OskDialogBase> { return {}; };
		g_emu_callbacks.get_save_dialog = []() -> std::unique_ptr<SaveDialogBase> { return {}; };
		g_emu_callbacks.get_sendmessage_dialog = []() -> std::shared_ptr<SendMessageDialogBase> { return {}; };
		g_emu_callbacks.get_recvmessage_dialog = []() -> std::shared_ptr<RecvMessageDialogBase> { return {}; };
		g_emu_callbacks.get_trophy_notification_dialog = []() -> std::unique_ptr<TrophyNotificationBase> { return {}; };

		g_emu_callbacks.on_run = [](bool) { g_state = shell_state::running; };
		g_emu_callbacks.on_pause = []() {};
		g_emu_callbacks.on_resume = []() {};
		g_emu_callbacks.on_stop = []()
		{
			if (g_state != shell_state::failed)
				g_state = shell_state::finished;
		};
		g_emu_callbacks.on_ready = []() {};
		g_emu_callbacks.on_emulation_stop_no_response = [](std::shared_ptr<atomic_t<bool>> closed, int)
		{
			if (!closed || !*closed)
			{
				sys_log.fatal("Stopping emulator took too long; some thread has probably deadlocked.");
				g_state = shell_state::failed;
			}
		};
		g_emu_callbacks.on_save_state_progress = [](std::shared_ptr<atomic_t<bool>>, stx::shared_ptr<utils::serial>, stx::atomic_ptr<std::string>*, std::shared_ptr<void>) {};
		g_emu_callbacks.enable_disc_eject = [](bool) {};
		g_emu_callbacks.enable_disc_insert = [](bool) {};
		g_emu_callbacks.on_missing_fw = []() { sys_log.error("UWP: PS3 firmware is not installed."); };
		g_emu_callbacks.handle_taskbar_progress = [](s32, s32) {};

		g_emu_callbacks.get_localized_string = [](localized_string_id, const char*) -> std::string { return {}; };
		g_emu_callbacks.get_localized_u32string = [](localized_string_id, const char*) -> std::u32string { return {}; };
		g_emu_callbacks.get_localized_setting = [](const cfg::_base*, u32) -> std::string { return {}; };
		g_emu_callbacks.get_photo_path = [](std::string_view) -> std::string { return {}; };
		g_emu_callbacks.play_sound = [](const std::string&, std::optional<f32>) {};
		g_emu_callbacks.get_image_info = [](const std::string&, std::string&, s32&, s32&, s32&) { return false; };
		g_emu_callbacks.get_scaled_image = [](const std::string&, s32, s32, s32&, s32&, u8*, bool) { return false; };
		g_emu_callbacks.get_font_dirs = []() { return std::vector<std::string>{}; };
		g_emu_callbacks.on_install_pkgs = [](const std::vector<std::string>&, bool) { return false; };
		g_emu_callbacks.add_breakpoint = [](u32) {};
		g_emu_callbacks.display_sleep_control_supported = []() { return false; };
		g_emu_callbacks.enable_display_sleep = [](bool) {};
		g_emu_callbacks.check_microphone_permissions = []() {};
		g_emu_callbacks.make_video_source = []() -> std::unique_ptr<video_source> { return nullptr; };
		g_emu_callbacks.enable_gamemode = [](bool) {};
		g_emu_callbacks.get_database_config = [](const std::string&) -> std::string { return {}; };
	}

	DWORD WINAPI emulator_thread(void*)
	{
		{
			g_local_dir = get_local_state_dir();
			g_android_executable_dir = get_package_dir();
			g_android_config_dir = g_local_dir + "rpcs3/";
			g_android_cache_dir = g_local_dir + "rpcs3/cache/";
			fs::create_path(g_android_config_dir);
			fs::create_path(g_android_cache_dir);

			static std::unique_ptr<logs::listener> log_file = logs::make_file_listener(g_android_config_dir + "RPCS3.log", 256 * 1024 * 1024);

			sys_log.always()("RPCS3 UWP shell starting");
			sys_log.always()("%s", utils::get_system_info());
			log_app_memory("startup");

			thread_ctrl::set_native_priority(0);
			create_callbacks();

			Emu.SetHasGui(false);
			Emu.SetHeadless(true);
			Emu.SetUsr("00000001");
			Emu.Init();

			g_cfg.video.renderer.set(video_renderer::null);
			g_cfg.audio.renderer.set(audio_renderer::null);

			// Built without LLVM: run PPU and SPU code on the interpreters.
			g_cfg.core.ppu_decoder.set(ppu_decoder_type::_static);
			g_cfg.core.spu_decoder.set(spu_decoder_type::_static);

			// No firmware installed yet: use RPCS3's built-in (HLE) version of every firmware
			// library, otherwise the boot stops with "firmware missing".
			if (!fs::is_file(g_cfg_vfs.get_dev_flash() + "sys/external/liblv2.sprx"))
			{
				std::set<std::string> libs;
				for (const auto& [name, hle] : g_prx_list)
				{
					if (!hle)
						libs.emplace(std::string(name) + ":hle");
				}

				g_cfg.core.libraries_control.set_set(std::move(libs));
				sys_log.warning("UWP: no firmware installed, using HLE for all firmware libraries");
			}

			// BootGame reloads the global config file, so store these settings there.
			Emulator::SaveSettings(g_cfg.to_string(), {});

			const std::string target = find_boot_target();
			sys_log.always()("UWP: booting '%s'", target);

			const game_boot_result result = Emu.BootGame(target, "", true);
			if (result != game_boot_result::no_errors)
			{
				sys_log.fatal("UWP: boot failed: %s", result);
				g_state = shell_state::failed;
				return 1;
			}

			while (g_state == shell_state::booting || g_state == shell_state::running)
			{
				Sleep(1000);
				static u32 seconds = 0;
				if (++seconds % 10 == 0)
					log_app_memory("running");
			}
		}

		log_app_memory("end");
		return 0;
	}

	template <typename I>
	struct inspectable_impl : I
	{
		LONG refs = 1;
		const GUID& iid;

		explicit inspectable_impl(const GUID& id) : iid(id) {}
		virtual ~inspectable_impl() = default;

		HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** out) override
		{
			if (riid == __uuidof(IUnknown) || riid == __uuidof(IInspectable) || riid == iid)
			{
				*out = static_cast<I*>(this);
				this->AddRef();
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
		HRESULT STDMETHODCALLTYPE GetIids(ULONG* count, IID** iids) override
		{
			*count = 0;
			*iids = nullptr;
			return S_OK;
		}
		HRESULT STDMETHODCALLTYPE GetRuntimeClassName(HSTRING* name) override
		{
			*name = nullptr;
			return S_OK;
		}
		HRESULT STDMETHODCALLTYPE GetTrustLevel(TrustLevel* level) override
		{
			*level = BaseTrust;
			return S_OK;
		}
	};

	struct view final : inspectable_impl<winrt_min::IFrameworkView>
	{
		winrt_min::ICoreWindow* window = nullptr;

		view() : inspectable_impl(winrt_min::IID_IFrameworkView) {}

		HRESULT STDMETHODCALLTYPE Initialize(IInspectable*) override { return S_OK; }
		HRESULT STDMETHODCALLTYPE SetWindow(winrt_min::ICoreWindow* w) override
		{
			window = w;
			window->AddRef();
			return S_OK;
		}
		HRESULT STDMETHODCALLTYPE Load(HSTRING) override { return S_OK; }
		HRESULT STDMETHODCALLTYPE Uninitialize() override { return S_OK; }

		HRESULT STDMETHODCALLTYPE Run() override
		{
			window->Activate();
			winrt_min::ICoreDispatcher* dispatcher = nullptr;
			window->get_Dispatcher(&dispatcher);

			// RPCS3 threads need large stacks; give the emulator its own thread.
			CloseHandle(CreateThread(nullptr, 16 * 1024 * 1024, emulator_thread, nullptr, 0, nullptr));

			ID3D11Device* dev = nullptr;
			ID3D11DeviceContext* ctx = nullptr;
			IDXGISwapChain1* sc = nullptr;
			ID3D11RenderTargetView* rtv = nullptr;
			D3D_FEATURE_LEVEL fl = D3D_FEATURE_LEVEL_11_0;
			if (SUCCEEDED(D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, D3D11_CREATE_DEVICE_BGRA_SUPPORT, &fl, 1, D3D11_SDK_VERSION, &dev, nullptr, &ctx)))
			{
				IDXGIDevice* dxdev = nullptr;
				IDXGIAdapter* adapter = nullptr;
				IDXGIFactory2* factory = nullptr;
				dev->QueryInterface(__uuidof(IDXGIDevice), reinterpret_cast<void**>(&dxdev));
				dxdev->GetAdapter(&adapter);
				adapter->GetParent(__uuidof(IDXGIFactory2), reinterpret_cast<void**>(&factory));
				DXGI_SWAP_CHAIN_DESC1 desc{};
				desc.Width = 1920;
				desc.Height = 1080;
				desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
				desc.SampleDesc.Count = 1;
				desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
				desc.BufferCount = 2;
				desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
				desc.Scaling = DXGI_SCALING_STRETCH;
				if (SUCCEEDED(factory->CreateSwapChainForCoreWindow(dev, reinterpret_cast<IUnknown*>(window), &desc, nullptr, &sc)))
				{
					ID3D11Texture2D* backbuffer = nullptr;
					sc->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&backbuffer));
					dev->CreateRenderTargetView(backbuffer, nullptr, &rtv);
					backbuffer->Release();
				}
				factory->Release();
				adapter->Release();
				dxdev->Release();
			}

			for (;;)
			{
				dispatcher->ProcessEvents(0 /* ProcessAllIfPresent */);
				if (!sc || !rtv)
				{
					Sleep(100);
					continue;
				}
				static const float blue[4] = {0.10f, 0.25f, 0.60f, 1}, green[4] = {0.10f, 0.55f, 0.20f, 1}, red[4] = {0.70f, 0.12f, 0.12f, 1};
				const shell_state st = g_state;
				ctx->ClearRenderTargetView(rtv, st == shell_state::failed ? red : st == shell_state::finished ? green : blue);
				sc->Present(1, 0);
			}
			return S_OK;
		}
	};

	struct view_source final : inspectable_impl<winrt_min::IFrameworkViewSource>
	{
		view_source() : inspectable_impl(winrt_min::IID_IFrameworkViewSource) {}

		HRESULT STDMETHODCALLTYPE CreateView(winrt_min::IFrameworkView** out) override
		{
			*out = new view();
			return S_OK;
		}
	};
}

// Called by the emulator for unrecoverable errors.
[[noreturn]] void report_fatal_error(std::string_view text, bool /*is_html*/ = false, bool /*include_help_text*/ = true)
{
	sys_log.fatal("%s", text);
	g_state = shell_state::failed;
	logs::listener::sync_all();
	// Keep the red screen up so the user can see it; the emulator thread stops here.
	for (;;)
		Sleep(INFINITE);
}

int WINAPI wWinMain(HINSTANCE, HINSTANCE, PWSTR, int)
{
	RoInitialize(RO_INIT_MULTITHREADED);
	winrt_min::ICoreApplication* app = nullptr;
	if (FAILED(winrt_min::get_factory(L"Windows.ApplicationModel.Core.CoreApplication", winrt_min::IID_ICoreApplication, &app)))
		return 1;
	auto* source = new view_source();
	app->Run(source);
	source->Release();
	app->Release();
	return 0;
}
