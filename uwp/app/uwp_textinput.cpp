// System keyboard for the menu's text fields: a Windows.UI.Text.Core edit context the keyboard
// types into, shown with Windows.UI.ViewManagement.InputPane. MinGW has no WinRT headers, so the
// interfaces are declared by hand. Vtable order and IIDs follow the Windows SDK metadata (as in
// windows-rs); the delegate IIDs are the parameterized TypedEventHandler<,> IIDs computed from
// their type signatures.

#include "stdafx.h"
#include "uwp_textinput.h"

#include "Utilities/StrUtil.h"

#include <roapi.h>
#include <winstring.h>
#include <inspectable.h>

#include <atomic>
#include <chrono>

LOG_CHANNEL(text_log, "UWPTEXT");

namespace uwp_textinput
{
	namespace
	{
#define TI_SLOT(n) virtual HRESULT STDMETHODCALLTYPE slot_##n() = 0

		struct event_token
		{
			INT64 value;
		};

		struct text_range
		{
			INT32 start;
			INT32 end;
		};

		struct rect_f
		{
			float x, y, width, height;
		};

		struct ICoreTextTextRequest : IInspectable
		{
			virtual HRESULT STDMETHODCALLTYPE get_Range(text_range* value) = 0;
			TI_SLOT(get_Text);
			virtual HRESULT STDMETHODCALLTYPE put_Text(HSTRING value) = 0;
			TI_SLOT(get_IsCanceled);
			TI_SLOT(GetDeferral);
		};

		struct ICoreTextTextRequestedEventArgs : IInspectable
		{
			virtual HRESULT STDMETHODCALLTYPE get_Request(ICoreTextTextRequest** value) = 0;
		};

		struct ICoreTextSelectionRequest : IInspectable
		{
			TI_SLOT(get_Selection);
			virtual HRESULT STDMETHODCALLTYPE put_Selection(text_range value) = 0;
			TI_SLOT(get_IsCanceled);
			TI_SLOT(GetDeferral);
		};

		struct ICoreTextSelectionRequestedEventArgs : IInspectable
		{
			virtual HRESULT STDMETHODCALLTYPE get_Request(ICoreTextSelectionRequest** value) = 0;
		};

		struct ICoreTextTextUpdatingEventArgs : IInspectable
		{
			virtual HRESULT STDMETHODCALLTYPE get_Range(text_range* value) = 0;
			virtual HRESULT STDMETHODCALLTYPE get_Text(HSTRING* value) = 0;
			virtual HRESULT STDMETHODCALLTYPE get_NewSelection(text_range* value) = 0;
			TI_SLOT(get_InputLanguage);
			TI_SLOT(get_Result);
			virtual HRESULT STDMETHODCALLTYPE put_Result(INT32 value) = 0; // CoreTextTextUpdatingResult
			TI_SLOT(get_IsCanceled);
			TI_SLOT(GetDeferral);
		};

		struct ICoreTextEditContext : IInspectable
		{
			TI_SLOT(get_Name);
			TI_SLOT(put_Name);
			TI_SLOT(get_InputScope);
			virtual HRESULT STDMETHODCALLTYPE put_InputScope(INT32 value) = 0; // CoreTextInputScope
			TI_SLOT(get_IsReadOnly);
			TI_SLOT(put_IsReadOnly);
			TI_SLOT(get_InputPaneDisplayPolicy);
			virtual HRESULT STDMETHODCALLTYPE put_InputPaneDisplayPolicy(INT32 value) = 0;
			virtual HRESULT STDMETHODCALLTYPE add_TextRequested(IUnknown* handler, event_token* token) = 0;
			TI_SLOT(remove_TextRequested);
			virtual HRESULT STDMETHODCALLTYPE add_SelectionRequested(IUnknown* handler, event_token* token) = 0;
			TI_SLOT(remove_SelectionRequested);
			TI_SLOT(add_LayoutRequested);
			TI_SLOT(remove_LayoutRequested);
			virtual HRESULT STDMETHODCALLTYPE add_TextUpdating(IUnknown* handler, event_token* token) = 0;
			TI_SLOT(remove_TextUpdating);
			TI_SLOT(add_SelectionUpdating);
			TI_SLOT(remove_SelectionUpdating);
			TI_SLOT(add_FormatUpdating);
			TI_SLOT(remove_FormatUpdating);
			TI_SLOT(add_CompositionStarted);
			TI_SLOT(remove_CompositionStarted);
			TI_SLOT(add_CompositionCompleted);
			TI_SLOT(remove_CompositionCompleted);
			virtual HRESULT STDMETHODCALLTYPE add_FocusRemoved(IUnknown* handler, event_token* token) = 0;
			TI_SLOT(remove_FocusRemoved);
			virtual HRESULT STDMETHODCALLTYPE NotifyFocusEnter() = 0;
			virtual HRESULT STDMETHODCALLTYPE NotifyFocusLeave() = 0;
			virtual HRESULT STDMETHODCALLTYPE NotifyTextChanged(text_range modified, INT32 new_length, text_range new_selection) = 0;
			virtual HRESULT STDMETHODCALLTYPE NotifySelectionChanged(text_range selection) = 0;
			TI_SLOT(NotifyLayoutChanged);
		};

