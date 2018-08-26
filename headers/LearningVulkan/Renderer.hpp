#pragma once

#include <cstdint>
#include <Windows.h>
#include "vulkan/vulkan.hpp"

struct VulkanContext
{
	uint32_t width;
	uint32_t height;

	VkInstance instance;

	VkSurfaceKHR surface;

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