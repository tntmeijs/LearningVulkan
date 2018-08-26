#pragma once

#include <cstdint>
#include "vulkan/vulkan.hpp"

struct VulkanContext
{
	uint32_t width;
	uint32_t height;

	VkInstance instance;
	VkDebugReportCallbackEXT debugCallback;
};

class Renderer
{
public:
	Renderer();
	~Renderer();

	void initialize(uint32_t width, uint32_t height);

private:
	void loadExtensions();

private:
	VulkanContext context;
};