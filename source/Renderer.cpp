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

	// Find a suitable physical device (for now, just use the first one that
	// supports rendering)
	uint32_t physicalDeviceCount = 0;
	vkEnumeratePhysicalDevices(context.instance, &physicalDeviceCount, nullptr);

	assert(physicalDeviceCount != 0,
		"Failed to find any physical devices on this machine.");

	VkPhysicalDevice *physicalDevices = new VkPhysicalDevice[physicalDeviceCount];
	vkEnumeratePhysicalDevices(context.instance, &physicalDeviceCount, physicalDevices);
	
	for (uint32_t i = 0; i < physicalDeviceCount; ++i)
	{
		// Get the properties of this physical device
		VkPhysicalDeviceProperties physicalDeviceProperties = {};
		vkGetPhysicalDeviceProperties(
			physicalDevices[i],
			&physicalDeviceProperties);

		uint32_t queueFamilyCount = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(
			physicalDevices[i],
			&queueFamilyCount,
			nullptr);

		VkQueueFamilyProperties *queueFamilyProperties = new VkQueueFamilyProperties[queueFamilyCount];
		vkGetPhysicalDeviceQueueFamilyProperties(
			physicalDevices[i],
			&queueFamilyCount,
			queueFamilyProperties);

		// Check whether at least one of the queue families supports presenting
		for (uint32_t j = 0; j < queueFamilyCount; ++j)
		{
			VkBool32 supportsPresent = false;
			vkGetPhysicalDeviceSurfaceSupportKHR(
				physicalDevices[i],
				j,
				context.surface,
				&supportsPresent);

			if (supportsPresent &&
				(queueFamilyProperties[j].queueFlags & VK_QUEUE_GRAPHICS_BIT))
			{
				context.physicalDevice = physicalDevices[i];
				context.physicalDeviceProperties = physicalDeviceProperties;
				context.presentQueueIndex = j;
				break;
			}
		}

		delete[] queueFamilyProperties;

		// A physical device has already been found, no need to loop again
		if (context.physicalDevice)
			break;
	}

	delete[] physicalDevices;

	assert(context.physicalDevice,
		"Failed to detect any physical device that can render and present.");

	// Information for accessing one of the rendering queues of this device
	VkDeviceQueueCreateInfo queueCreateInfo = {};
	queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	queueCreateInfo.queueFamilyIndex = context.presentQueueIndex;
	queueCreateInfo.queueCount = 1;
	float queuePriorities[] = { 1.0f };	// Ask for the highest priority (0 to 1)
	queueCreateInfo.pQueuePriorities = queuePriorities;

	// Logical device information
	VkDeviceCreateInfo deviceCreateInfo = {};
	deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	deviceCreateInfo.queueCreateInfoCount = 1;
	deviceCreateInfo.pQueueCreateInfos = &queueCreateInfo;
	deviceCreateInfo.enabledLayerCount = 1;
	deviceCreateInfo.ppEnabledLayerNames = layers;

	// Swap chain extension is required
	const char *deviceExtensions[] = { "VK_KHR_swapchain" };
	deviceCreateInfo.enabledExtensionCount = 1;
	deviceCreateInfo.ppEnabledExtensionNames = deviceExtensions;

	VkPhysicalDeviceFeatures physicalDeviceFeatures = {};
	physicalDeviceFeatures.shaderClipDistance = VK_TRUE;

	deviceCreateInfo.pEnabledFeatures = &physicalDeviceFeatures;

	// Create the logical device
	result = vkCreateDevice(
		context.physicalDevice,
		&deviceCreateInfo,
		nullptr,
		&context.device);

	Utility::checkVulkanResult(result, "Failed to create a logical device.");
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
