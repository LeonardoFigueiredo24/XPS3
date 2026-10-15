#pragma once

// The handful of WinRT ABI interfaces the UWP shell needs. MinGW has no Windows SDK
// WinRT headers, so these are declared by hand. Vtable order and IIDs match the
// Windows SDK (windows.applicationmodel.core.h, windows.ui.core.h, windows.storage.h,
// windows.system.h). Unused methods are placeholder slots.

#include <windows.h>
#include <inspectable.h>
#include <roapi.h>
#include <winstring.h>

namespace winrt_min
{
#define WINRT_MIN_SLOT(n) virtual HRESULT STDMETHODCALLTYPE slot_##n() = 0

	struct ICoreDispatcher : IInspectable
	{
		WINRT_MIN_SLOT(get_HasThreadAccess);
		virtual HRESULT STDMETHODCALLTYPE ProcessEvents(int options) = 0; // CoreProcessEventsOption
		WINRT_MIN_SLOT(RunAsync);
		WINRT_MIN_SLOT(RunIdleAsync);
	};

	struct ICoreWindow : IInspectable
	{
		WINRT_MIN_SLOT(get_AutomationHostProvider);
		WINRT_MIN_SLOT(get_Bounds);
		WINRT_MIN_SLOT(get_CustomProperties);
		virtual HRESULT STDMETHODCALLTYPE get_Dispatcher(ICoreDispatcher** value) = 0;
		WINRT_MIN_SLOT(get_FlowDirection);
		WINRT_MIN_SLOT(put_FlowDirection);
		WINRT_MIN_SLOT(get_IsInputEnabled);
		WINRT_MIN_SLOT(put_IsInputEnabled);
		WINRT_MIN_SLOT(get_PointerCursor);
		WINRT_MIN_SLOT(put_PointerCursor);
		WINRT_MIN_SLOT(get_PointerPosition);
		WINRT_MIN_SLOT(get_Visible);
		virtual HRESULT STDMETHODCALLTYPE Activate() = 0;
		// Remaining methods are never called.
	};

	struct IFrameworkView : IInspectable
	{
		virtual HRESULT STDMETHODCALLTYPE Initialize(IInspectable* application_view) = 0;
		virtual HRESULT STDMETHODCALLTYPE SetWindow(ICoreWindow* window) = 0;
		virtual HRESULT STDMETHODCALLTYPE Load(HSTRING entry_point) = 0;
		virtual HRESULT STDMETHODCALLTYPE Run() = 0;
		virtual HRESULT STDMETHODCALLTYPE Uninitialize() = 0;
	};

	struct IFrameworkViewSource : IInspectable
	{
		virtual HRESULT STDMETHODCALLTYPE CreateView(IFrameworkView** view) = 0;
	};

	struct ICoreApplication : IInspectable
	{
		WINRT_MIN_SLOT(get_Id);
		WINRT_MIN_SLOT(add_Suspending);
		WINRT_MIN_SLOT(remove_Suspending);
		WINRT_MIN_SLOT(add_Resuming);
		WINRT_MIN_SLOT(remove_Resuming);
		WINRT_MIN_SLOT(get_Properties);
		WINRT_MIN_SLOT(GetCurrentView);
		virtual HRESULT STDMETHODCALLTYPE Run(IFrameworkViewSource* source) = 0;
		WINRT_MIN_SLOT(RunWithActivationFactories);
	};

	struct IStorageItem : IInspectable
	{
		WINRT_MIN_SLOT(RenameAsyncOverloadDefaultOptions);
		WINRT_MIN_SLOT(RenameAsync);
		WINRT_MIN_SLOT(DeleteAsyncOverloadDefaultOptions);
		WINRT_MIN_SLOT(DeleteAsync);
		WINRT_MIN_SLOT(GetBasicPropertiesAsync);
		WINRT_MIN_SLOT(get_Name);
		virtual HRESULT STDMETHODCALLTYPE get_Path(HSTRING* value) = 0;
		WINRT_MIN_SLOT(get_Attributes);
		WINRT_MIN_SLOT(get_DateCreated);
		WINRT_MIN_SLOT(IsOfType);
	};

	struct IApplicationData : IInspectable
	{
		WINRT_MIN_SLOT(get_Version);
		WINRT_MIN_SLOT(SetVersionAsync);
		WINRT_MIN_SLOT(ClearAllAsync);
		WINRT_MIN_SLOT(ClearAsync);
		WINRT_MIN_SLOT(get_LocalSettings);
		WINRT_MIN_SLOT(get_RoamingSettings);
		virtual HRESULT STDMETHODCALLTYPE get_LocalFolder(IInspectable** value) = 0; // StorageFolder
		WINRT_MIN_SLOT(get_RoamingFolder);
		WINRT_MIN_SLOT(get_TemporaryFolder);
	};

	struct IApplicationDataStatics : IInspectable
	{
		virtual HRESULT STDMETHODCALLTYPE get_Current(IApplicationData** value) = 0;
	};

	struct IMemoryManagerStatics : IInspectable
	{
		virtual HRESULT STDMETHODCALLTYPE get_AppMemoryUsage(UINT64* value) = 0;
		virtual HRESULT STDMETHODCALLTYPE get_AppMemoryUsageLimit(UINT64* value) = 0;
	};

#undef WINRT_MIN_SLOT

	// {0aacf7a4-5e1d-49df-8034-fb6a68bc5ed1}
	inline constexpr GUID IID_ICoreApplication{0x0aacf7a4, 0x5e1d, 0x49df, {0x80, 0x34, 0xfb, 0x6a, 0x68, 0xbc, 0x5e, 0xd1}};
	// {cd770614-65c4-426c-9494-34fc43554862}
	inline constexpr GUID IID_IFrameworkViewSource{0xcd770614, 0x65c4, 0x426c, {0x94, 0x94, 0x34, 0xfc, 0x43, 0x55, 0x48, 0x62}};
	// {faab5cd0-8924-45ac-ad0f-a08fae5d0324}
	inline constexpr GUID IID_IFrameworkView{0xfaab5cd0, 0x8924, 0x45ac, {0xad, 0x0f, 0xa0, 0x8f, 0xae, 0x5d, 0x03, 0x24}};
	// {5612147b-e843-45e3-94d8-06169e3c8e17}
	inline constexpr GUID IID_IApplicationDataStatics{0x5612147b, 0xe843, 0x45e3, {0x94, 0xd8, 0x06, 0x16, 0x9e, 0x3c, 0x8e, 0x17}};
	// {4207a996-ca2f-42f7-bde8-8b10457a7f30}
	inline constexpr GUID IID_IStorageItem{0x4207a996, 0xca2f, 0x42f7, {0xbd, 0xe8, 0x8b, 0x10, 0x45, 0x7a, 0x7f, 0x30}};
	// {5c6c279c-d7ca-4779-9188-4057219ce64c}
	inline constexpr GUID IID_IMemoryManagerStatics{0x5c6c279c, 0xd7ca, 0x4779, {0x91, 0x88, 0x40, 0x57, 0x21, 0x9c, 0xe6, 0x4c}};

	// Activation factory lookup for a runtime class name.
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
}
