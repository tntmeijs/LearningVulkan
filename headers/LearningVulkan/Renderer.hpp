#pragma once

#include <cstdint>
#include "vulkan/vulkan.hpp"

struct VulkanContext
{
	uint32_t width;
	uint32_t height;

	VkInstance instance;
};

class Renderer
{
public:
	Renderer();
	~Renderer();

	void initialize(uint32_t width, uint32_t height);

private:
	VulkanContext context;
};