		struct ICoreTextServicesManager : IInspectable
		{
			TI_SLOT(get_InputLanguage);
			TI_SLOT(add_InputLanguageChanged);
			TI_SLOT(remove_InputLanguageChanged);
			virtual HRESULT STDMETHODCALLTYPE CreateEditContext(ICoreTextEditContext** value) = 0;
		};

		struct ICoreTextServicesManagerStatics : IInspectable
		{
			virtual HRESULT STDMETHODCALLTYPE GetForCurrentView(ICoreTextServicesManager** value) = 0;
		};

		struct IInputPane : IInspectable
		{
			virtual HRESULT STDMETHODCALLTYPE add_Showing(IUnknown* handler, event_token* token) = 0;
			TI_SLOT(remove_Showing);
			virtual HRESULT STDMETHODCALLTYPE add_Hiding(IUnknown* handler, event_token* token) = 0;
			TI_SLOT(remove_Hiding);
			virtual HRESULT STDMETHODCALLTYPE get_OccludedRect(rect_f* value) = 0;
		};

		struct IInputPane2 : IInspectable
		{
			virtual HRESULT STDMETHODCALLTYPE TryShow(boolean* result) = 0;
			virtual HRESULT STDMETHODCALLTYPE TryHide(boolean* result) = 0;
		};

		struct IInputPaneStatics : IInspectable
		{
			virtual HRESULT STDMETHODCALLTYPE GetForCurrentView(IInputPane** value) = 0;
		};
#undef TI_SLOT

		// {1520a388-e2cf-4d65-aeb9-b32d86fe39b9}
		constexpr GUID IID_ICoreTextServicesManagerStatics{0x1520a388, 0xe2cf, 0x4d65, {0xae, 0xb9, 0xb3, 0x2d, 0x86, 0xfe, 0x39, 0xb9}};
		// {95f4af3a-ef47-424a-9741-fd2815eba2bd}
		constexpr GUID IID_IInputPaneStatics{0x95f4af3a, 0xef47, 0x424a, {0x97, 0x41, 0xfd, 0x28, 0x15, 0xeb, 0xa2, 0xbd}};
		// {8a6b3f26-7090-4793-944c-c3f2cde26276}
		constexpr GUID IID_IInputPane2{0x8a6b3f26, 0x7090, 0x4793, {0x94, 0x4c, 0xc3, 0xf2, 0xcd, 0xe2, 0x62, 0x76}};
		// {94ea2b94-e9cc-49e0-c0ff-ee64ca8f5b90}
		constexpr GUID IID_IAgileObject_{0x94ea2b94, 0xe9cc, 0x49e0, {0xc0, 0xff, 0xee, 0x64, 0xca, 0x8f, 0x5b, 0x90}};
		constexpr GUID IID_IUnknown_{0x00000000, 0x0000, 0x0000, {0xc0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46}};

