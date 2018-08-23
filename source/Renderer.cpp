#include "LearningVulkan/Renderer.hpp"

#include "vulkan/vulkan.hpp"

Renderer::Renderer()
{
}

Renderer::~Renderer()
{
}

void Renderer::initialize(uint32_t width, uint32_t height)
{
	VkApplicationInfo applicationInfo = {};
	applicationInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	applicationInfo.pNext = nullptr;
	applicationInfo.pApplicationName = "Learning Vulkan";
	applicationInfo.pEngineName = "Unknown";
	applicationInfo.apiVersion = VK_MAKE_VERSION(1, 1, 82);
	applicationInfo.engineVersion = 1;
}
