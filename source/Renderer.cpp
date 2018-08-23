#include "LearningVulkan/Renderer.hpp"
#include "LearningVulkan/Utility.hpp"

#include "vulkan/vulkan.hpp"

Renderer::Renderer()
{
}

Renderer::~Renderer()
{
}

void Renderer::initialize(uint32_t width, uint32_t height)
{
	// General information about this application
	VkApplicationInfo applicationInfo = {};
	applicationInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	applicationInfo.pNext = nullptr;
	applicationInfo.pApplicationName = "Learning Vulkan";
	applicationInfo.pEngineName = "Unknown";
	applicationInfo.apiVersion = VK_MAKE_VERSION(1, 1, 82);
	applicationInfo.engineVersion = 1;

	// Information needed to create the Vulkan instance
	VkInstanceCreateInfo instanceCreateInfo = {};
	instanceCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	instanceCreateInfo.pApplicationInfo = &applicationInfo;
	instanceCreateInfo.enabledLayerCount = 0;
	instanceCreateInfo.ppEnabledLayerNames = nullptr;
	instanceCreateInfo.enabledExtensionCount = 0;
	instanceCreateInfo.ppEnabledExtensionNames = nullptr;

#if defined(_DEBUG)
	// Get the number of supported validation layers
	uint32_t layerCount = 0;
	vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

	assert(layerCount != 0, "Failed to find any validation layers on this system.");

	VkLayerProperties *availableLayers = new VkLayerProperties[layerCount];
	vkEnumerateInstanceLayerProperties(&layerCount, availableLayers);

	bool foundValidationLayer = false;
	for (uint32_t i = 0; i < layerCount; ++i)
	{
		// Look for the LunarG validation layer
		if (strcmp(availableLayers[i].layerName, "VK_LAYER_LUNARG_standard_validation") == 0)
		{
			foundValidationLayer = true;
		}
	}

	assert(foundValidationLayer, "Failed to find the \"VK_LAYER_LUNARG_standard_validation\" validation layer.");

	const char *layers[] = { "VK_LAYER_LUNARG_standard_validation" };

	delete[] availableLayers;

	instanceCreateInfo.enabledLayerCount = 1;
	instanceCreateInfo.ppEnabledLayerNames = layers;
#endif

	// Create the Vulkan instance and check for errors
	VkResult result = vkCreateInstance(&instanceCreateInfo, nullptr, &context.instance);
	Utility::checkVulkanResult(result, "Failed to create a Vulkan instance.");
}
