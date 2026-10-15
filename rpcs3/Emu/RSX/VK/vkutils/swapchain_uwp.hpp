#pragma once

#include "swapchain_core.h"

// UWP has no Vulkan window-system integration (Dozen only presents to HWNDs). Frames are
// copied to host memory like the other native swapchains and handed to the app shell,
// which shows them in its own CoreWindow swapchain.
extern void uwp_get_display_size(u32& width, u32& height);
extern void uwp_present_frame(const void* bgra_pixels, u32 width, u32 height);

namespace vk
{
	class swapchain_UWP : public native_swapchain_base
	{
	public:
		using native_swapchain_base::native_swapchain_base;

		bool init() override
		{
			if (!swapchain_images.empty())
				destroy(false);

			uwp_get_display_size(m_width, m_height);
			init_swapchain_images(dev, 3);
			return true;
		}

		void create(display_handle_t& handle) override
		{
			window_handle = handle;
		}

		void destroy(bool full = true) override
		{
			swapchain_images.clear();

			if (full)
				dev.destroy();
		}

		VkResult present(VkSemaphore /*semaphore*/, u32 image) override
		{
			auto& src = swapchain_images[image];
			uwp_present_frame(src.second->get_pixels(), m_width, m_height);
			src.second->free_pixels();
			src.first = false;
			return VK_SUCCESS;
		}
	};

	using swapchain_NATIVE = swapchain_UWP;

	[[maybe_unused]] static
	VkSurfaceKHR make_WSI_surface(VkInstance, display_handle_t, WSI_config*)
	{
		return VK_NULL_HANDLE;
	}
}
