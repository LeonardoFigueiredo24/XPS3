// RPCS3 UWP shell for Xbox Series X|S Dev Mode.
//
// On launch it sets up RPCS3's folders in the app's LocalState and shows the controller
// menu (uwp_frontend.cpp): game list, firmware installer, settings. An empty
// LocalState/autoboot file skips the menu and boots the first program in LocalState/boot/
// (or the bundled test ELF) like the earlier test builds, or the XMB if it contains "xmb".
// With firmware installed and no LocalState/menu file, the XMB boots at startup. RPCS3.log goes to LocalState/rpcs3.
// While a game boots or runs the screen shows its frames, or a status color when there are
// none: blue = booting/running, green = exited cleanly, red = boot failed or fatal error.

#include "stdafx.h"
#include <algorithm>
#include <winsock2.h>
#include "winrt_min.h"
#include "uwp_frontend.h"

#include "Emu/System.h"
#include "Emu/system_config.h"
#include "Emu/system_utils.hpp"
#include "Emu/vfs_config.h"
#include "Emu/emu_callbacks.h"
#include "Emu/IdManager.h"
#include "Emu/RSX/Null/NullGSRender.h"
#include "Emu/RSX/GSFrameBase.h"
#include "Emu/RSX/VK/VKGSRender.h"
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
#include "Emu/Audio/XAudio2/XAudio2Backend.h"
#include "uwp_input.h"
#include "Input/pad_thread.h"
#include "Utilities/File.h"
#include "Utilities/Thread.h"
#include "util/logs.hpp"
#include "util/sysinfo.hpp"
#include "util/video_source.h"

#include <cstdio>
#include <deque>
#include <functional>
#include <map>
#include <mutex>
#include <vector>
#include <d3d11_1.h>
#include <dxgi1_2.h>

