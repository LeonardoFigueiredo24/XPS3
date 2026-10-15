// Minimal stand-in for the Vulkan loader (vulkan-1.dll) on UWP: loads Mesa's Dozen
// driver from the package and forwards every vk* call to it. There is a single driver,
// so no layers or ICD discovery; instance and device functions are resolved through
// vkGetInstanceProcAddr once the instance exists (Mesa returns dispatching trampolines).

#include <windows.h>
#include <vulkan/vulkan.h>
#include <cstring>

extern "C"
{
	extern const char* const uwp_vk_names[];
	extern const size_t uwp_vk_count;
	extern void* uwp_vk_ptrs[];
	HMODULE WINAPI LoadPackagedLibrary(LPCWSTR name, DWORD reserved);
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

		const VkResult res = fn(info, alloc, instance);
		if (res == VK_SUCCESS)
			resolve_all(*instance);
		return res;
	}
}