		// TypedEventHandler<CoreTextEditContext, CoreTextTextRequestedEventArgs> {c7e08176-4134-50b7-bc73-729e9f9ad22a}
		constexpr GUID IID_TextRequestedHandler{0xc7e08176, 0x4134, 0x50b7, {0xbc, 0x73, 0x72, 0x9e, 0x9f, 0x9a, 0xd2, 0x2a}};
		// TypedEventHandler<CoreTextEditContext, CoreTextSelectionRequestedEventArgs> {882e51e1-f4a3-57e5-9392-6a8c38579181}
		constexpr GUID IID_SelectionRequestedHandler{0x882e51e1, 0xf4a3, 0x57e5, {0x93, 0x92, 0x6a, 0x8c, 0x38, 0x57, 0x91, 0x81}};
		// TypedEventHandler<CoreTextEditContext, CoreTextTextUpdatingEventArgs> {fd896a84-df7c-50d5-9167-58f616bddb6e}
		constexpr GUID IID_TextUpdatingHandler{0xfd896a84, 0xdf7c, 0x50d5, {0x91, 0x67, 0x58, 0xf6, 0x16, 0xbd, 0xdb, 0x6e}};
		// TypedEventHandler<CoreTextEditContext, Object> {ef53b467-c472-5b59-a827-38adc3a9d326}
		constexpr GUID IID_FocusRemovedHandler{0xef53b467, 0xc472, 0x5b59, {0xa8, 0x27, 0x38, 0xad, 0xc3, 0xa9, 0xd3, 0x26}};
		// TypedEventHandler<InputPane, InputPaneVisibilityEventArgs> {b813d684-d953-5a8a-9b30-78b79fb9147b}
		constexpr GUID IID_InputPaneVisibilityHandler{0xb813d684, 0xd953, 0x5a8a, {0x9b, 0x30, 0x78, 0xb7, 0x9f, 0xb9, 0x14, 0x7b}};

		// CoreTextInputScope values.
		constexpr INT32 scope_url = 1, scope_email = 5, scope_username = 6, scope_password = 31, scope_alnum = 40;

		// A TypedEventHandler<,> for one event: Invoke(sender, args).
		struct handler final : IUnknown
		{
			GUID iid;
			std::function<void(IInspectable*)> fn; // gets args
			std::atomic<ULONG> refs{1};

			handler(const GUID& iid, std::function<void(IInspectable*)> fn)
				: iid(iid), fn(std::move(fn))
			{
			}

			HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** out) override
			{
				if (IsEqualGUID(riid, iid) || IsEqualGUID(riid, IID_IUnknown_) || IsEqualGUID(riid, IID_IAgileObject_))
				{
					*out = this;
					AddRef();
					return S_OK;
				}
				*out = nullptr;
				return E_NOINTERFACE;
			}

			ULONG STDMETHODCALLTYPE AddRef() override
			{
				return ++refs;
			}

			ULONG STDMETHODCALLTYPE Release() override
			{
				const ULONG n = --refs;
				if (!n)
					delete this;
				return n;
			}

			virtual HRESULT STDMETHODCALLTYPE Invoke(IInspectable* /*sender*/, IInspectable* args)
			{
				fn(args);
				return S_OK;
			}
		};

		template <typename T>
		HRESULT get_factory(const wchar_t* class_name, const GUID& iid, T** out)
		{
			HSTRING_HEADER header;
			HSTRING name;
			HRESULT hr = WindowsCreateStringReference(class_name, static_cast<UINT32>(wcslen(class_name)), &header, &name);
			if (FAILED(hr))
				return hr;
			return RoGetActivationFactory(name, iid, reinterpret_cast<void**>(out));
		}

		// UI-thread state.
		bool g_init_tried = false;
		ICoreTextEditContext* g_context = nullptr;
		IInputPane* g_pane = nullptr;
		IInputPane2* g_pane2 = nullptr;

		bool g_active = false;          // editing a field
		bool g_pane_shown = false;      // Showing event since show()
		bool g_pane_hidden = false;     // Hiding or FocusRemoved event since show()
		std::chrono::steady_clock::time_point g_shown_at{};
		std::u16string g_text;
		usz g_max_len = 0;
		std::function<bool(char16_t)> g_accept;
		std::function<char16_t(char16_t)> g_transform;
		bool g_changed = false;
		bool g_submitted = false;

		INT32 text_len()
		{
			return static_cast<INT32>(g_text.size());
		}

