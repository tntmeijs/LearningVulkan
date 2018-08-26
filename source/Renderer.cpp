#include "LearningVulkan/Renderer.hpp"
#include "LearningVulkan/Utility.hpp"

#include "vulkan/vulkan.hpp"
#include <Windows.h>

// Extensions
PFN_vkCreateDebugReportCallbackEXT fpVkCreateDebugReportCallbackEXT = nullptr;
PFN_vkDestroyDebugReportCallbackEXT fpVkDestroyDebugReportCallbackEXT = nullptr;
PFN_vkDebugReportMessageEXT fpVkDebugReportMessageEXT = nullptr;
PFN_vkCreateWin32SurfaceKHR fpVkCreateWin32SurfaceKHR = nullptr;

// Callback for the debug report extension
VKAPI_ATTR VkBool32 VKAPI_CALL debugReportCallback(
	VkDebugReportFlagsEXT flags,
	VkDebugReportObjectTypeEXT objectType,
	uint64_t object,
	size_t location,
	int32_t messageCode,
	const char *pLayerPrefix,
	const char *pMessage,
	void *pUserData)
{
	printf("%s\t%s\n", pLayerPrefix, pMessage);
	return VK_FALSE;
}

Renderer::Renderer()
{
}

Renderer::~Renderer()
{
	fpVkDestroyDebugReportCallbackEXT(
		context.instance,
		context.debugCallback,
		nullptr);
	
	vkDestroyInstance(context.instance, nullptr);
}

void Renderer::initialize(uint32_t width, uint32_t height, HWND windowHandle)
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

	// Get the number of supported extensions
	uint32_t extensionCount = 0;
	vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);

	assert(extensionCount != 0, "Failed to find any extensions on this system.");
	VkExtensionProperties *availableExtensions = new VkExtensionProperties[extensionCount];
	vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, availableExtensions);
	
	// Extensions that are needed for this application
	const char *extensions[] = { "VK_KHR_surface", "VK_KHR_win32_surface", "VK_EXT_debug_report" };
	const uint32_t requiredNumberOfExtensions = sizeof(extensions) / sizeof(char *);
	uint32_t numberOfExtensionsFound = 0;

	for (uint32_t i = 0; i < extensionCount; ++i)
	{
		for (uint32_t j = 0; j < requiredNumberOfExtensions; ++j)
		// Found one of the required extensions
		if (strcmp(availableExtensions[i].extensionName, extensions[j]) == 0)
		{
			++numberOfExtensionsFound;
		}
	}
	
	assert(numberOfExtensionsFound == requiredNumberOfExtensions, "Failed to find all required extensions.");

	delete[] availableExtensions;

	instanceCreateInfo.enabledExtensionCount = requiredNumberOfExtensions;
	instanceCreateInfo.ppEnabledExtensionNames = extensions;

	VkResult result;

	// Create the Vulkan instance and check for errors
	result = vkCreateInstance(&instanceCreateInfo, nullptr, &context.instance);
	Utility::checkVulkanResult(result, "Failed to create a Vulkan instance.");

	// Load the extensions that were checked for above
	loadExtensions();

	// Setup the debug callback
	VkDebugReportCallbackCreateInfoEXT callbackCreateInfoEXT = {};
	callbackCreateInfoEXT.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CREATE_INFO_EXT;
	callbackCreateInfoEXT.flags =	VK_DEBUG_REPORT_ERROR_BIT_EXT |
									VK_DEBUG_REPORT_WARNING_BIT_EXT |
									VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT;
	callbackCreateInfoEXT.pfnCallback = &debugReportCallback;
	callbackCreateInfoEXT.pUserData = nullptr;

	result = fpVkCreateDebugReportCallbackEXT(
		context.instance,
		&callbackCreateInfoEXT,
		nullptr,
		&context.debugCallback);

	Utility::checkVulkanResult(
		result,
		"Failed to create the debug report extension callback.");

	// Create a Windows surface
	VkWin32SurfaceCreateInfoKHR surfaceCreateInfo = {};
	surfaceCreateInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
	surfaceCreateInfo.hinstance = GetModuleHandle(0);
	surfaceCreateInfo.hwnd = windowHandle;

	result = vkCreateWin32SurfaceKHR(
		context.instance,
		&surfaceCreateInfo,
		nullptr,
		&context.surface);

	Utility::checkVulkanResult(result, "Failed to create a Windows surface.");
}

void Renderer::loadExtensions()
{
	PFN_vkVoidFunction functionPointer = nullptr;
	
	functionPointer = vkGetInstanceProcAddr(context.instance, "vkCreateDebugReportCallbackEXT");
	assert(functionPointer != nullptr, "Failed to load the \"vkCreateDebugReportCallbackEXT\" extension.");
	fpVkCreateDebugReportCallbackEXT = reinterpret_cast<PFN_vkCreateDebugReportCallbackEXT>(functionPointer);
	functionPointer = nullptr;

	functionPointer = vkGetInstanceProcAddr(context.instance, "vkDestroyDebugReportCallbackEXT");
	assert(functionPointer != nullptr, "Failed to load the \"vkDestroyDebugReportCallbackEXT\" extension.");
	fpVkDestroyDebugReportCallbackEXT = reinterpret_cast<PFN_vkDestroyDebugReportCallbackEXT>(functionPointer);
	functionPointer = nullptr;

	functionPointer = vkGetInstanceProcAddr(context.instance, "vkDebugReportMessageEXT");
	assert(functionPointer != nullptr, "Failed to load the \"vkDebugReportMessageEXT\" extension.");
	fpVkDebugReportMessageEXT = reinterpret_cast<PFN_vkDebugReportMessageEXT>(functionPointer);
	functionPointer = nullptr;

	functionPointer = vkGetInstanceProcAddr(context.instance, "vkCreateWin32SurfaceKHR");
	assert(functionPointer != nullptr, "Failed to load the \"vkCreateWin32SurfaceKHR\" extension.");
	fpVkCreateWin32SurfaceKHR = reinterpret_cast<PFN_vkCreateWin32SurfaceKHR>(functionPointer);
	functionPointer = nullptr;
}
