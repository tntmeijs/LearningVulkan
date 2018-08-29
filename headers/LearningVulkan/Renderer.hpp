#pragma once

#include <cstdint>
#include <Windows.h>
#include "vulkan/vulkan.hpp"

struct VulkanContext
{
	uint32_t width;
	uint32_t height;
	uint32_t presentQueueIndex;

	VkInstance instance;
	VkPhysicalDevice physicalDevice;
	VkDevice device;
	VkPhysicalDeviceProperties physicalDeviceProperties;
	VkSurfaceKHR surface;
	VkSwapchainKHR swapChain;
	VkDebugReportCallbackEXT debugCallback;
};

class Renderer
{
public:
	Renderer();
	~Renderer();

	void initialize(uint32_t width, uint32_t height, HWND windowHandle);

private:
	void loadExtensions();

private:
	VulkanContext context;
};