		void on_text_requested(IInspectable* args)
		{
			ICoreTextTextRequest* req = nullptr;
			if (FAILED(reinterpret_cast<ICoreTextTextRequestedEventArgs*>(args)->get_Request(&req)) || !req)
				return;
			text_range range{};
			req->get_Range(&range);
			const INT32 start = std::clamp(range.start, 0, text_len());
			const INT32 end = std::clamp(range.end, start, text_len());
			HSTRING str = nullptr;
			if (SUCCEEDED(WindowsCreateString(reinterpret_cast<const wchar_t*>(g_text.data() + start), static_cast<UINT32>(end - start), &str)))
			{
				req->put_Text(str);
				WindowsDeleteString(str);
			}
			req->Release();
		}

		void on_selection_requested(IInspectable* args)
		{
			ICoreTextSelectionRequest* req = nullptr;
			if (FAILED(reinterpret_cast<ICoreTextSelectionRequestedEventArgs*>(args)->get_Request(&req)) || !req)
				return;
			req->put_Selection({text_len(), text_len()});
			req->Release();
		}

		void on_text_updating(IInspectable* args)
		{
			auto* e = reinterpret_cast<ICoreTextTextUpdatingEventArgs*>(args);
			text_range range{};
			HSTRING str = nullptr;
			e->get_Range(&range);
			e->get_Text(&str);
			UINT32 len = 0;
			const wchar_t* raw = str ? WindowsGetStringRawBuffer(str, &len) : nullptr;
			std::u16string typed(reinterpret_cast<const char16_t*>(raw ? raw : L""), len);
			if (str)
				WindowsDeleteString(str);

			// Enter submits the field; it isn't part of the text.
			if (typed.find_first_of(u"\r\n") != umax)
			{
				g_submitted = true;
				e->put_Result(1); // Failed: the text stays as it was
				return;
			}

			bool ok = true;
			for (char16_t& c : typed)
			{
				if (g_transform)
					c = g_transform(c);
				if (g_accept && !g_accept(c))
					ok = false;
			}

			const INT32 start = std::clamp(range.start, 0, text_len());
			const INT32 end = std::clamp(range.end, start, text_len());
			std::u16string next = g_text;
			next.replace(start, end - start, typed);
			if (!ok || next.size() > g_max_len)
			{
				e->put_Result(1);
				return;
			}

			g_text = std::move(next);
			g_changed = true;
			e->put_Result(0);
		}