LOG_CHANNEL(vk_drv_log, "DZN");
extern "C" void uwp_heap_guard_open_log(const wchar_t* path);
extern "C" void (*uwp_vkshim_log)(const char* where, const char* text);
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

	// Output size of the emulated display; matches the CoreWindow swapchain.
	constexpr u32 c_display_width = 1920;
	constexpr u32 c_display_height = 1080;

	// Latest frame from the Vulkan renderer, picked up by the UI thread.
	std::mutex g_frame_mutex;
	// Triple buffer: the emulator fills g_frame_spare without the lock and swaps it into
	// g_frame_pixels; the render loop swaps that out into its own buffer, so neither side
	// holds the lock during an 8 MB copy.
	std::vector<u8> g_frame_pixels, g_frame_spare;
	std::mutex g_frame_spare_mutex; // only the emulator side; guards g_frame_spare
	u32 g_frame_width = 0, g_frame_height = 0;
	u64 g_frame_serial = 0;

	// Stand-in for the desktop game window: fixed size, no native handle. The Vulkan
	// renderer presents through swapchain_UWP, which calls uwp_present_frame().
	class uwp_gs_frame final : public GSFrameBase
	{
	public:
		void close() override {}
		void reset() override {}
		bool shown() override { return true; }
		void hide() override {}
		void show() override {}
		void toggle_fullscreen() override {}

		void delete_context(draw_context_t) override {}
		draw_context_t make_context() override { return nullptr; }
		void set_current(draw_context_t) override {}
		void flip(draw_context_t, bool) override {}
		int client_width() override { return c_display_width; }
		int client_height() override { return c_display_height; }
		f64 client_display_rate() override { return 60.0; }
		bool has_alpha() override { return false; }

		display_handle_t handle() const override { return nullptr; }

		bool can_consume_frame() const override { return false; }
		void present_frame(std::vector<u8>&&, u32, u32, u32, bool) const override {}
		void take_screenshot(std::vector<u8>&&, u32, u32, bool) override {}

		void update_title(double) override {}
	};

	// The emulator thread plays the role of desktop RPCS3's Qt main thread: requests from
	// other threads are queued and run by its loop, never on the requesting thread (the
	// emulation join thread would otherwise wait on itself during shutdown).
	DWORD g_main_thread_id = 0;
	std::mutex g_main_queue_mutex;
	std::deque<std::pair<std::function<void()>, atomic_t<u32>*>> g_main_queue;

	void run_main_thread_queue()
	{
		while (true)
		{
			std::pair<std::function<void()>, atomic_t<u32>*> item;
			{
				std::lock_guard lock(g_main_queue_mutex);
				if (g_main_queue.empty())
					return;
				item = std::move(g_main_queue.front());
				g_main_queue.pop_front();
			}

			item.first();
			if (item.second)
			{
				*item.second = true;
				item.second->notify_one();
			}
		}
	}
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

	// Pick what to boot: first file in LocalState/boot/, else the bundled test ELF (a GCM triangle).
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

		return get_package_dir() + "test/gs_gcm_basic_triangle.elf";
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

		g_emu_callbacks.call_from_main_thread = [](std::function<void()> func, atomic_t<u32>* wake_up)
		{
			if (GetCurrentThreadId() == g_main_thread_id)
			{
				func();
				if (wake_up)
				{
					*wake_up = true;
					wake_up->notify_one();
				}
				return;
			}

			std::lock_guard lock(g_main_queue_mutex);
			g_main_queue.emplace_back(std::move(func), wake_up);
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

		g_emu_callbacks.get_audio = []() -> std::shared_ptr<AudioBackend>
		{
			if (g_cfg.audio.renderer == audio_renderer::xaudio)
			{
				std::shared_ptr<AudioBackend> xaudio = std::make_shared<XAudio2Backend>();
				if (xaudio->Initialized())
					return xaudio;
				sys_log.error("UWP: XAudio2 failed to start, no sound");
			}
			return std::make_shared<NullAudioBackend>();
		};
		g_emu_callbacks.get_audio_enumerator = [](u64) -> std::shared_ptr<audio_device_enumerator> { return std::make_shared<null_enumerator>(); };

		g_emu_callbacks.init_gs_render = [](utils::serial* ar)
		{
			if (g_cfg.video.renderer == video_renderer::vulkan)
				g_fxo->init<rsx::thread, named_thread<VKGSRender>>(ar);
			else
				g_fxo->init<rsx::thread, named_thread<NullGSRender>>(ar);
		};
		g_emu_callbacks.close_gs_frame = []() {};
		g_emu_callbacks.get_gs_frame = []() -> std::unique_ptr<GSFrameBase> { return std::make_unique<uwp_gs_frame>(); };
		g_emu_callbacks.get_camera_handler = []() -> std::shared_ptr<camera_handler_base> { return std::make_shared<null_camera_handler>(); };
		g_emu_callbacks.get_music_handler = []() -> std::shared_ptr<music_handler_base> { return std::make_shared<null_music_handler>(); };

		g_emu_callbacks.get_msg_dialog = []() -> std::shared_ptr<MsgDialogBase> { return {}; };
		g_emu_callbacks.get_osk_dialog = []() -> std::shared_ptr<OskDialogBase> { return {}; };
		g_emu_callbacks.get_save_dialog = []() -> std::unique_ptr<SaveDialogBase> { return {}; };
		g_emu_callbacks.get_sendmessage_dialog = []() -> std::shared_ptr<SendMessageDialogBase> { return {}; };
		g_emu_callbacks.get_recvmessage_dialog = []() -> std::shared_ptr<RecvMessageDialogBase> { return {}; };
		g_emu_callbacks.get_trophy_notification_dialog = []() -> std::unique_ptr<TrophyNotificationBase> { return {}; };

		g_emu_callbacks.on_run = [](bool)
		{
			g_state = shell_state::running;
			uwp_frontend::on_emulation_started();
		};
		g_emu_callbacks.on_pause = []() {};
		g_emu_callbacks.on_resume = []() {};
		g_emu_callbacks.on_stop = []()
		{
			if (g_state != shell_state::failed)
				g_state = shell_state::finished;
			uwp_frontend::on_emulation_stopped();
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
		g_emu_callbacks.get_font_dirs = []() { return std::vector<std::string>{get_package_dir() + "fonts/"}; };
		g_emu_callbacks.on_install_pkgs = [](const std::vector<std::string>&, bool) { return false; };
		g_emu_callbacks.add_breakpoint = [](u32) {};
		g_emu_callbacks.display_sleep_control_supported = []() { return false; };
		g_emu_callbacks.enable_display_sleep = [](bool) {};
		g_emu_callbacks.check_microphone_permissions = []() {};
		g_emu_callbacks.make_video_source = []() -> std::unique_ptr<video_source> { return nullptr; };
		g_emu_callbacks.enable_gamemode = [](bool) {};
		g_emu_callbacks.get_database_config = [](const std::string&) -> std::string { return {}; };

		// Desktop resolves paths with Qt's canonicalFilePath (forward slashes, no trailing slash,
		// empty if missing). Boot code relies on that, e.g. "/dev_flash" + path.substr(resolved
		// dev_flash size) for firmware executables, which lost its '/' with the default callback.
		g_emu_callbacks.resolve_path_may_not_exist = [](std::string_view sv) -> std::string
		{
			std::string path(sv);
			std::replace(path.begin(), path.end(), '\\', '/');
			while (path.size() > 1 && path.back() == '/' && !(path.size() == 3 && path[1] == ':'))
				path.pop_back();
			return path;
		};
		g_emu_callbacks.resolve_path = [](std::string_view sv) -> std::string
		{
			if (!fs::exists(std::string(sv)))
				return {};
			return g_emu_callbacks.resolve_path_may_not_exist(sv);
		};
	}

	DWORD WINAPI emulator_thread(void*)
	{
		{
			g_local_dir = get_local_state_dir();

			// Desktop RPCS3 starts Winsock in main(); without it every guest socket call fails
			// with WSANOTINITIALISED (10093), which sys_net treats as fatal (seen in the XMB).
			{
				WSADATA wsa_data{};
				const int wsa_res = ::WSAStartup(MAKEWORD(2, 2), &wsa_data);
				if (wsa_res != 0)
					OutputDebugStringA("UWP: WSAStartup failed\n");
			}
			{
				const std::string guard_log = g_local_dir + "heap_guard.log";
				std::wstring wpath(guard_log.size() + 1, L'\0');
				wpath.resize(::MultiByteToWideChar(CP_UTF8, 0, guard_log.c_str(), -1, wpath.data(), static_cast<int>(wpath.size())));
				uwp_heap_guard_open_log(wpath.c_str());
			}
			g_android_executable_dir = get_package_dir();
			g_android_config_dir = g_local_dir + "rpcs3/";
			g_android_cache_dir = g_local_dir + "rpcs3/cache/";
			fs::create_path(g_android_config_dir);
			fs::create_path(g_android_cache_dir);

			// Overlay UI icons ship in the package; RPCS3 looks for them in its config dir.
			const std::string icons_src = get_package_dir() + "Icons/ui/";
			const std::string icons_dst = g_android_config_dir + "Icons/ui/";
			fs::create_path(icons_dst);
			for (const auto& entry : fs::dir(icons_src))
			{
				if (!entry.is_directory && !fs::is_file(icons_dst + entry.name))
					fs::copy_file(icons_src + entry.name, icons_dst + entry.name, false);
			}

			static std::unique_ptr<logs::listener> log_file = logs::make_file_listener(g_android_config_dir + "RPCS3.log", 256 * 1024 * 1024);

			// Any fatal log line (e.g. a dying RSX thread, which doesn't stop the emulator) turns the screen red.
			struct fatal_listener final : logs::listener
			{
				void log(u64, const logs::message& msg, std::string_view, std::string_view) override
				{
					if (logs::level{msg} == logs::level::fatal)
						g_state = shell_state::failed;
				}
			};
			static fatal_listener s_fatal_listener;
			logs::listener::add(&s_fatal_listener);

			// Mesa's own messages (shader compiler, D3D12 failures) go to stderr; keep them in LocalState.
			if (FILE* f = std::freopen((g_local_dir + "mesa.log").c_str(), "w", stderr))
				std::setvbuf(f, nullptr, _IONBF, 0);
			_putenv_s("MESA_LOG_LEVEL", "info");
			// D3D12 debug layer for Dozen only on request (LocalState/d3d12_debug). On the Xbox it is
			// only partly present and runs since it was enabled by default (builds 12-18) hit heap corruption.
			if (fs::is_file(g_local_dir + "d3d12_debug"))
				_putenv_s("DZN_DEBUG", "d3d12");
			// VK_EXT_external_memory_host (D3D12 heaps opened on PS3 memory) only on request
			// (LocalState/ext_host_mem) while it is a suspect for the startup heap corruption.
			if (fs::is_file(g_local_dir + "ext_host_mem"))
				_putenv_s("DZN_EXT_HOST_MEM", "1");

			// Errors and warnings from the Vulkan driver (Mesa Dozen), with Mesa's source file:line.
			uwp_vkshim_log = [](const char* where, const char* text)
			{
				vk_drv_log.error("%s: %s", where, text);
			};

			sys_log.always()("RPCS3 UWP shell starting");
			sys_log.always()("%s", utils::get_system_info());
			log_app_memory("startup");

			thread_ctrl::set_native_priority(0);
			g_main_thread_id = GetCurrentThreadId();
			create_callbacks();

			Emu.SetSupportedRenderers({video_renderer::null, video_renderer::vulkan});
			Emu.SetHasGui(false);
			Emu.SetHeadless(false); // headless mode would force the null renderer
			Emu.SetUsr("00000001");
			Emu.Init();

			// Vulkan through Mesa's Dozen (D3D12) unless the menu's settings chose Null. An empty
			// LocalState/null_renderer file forces the null renderer for CPU-only tests.
			const bool use_null = fs::is_file(g_local_dir + "null_renderer");
			if (use_null)
				g_cfg.video.renderer.set(video_renderer::null);
			else if (g_cfg.video.renderer != video_renderer::null)
				g_cfg.video.renderer.set(video_renderer::vulkan);
			sys_log.always()("UWP: renderer: %s", g_cfg.video.renderer.get());
			// XAudio2 sound; an empty LocalState/null_audio file turns it off.
			const bool null_audio = fs::is_file(g_local_dir + "null_audio");
			g_cfg.audio.renderer.set(null_audio ? audio_renderer::null : audio_renderer::xaudio);
			sys_log.always()("UWP: audio: %s", null_audio ? "null" : "XAudio2");
			// Controllers 1-4 as PS3 pads 1-4 (written once; the XMB and games read it at boot).
			uwp_input_default_config();
			g_cfg.audio.music.set(music_handler::null);
			g_cfg.io.camera.set(camera_handler::null);

			// Built without LLVM: PPU on the interpreter, SPU on the ASMJIT recompiler (needs no
			// LLVM). An empty LocalState/spu_interpreter file falls back to the SPU interpreter.
			const bool spu_interp = fs::is_file(g_local_dir + "spu_interpreter");
			g_cfg.core.ppu_decoder.set(ppu_decoder_type::_static);
			g_cfg.core.spu_decoder.set(spu_interp ? spu_decoder_type::_static : spu_decoder_type::asmjit);
			sys_log.always()("UWP: SPU decoder: %s", g_cfg.core.spu_decoder.get());

			Emulator::SaveSettings(g_cfg.to_string(), {});

			uwp_frontend::init({g_local_dir, get_package_dir(), [](std::function<void()> func)
			{
				g_emu_callbacks.call_from_main_thread(std::move(func), nullptr);
			}});

			// With firmware installed the app boots the PS3 system menu (XMB) straight away;
			// an empty LocalState/menu file opens the controller menu instead.
			const std::string xmb_path = g_cfg_vfs.get_dev_flash() + "vsh/module/vsh.self";
			if (fs::is_file(g_local_dir + "autoboot"))
			{
				fs::file f(g_local_dir + "autoboot");
				const std::string what = f ? fmt::trim(f.to_string(), " \t\r\n") : std::string();
				const std::string target = what == "xmb" ? xmb_path : find_boot_target();
				if (!uwp_frontend::boot(target))
					g_state = shell_state::failed;
			}
			else if (!fs::is_file(g_local_dir + "menu") && fs::is_file(xmb_path))
			{
				sys_log.always()("UWP: firmware installed, booting the XMB (create LocalState/menu to open the menu instead)");
				if (!uwp_frontend::boot(xmb_path))
					g_state = shell_state::failed;
			}

			// Serve main-thread requests for as long as the app lives (shutdown needs them too).
			bool logged_end = false;
			for (u32 ticks = 1;; ticks++)
			{
				run_main_thread_queue();
				Sleep(10);

				if (g_state == shell_state::booting || g_state == shell_state::running)
				{
					if (ticks % 1000 == 0)
						log_app_memory("running");
				}
				else if (!logged_end && Emu.IsStopped(true))
				{
					log_app_memory("end");
					logged_end = true;
				}
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
			uwp_input_attach_view();
			winrt_min::ICoreDispatcher* dispatcher = nullptr;
			window->get_Dispatcher(&dispatcher);

			// RPCS3 threads need large stacks; give the emulator its own thread.
			CloseHandle(CreateThread(nullptr, 16 * 1024 * 1024, emulator_thread, nullptr, 0, nullptr));

			ID3D11Device* dev = nullptr;
			ID3D11DeviceContext* ctx = nullptr;
			IDXGISwapChain1* sc = nullptr;
			ID3D11RenderTargetView* rtv = nullptr;
			const D3D_FEATURE_LEVEL fls[] = { D3D_FEATURE_LEVEL_12_1, D3D_FEATURE_LEVEL_12_0, D3D_FEATURE_LEVEL_11_1, D3D_FEATURE_LEVEL_11_0 };
			D3D_FEATURE_LEVEL got_fl{};
			const HRESULT hr_dev = D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, D3D11_CREATE_DEVICE_BGRA_SUPPORT, fls, 4, D3D11_SDK_VERSION, &dev, &got_fl, &ctx);
			sys_log.notice("UWP: D3D11CreateDevice: 0x%08x, feature level 0x%x", static_cast<u32>(hr_dev), static_cast<u32>(got_fl));
			if (SUCCEEDED(hr_dev))
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
				const HRESULT hr_sc = factory->CreateSwapChainForCoreWindow(dev, reinterpret_cast<IUnknown*>(window), &desc, nullptr, &sc);
				sys_log.notice("UWP: CreateSwapChainForCoreWindow: 0x%08x", static_cast<u32>(hr_sc));
				if (SUCCEEDED(hr_sc))
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
				dispatcher->ProcessEvents(3 /* CoreProcessEventsOption::ProcessAllIfPresent: don't wait for input */);
				if (!sc || !rtv)
				{
					Sleep(100);
					continue;
				}

				// The menu, whenever no game is booting or running.
				const u32* menu_pixels = nullptr;
				bool menu_changed = false;
				if (uwp_frontend::frame(window, menu_pixels, menu_changed))
				{
					static ID3D11Texture2D* menu_tex = nullptr;
					if (!menu_tex)
					{
						D3D11_TEXTURE2D_DESC td{};
						td.Width = uwp_frontend::canvas_width;
						td.Height = uwp_frontend::canvas_height;
						td.MipLevels = 1;
						td.ArraySize = 1;
						td.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
						td.SampleDesc.Count = 1;
						td.Usage = D3D11_USAGE_DEFAULT;
						dev->CreateTexture2D(&td, nullptr, &menu_tex);
						menu_changed = true;
					}
					if (menu_tex)
					{
						if (menu_changed)
							ctx->UpdateSubresource(menu_tex, 0, nullptr, menu_pixels, uwp_frontend::canvas_width * 4, 0);
						ID3D11Resource* target = nullptr;
						rtv->GetResource(&target);
						ctx->CopyResource(target, menu_tex);
						target->Release();
					}
					sc->Present(1, 0);
					continue;
				}
				static const float blue[4] = {0.10f, 0.25f, 0.60f, 1}, green[4] = {0.10f, 0.55f, 0.20f, 1}, red[4] = {0.70f, 0.12f, 0.12f, 1};
				const shell_state st = g_state;

				// Show the emulator's latest frame while it runs; otherwise the status color.
				static u64 shown_serial = 0;
				static ID3D11Texture2D* frame_tex = nullptr;
				static std::vector<u8> shown_pixels;
				bool have_frame = false, upload = false;
				if (st == shell_state::running || st == shell_state::booting)
				{
					std::lock_guard lock(g_frame_mutex);
					if (g_frame_serial && g_frame_width == c_display_width && g_frame_height == c_display_height)
					{
						if (!frame_tex)
						{
							D3D11_TEXTURE2D_DESC td{};
							td.Width = c_display_width;
							td.Height = c_display_height;
							td.MipLevels = 1;
							td.ArraySize = 1;
							td.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
							td.SampleDesc.Count = 1;
							td.Usage = D3D11_USAGE_DEFAULT;
							dev->CreateTexture2D(&td, nullptr, &frame_tex);
						}
						if (frame_tex && shown_serial != g_frame_serial)
						{
							g_frame_pixels.swap(shown_pixels);
							shown_serial = g_frame_serial;
							upload = true;
						}
						have_frame = frame_tex != nullptr;
					}
				}
				if (upload)
					ctx->UpdateSubresource(frame_tex, 0, nullptr, shown_pixels.data(), c_display_width * 4, 0);

				if (have_frame)
				{
					ID3D11Resource* target = nullptr;
					rtv->GetResource(&target);
					ctx->CopyResource(target, frame_tex);
					target->Release();
				}
				else
				{
					ctx->ClearRenderTargetView(rtv, st == shell_state::failed ? red : st == shell_state::finished ? green : blue);
				}
				const HRESULT hr_present = sc->Present(1, 0);

				static u32 frames = 0;
				static shell_state last_logged = shell_state::booting;
				if (++frames == 1 || FAILED(hr_present) || st != last_logged)
				{
					sys_log.notice("UWP: frame %u, state %u, Present: 0x%08x", frames, static_cast<u32>(st), static_cast<u32>(hr_present));
					last_logged = st;
				}
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

// Used by the Vulkan renderer's UWP swapchain (Emu/RSX/VK/vkutils/swapchain_uwp.hpp).
void uwp_get_display_size(u32& width, u32& height)
{
	width = c_display_width;
	height = c_display_height;
}

void uwp_present_frame(const void* bgra_pixels, u32 width, u32 height)
{
	std::lock_guard spare_lock(g_frame_spare_mutex);
	g_frame_spare.assign(static_cast<const u8*>(bgra_pixels), static_cast<const u8*>(bgra_pixels) + usz{width} * height * 4);
	std::lock_guard lock(g_frame_mutex);
	g_frame_pixels.swap(g_frame_spare);
	g_frame_width = width;
	g_frame_height = height;
	g_frame_serial++;
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
