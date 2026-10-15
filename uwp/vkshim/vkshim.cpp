// Minimal stand-in for the Vulkan loader (vulkan-1.dll) on UWP: loads Mesa's Dozen
// driver from the package and forwards every vk* call to it. There is a single driver,
// so no layers or ICD discovery; instance and device functions are resolved through
// vkGetInstanceProcAddr once the instance exists (Mesa returns dispatching trampolines).

#include <windows.h>
#include <vulkan/vulkan.h>
#include <cstring>
#include <vector>

extern "C"
{
	extern const char* const uwp_vk_names[];
	extern const size_t uwp_vk_count;
	extern void* uwp_vk_ptrs[];
	HMODULE WINAPI LoadPackagedLibrary(LPCWSTR name, DWORD reserved);

	// Set by the app to receive driver errors/warnings (Mesa reports its source file:line as the prefix).
	void (*uwp_vkshim_log)(const char* where, const char* text) = nullptr;
}

namespace
{
	PFN_vkGetInstanceProcAddr g_gipa = nullptr;

	bool load_driver()
	{
		if (g_gipa)
			return true;

		HMODULE mod = LoadPackagedLibrary(L"vulkan_dzn.dll", 0);
		if (!mod)
			return false;

		g_gipa = reinterpret_cast<PFN_vkGetInstanceProcAddr>(GetProcAddress(mod, "vk_icdGetInstanceProcAddr"));
		return g_gipa != nullptr;
	}

	void resolve_all(VkInstance instance)
	{
		for (size_t i = 0; i < uwp_vk_count; i++)
		{
			if (auto fn = g_gipa(instance, uwp_vk_names[i]))
				uwp_vk_ptrs[i] = reinterpret_cast<void*>(fn);
		}
	}

	template <typename F>
	F global_fn(const char* name)
	{
		return load_driver() ? reinterpret_cast<F>(g_gipa(nullptr, name)) : nullptr;
	}
}

extern "C"
{
	VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL vkGetInstanceProcAddr(VkInstance instance, const char* name)
	{
		return load_driver() ? g_gipa(instance, name) : nullptr;
	}

	VKAPI_ATTR VkResult VKAPI_CALL vkEnumerateInstanceExtensionProperties(const char* layer, uint32_t* count, VkExtensionProperties* props)
	{
		const auto fn = global_fn<PFN_vkEnumerateInstanceExtensionProperties>("vkEnumerateInstanceExtensionProperties");
		return fn ? fn(layer, count, props) : VK_ERROR_INITIALIZATION_FAILED;
	}

	VKAPI_ATTR VkResult VKAPI_CALL vkEnumerateInstanceLayerProperties(uint32_t* count, VkLayerProperties*)
	{
		*count = 0; // No layers without the loader
		return VK_SUCCESS;
	}

	VKAPI_ATTR VkResult VKAPI_CALL vkEnumerateInstanceVersion(uint32_t* version)
	{
		const auto fn = global_fn<PFN_vkEnumerateInstanceVersion>("vkEnumerateInstanceVersion");
		if (!fn)
		{
			*version = VK_API_VERSION_1_0;
			return VK_SUCCESS;
		}
		return fn(version);
	}

	VKAPI_ATTR VkResult VKAPI_CALL vkCreateInstance(const VkInstanceCreateInfo* info, const VkAllocationCallbacks* alloc, VkInstance* instance)
	{
		const auto fn = global_fn<PFN_vkCreateInstance>("vkCreateInstance");
		if (!fn)
			return VK_ERROR_INCOMPATIBLE_DRIVER;

		// Always enable VK_EXT_debug_report so driver errors reach the app log; there are no layers.
		std::vector<const char*> exts(info->ppEnabledExtensionNames, info->ppEnabledExtensionNames + info->enabledExtensionCount);
		bool has_report = false;
		for (const char* e : exts)
			has_report |= std::strcmp(e, VK_EXT_DEBUG_REPORT_EXTENSION_NAME) == 0;
		if (!has_report)
			exts.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);

		VkInstanceCreateInfo patched = *info;
		patched.enabledLayerCount = 0;
		patched.ppEnabledLayerNames = nullptr;
		patched.enabledExtensionCount = static_cast<uint32_t>(exts.size());
		patched.ppEnabledExtensionNames = exts.data();

		const VkResult res = fn(&patched, alloc, instance);
		if (res != VK_SUCCESS)
			return res;

		resolve_all(*instance);

		VkDebugReportCallbackCreateInfoEXT cb{};
		cb.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
		cb.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT | VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT;
		cb.pfnCallback = [](VkDebugReportFlagsEXT, VkDebugReportObjectTypeEXT, uint64_t, size_t, int32_t, const char* prefix, const char* msg, void*) -> VkBool32
		{
			if (uwp_vkshim_log)
				uwp_vkshim_log(prefix ? prefix : "", msg ? msg : "");
			return VK_FALSE;
		};
		if (auto create = reinterpret_cast<PFN_vkCreateDebugReportCallbackEXT>(g_gipa(*instance, "vkCreateDebugReportCallbackEXT")))
		{
			VkDebugReportCallbackEXT handle;
			create(*instance, &cb, nullptr, &handle); // Lives as long as the instance
		}
		return res;
	}
}