		bool init()
		{
			if (g_init_tried)
				return g_context && g_pane2;
			g_init_tried = true;

			ICoreTextServicesManagerStatics* statics = nullptr;
			ICoreTextServicesManager* manager = nullptr;
			HRESULT hr = get_factory(L"Windows.UI.Text.Core.CoreTextServicesManager", IID_ICoreTextServicesManagerStatics, &statics);
			if (SUCCEEDED(hr))
				hr = statics->GetForCurrentView(&manager);
			if (SUCCEEDED(hr) && manager)
				hr = manager->CreateEditContext(&g_context);
			if (manager)
				manager->Release();
			if (statics)
				statics->Release();
			if (FAILED(hr) || !g_context)
			{
				text_log.error("No text edit context (0x%08x); the menu's own keyboard is used", static_cast<u32>(hr));
				g_context = nullptr;
				return false;
			}

			g_context->put_InputPaneDisplayPolicy(1); // Manual: shown with TryShow
			event_token token{};
			const auto add = [&](auto add_fn, const GUID& iid, std::function<void(IInspectable*)> fn)
			{
				handler* h = new handler(iid, std::move(fn));
				const HRESULT r = (g_context->*add_fn)(h, &token);
				h->Release();
				return r;
			};
			add(&ICoreTextEditContext::add_TextRequested, IID_TextRequestedHandler, on_text_requested);
			add(&ICoreTextEditContext::add_SelectionRequested, IID_SelectionRequestedHandler, on_selection_requested);
			hr = add(&ICoreTextEditContext::add_TextUpdating, IID_TextUpdatingHandler, on_text_updating);
			add(&ICoreTextEditContext::add_FocusRemoved, IID_FocusRemovedHandler, [](IInspectable*)
			{
				text_log.notice("Edit context lost focus");
				g_pane_hidden = true;
			});
			if (FAILED(hr))
				text_log.error("TextUpdating handler not accepted (0x%08x)", static_cast<u32>(hr));

			IInputPaneStatics* pane_statics = nullptr;
			hr = get_factory(L"Windows.UI.ViewManagement.InputPane", IID_IInputPaneStatics, &pane_statics);
			if (SUCCEEDED(hr))
				hr = pane_statics->GetForCurrentView(&g_pane);
			if (pane_statics)
				pane_statics->Release();
			if (SUCCEEDED(hr) && g_pane)
				hr = g_pane->QueryInterface(IID_IInputPane2, reinterpret_cast<void**>(&g_pane2));
			if (FAILED(hr) || !g_pane2)
			{
				text_log.error("No input pane (0x%08x); the menu's own keyboard is used", static_cast<u32>(hr));
				return false;
			}

			handler* showing = new handler(IID_InputPaneVisibilityHandler, [](IInspectable*)
			{
				text_log.notice("Keyboard showing");
				g_pane_shown = true;
			});
			g_pane->add_Showing(showing, &token);
			showing->Release();
			handler* hiding = new handler(IID_InputPaneVisibilityHandler, [](IInspectable*)
			{
				text_log.notice("Keyboard hiding");
				g_pane_hidden = true;
			});
			g_pane->add_Hiding(hiding, &token);
			hiding->Release();

			text_log.notice("System keyboard ready");
			return true;
		}
	}

	bool show(scope sc, const std::string& text, usz max_len, std::function<bool(char16_t)> accept, std::function<char16_t(char16_t)> transform)
	{
		if (!init())
			return false;

		if (g_active)
			g_context->NotifyFocusLeave();

		const INT32 old_len = text_len();
		g_text = utf8_to_utf16(text);
		g_max_len = max_len;
		g_accept = std::move(accept);
		g_transform = std::move(transform);
		g_changed = false;
		g_submitted = false;
		g_pane_shown = false;
		g_pane_hidden = false;
		g_shown_at = {};

		INT32 input_scope = scope_username;
		switch (sc)
		{
		case scope::username: input_scope = scope_username; break;
		case scope::password: input_scope = scope_password; break;
		case scope::email: input_scope = scope_email; break;
		case scope::url: input_scope = scope_url; break;
		case scope::code: input_scope = scope_alnum; break;
		}
		g_context->put_InputScope(input_scope);
		g_context->NotifyTextChanged({0, old_len}, text_len(), {text_len(), text_len()});
		g_context->NotifyFocusEnter();
		g_active = true;

		boolean shown = false;
		const HRESULT hr = g_pane2->TryShow(&shown);
		text_log.notice("TryShow: 0x%08x, %s", static_cast<u32>(hr), shown ? "shown" : "not shown");
		if (FAILED(hr) || !shown)
		{
			// A USB keyboard still types into the field; the menu shows its own keys.
			return false;
		}
		g_shown_at = std::chrono::steady_clock::now();
		return true;
	}

	void set_text(const std::string& text)
	{
		if (!g_active)
			return;
		const INT32 old_len = text_len();
		g_text = utf8_to_utf16(text);
		g_context->NotifyTextChanged({0, old_len}, text_len(), {text_len(), text_len()});
	}

	void hide()
	{
		if (!g_active)
			return;
		g_active = false;
		g_context->NotifyFocusLeave();
		boolean hidden = false;
		g_pane2->TryHide(&hidden);
	}

	bool keyboard_visible()
	{
		if (!g_active || g_shown_at == std::chrono::steady_clock::time_point{} || g_pane_hidden)
			return false;
		if (g_pane_shown)
			return true;

		// No Showing event (yet): trust TryShow for a moment, then the occluded area.
		if (std::chrono::steady_clock::now() - g_shown_at < std::chrono::seconds(2))
			return true;
		rect_f r{};
		return g_pane && SUCCEEDED(g_pane->get_OccludedRect(&r)) && r.height > 0;
	}

	bool poll(std::string& text, bool& submitted)
	{
		submitted = std::exchange(g_submitted, false);
		if (!g_active || (!std::exchange(g_changed, false) && !submitted))
			return false;
		text = utf16_to_utf8(g_text);
		return true;
	}
}
