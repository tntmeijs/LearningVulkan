#include "LearningVulkan/Renderer.hpp"
#include "LearningVulkan/Utility.hpp"

#include "vulkan/vulkan.hpp"
#include <Windows.h>
#include <assert.h>

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
	delete[] context.framebuffers;
	delete[] context.presentImages;
	
	fpVkDestroyDebugReportCallbackEXT(
		context.instance,
		context.debugCallback,
		nullptr);
	
	vkDestroyInstance(context.instance, nullptr);
}

void Renderer::initialize(uint32_t width, uint32_t height, HWND windowHandle)
{
	// Save the width and height for later use
	context.width = width;
	context.height = height;

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

	assert(layerCount != 0,
		"Failed to find any validation layers on this system.");

	VkLayerProperties *availableLayers = new VkLayerProperties[layerCount];
	vkEnumerateInstanceLayerProperties(&layerCount, availableLayers);

	bool foundValidationLayer = false;
	for (uint32_t i = 0; i < layerCount; ++i)
	{
		// Look for the LunarG validation layer
		if (strcmp(availableLayers[i].layerName,
			"VK_LAYER_LUNARG_standard_validation") == 0)
		{
			foundValidationLayer = true;
		}
	}

	assert(foundValidationLayer,
		"Failed to find the \"VK_LAYER_LUNARG_standard_validation\"validation layer.");

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
	const char *extensions[] =
	{
		"VK_KHR_surface",
		"VK_KHR_win32_surface",
		"VK_EXT_debug_report"
	};
	const uint32_t requiredNumberOfExtensions = sizeof(extensions) / sizeof(char *);
	uint32_t numberOfExtensionsFound = 0;

	for (uint32_t i = 0; i < extensionCount; ++i)
	{
		for (auto & extension : extensions)
		// Found one of the required extensions
		if (strcmp(availableExtensions[i].extensionName, extension) == 0)
		{
			++numberOfExtensionsFound;
		}
	}
	
	assert(numberOfExtensionsFound == requiredNumberOfExtensions,
		"Failed to find all required extensions.");

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
	surfaceCreateInfo.hinstance = GetModuleHandle(nullptr);
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

	auto *physicalDevices = new VkPhysicalDevice[physicalDeviceCount];
	vkEnumeratePhysicalDevices(
		context.instance,
		&physicalDeviceCount,
		physicalDevices);
	
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

	// Fill up the physical device memory properties
	vkGetPhysicalDeviceMemoryProperties(
		context.physicalDevice,
		&context.physicalDeviceMemoryProperties);

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

	// Set the color format and color space for the swap chain (created down below)
	uint32_t colorFormatCount = 0;
	vkGetPhysicalDeviceSurfaceFormatsKHR(
		context.physicalDevice,
		context.surface,
		&colorFormatCount,
		nullptr);

	assert(colorFormatCount != 0, "Failed to find any color formats.");

	VkSurfaceFormatKHR *surfaceFormats = new VkSurfaceFormatKHR[colorFormatCount];
	vkGetPhysicalDeviceSurfaceFormatsKHR(
		context.physicalDevice,
		context.surface,
		&colorFormatCount,
		surfaceFormats);

	VkFormat surfaceColorFormat;
	VkColorSpaceKHR surfaceColorSpace;

	// If the array of formats only contain one entry of VK_FORMAT_UNDEFINED,
	// it means that the surface has no preferred formats
	if (colorFormatCount == 0 &&
		surfaceFormats[0].format == VK_FORMAT_UNDEFINED)
	{
		// Use this as the default format
		surfaceColorFormat = VK_FORMAT_R8G8B8A8_UNORM;
	}
	else
	{
		// Use whatever format the surface prefers
		surfaceColorFormat = surfaceFormats[0].format;
	}

	// Use the first available color space
	surfaceColorSpace = surfaceFormats[0].colorSpace;

	delete[] surfaceFormats;

	VkSurfaceCapabilitiesKHR surfaceCapabilities = {};
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(
		context.physicalDevice,
		context.surface,
		&surfaceCapabilities);

	// If surfaceCapabilities.maxImageCount == 0, then there is no limit on the
	// number of images (no idea why you would ever need 4+ images, though...)
	uint32_t desiredImageCount = 2;	// Double-buffering
	
	// Adjust the desired image count if the current value is not supported
	if (desiredImageCount < surfaceCapabilities.minImageCount)
	{
		// Cannot use less than the minimum number of images supported
		desiredImageCount = surfaceCapabilities.minImageCount;
	}
	else if (surfaceCapabilities.maxImageCount != 0 &&
			 desiredImageCount > surfaceCapabilities.maxImageCount)
	{
		// Cannot use more than the maximum number of images supported
		desiredImageCount = surfaceCapabilities.maxImageCount;
	}

	// If surfaceCapabilities.currentExtent has -1 for either the width or
	// height, it means that the values can be set to any value. Otherwise,
	// those non-zero values will have to be matched exactly.
	VkExtent2D surfaceResolution = surfaceCapabilities.currentExtent;

	if (surfaceResolution.width == -1 ||
		surfaceResolution.height == -1)
	{
		surfaceResolution.width = context.width;
		surfaceResolution.height = context.height;
	}
	else
	{
		// Surface resolution has to be matched exactly therefore, the
		// dimensions of the context have to be updated
		context.width = surfaceResolution.width;
		context.height = surfaceResolution.height;
	}

	VkSurfaceTransformFlagBitsKHR preTransform = surfaceCapabilities.currentTransform;

	// No surface rotation
	if (surfaceCapabilities.supportedTransforms &
		VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR)
	{
		preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
	}

	// Get the supported present modes
	uint32_t presentModeCount = 0;
	vkGetPhysicalDeviceSurfacePresentModesKHR(
		context.physicalDevice,
		context.surface,
		&presentModeCount,
		nullptr);

	auto *presentModes = new VkPresentModeKHR[presentModeCount];
	vkGetPhysicalDeviceSurfacePresentModesKHR(
		context.physicalDevice,
		context.surface,
		&presentModeCount,
		presentModes);

	// This is the default value that MUST be supported according to the Vulkan
	// specification
	VkPresentModeKHR presentationMode = VK_PRESENT_MODE_FIFO_KHR;

	// VK_PRESENT_MODE_MAILBOX_KHR is preferred, so use that if it is supported
	for (uint32_t i = 0; i < presentModeCount; ++i)
	{
		if (presentModes[i] == VK_PRESENT_MODE_MAILBOX_KHR)
		{
			presentationMode = VK_PRESENT_MODE_MAILBOX_KHR;
			break;
		}
	}
	
	delete[] presentModes;

	// Create the swap chain
	VkSwapchainCreateInfoKHR swapChainCreateInfo = {};
	swapChainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	swapChainCreateInfo.surface = context.surface;
	swapChainCreateInfo.minImageCount = desiredImageCount;
	swapChainCreateInfo.imageFormat = surfaceColorFormat;
	swapChainCreateInfo.imageColorSpace = surfaceColorSpace;
	swapChainCreateInfo.imageExtent = surfaceResolution;
	swapChainCreateInfo.imageArrayLayers = 1;
	swapChainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	swapChainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	swapChainCreateInfo.preTransform = preTransform;
	swapChainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	swapChainCreateInfo.presentMode = presentationMode;
	swapChainCreateInfo.clipped = VK_TRUE;

	result = vkCreateSwapchainKHR(
		context.device,
		&swapChainCreateInfo,
		nullptr,
		&context.swapChain);

	Utility::checkVulkanResult(result, "Failed to create the swap chain.");

	// Get a handle to the present queue of this device
	vkGetDeviceQueue(
		context.device,
		context.presentQueueIndex,
		0,
		&context.presentQueue);

	VkCommandPoolCreateInfo commandPoolCreateInfo = {};
	commandPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	commandPoolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	commandPoolCreateInfo.queueFamilyIndex = context.presentQueueIndex;

	// Create the command pool (used to allocate command buffers)
	VkCommandPool commandPool;
	result = vkCreateCommandPool(
		context.device,
		&commandPoolCreateInfo,
		nullptr,
		&commandPool);

	Utility::checkVulkanResult(result, "Failed to create the command pool.");

	VkCommandBufferAllocateInfo commandBufferAllocationInfo = {};
	commandBufferAllocationInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	commandBufferAllocationInfo.commandPool = commandPool;
	commandBufferAllocationInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	commandBufferAllocationInfo.commandBufferCount = 1;

	// Create the setup command buffer
	result = vkAllocateCommandBuffers(
		context.device,
		&commandBufferAllocationInfo,
		&context.setupCommandBuffer);

	Utility::checkVulkanResult(
		result,
		"Failed to allocate the setup command buffer.");

	// Create the draw command buffer
	result = vkAllocateCommandBuffers(
		context.device,
		&commandBufferAllocationInfo,
		&context.drawCommandBuffer);

	Utility::checkVulkanResult(
		result,
		"Failed to allocate the draw command buffer");

	// Retrieve the swap chain images and store them for later use
	uint32_t imageCount = 0;
	vkGetSwapchainImagesKHR(
		context.device,
		context.swapChain,
		&imageCount,
		nullptr);

	context.presentImages = new VkImage[imageCount];
	vkGetSwapchainImagesKHR(
		context.device,
		context.swapChain,
		&imageCount,
		context.presentImages);

	VkImageViewCreateInfo presentImagesViewCreateInfo = {};
	presentImagesViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	presentImagesViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	presentImagesViewCreateInfo.format = surfaceColorFormat;
	presentImagesViewCreateInfo.components =
	{
		VK_COMPONENT_SWIZZLE_R,
		VK_COMPONENT_SWIZZLE_G,
		VK_COMPONENT_SWIZZLE_B,
		VK_COMPONENT_SWIZZLE_A
	};
	presentImagesViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	presentImagesViewCreateInfo.subresourceRange.baseMipLevel = 0;
	presentImagesViewCreateInfo.subresourceRange.levelCount = 1;
	presentImagesViewCreateInfo.subresourceRange.baseArrayLayer = 0;
	presentImagesViewCreateInfo.subresourceRange.layerCount = 1;

	VkFenceCreateInfo fenceCreateInfo = {};
	fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;

	VkFence submitFence;
	vkCreateFence(context.device, &fenceCreateInfo, nullptr, &submitFence);

	// Names such as "beginInfo" will be used further down in the code as well,
	// hence this code block has been wrapped in a new scope
	{
		VkCommandBufferBeginInfo beginInfo = {};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

		// Loop through the present images and change their layout
		auto *transitionedImages = new bool[imageCount];
		memset(transitionedImages, 0, sizeof(bool) * imageCount);
		uint32_t processedImageCount = 0;

		while (processedImageCount != imageCount)
		{
			VkSemaphore presentCompleteSemaphore;

			VkSemaphoreCreateInfo semaphoreCreateInfo = {};
			semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
			semaphoreCreateInfo.pNext = nullptr;
			semaphoreCreateInfo.flags = 0;

			vkCreateSemaphore(
				context.device,
				&semaphoreCreateInfo,
				nullptr,
				&presentCompleteSemaphore);

			uint32_t nextImageIndex = 0;
			vkAcquireNextImageKHR(
				context.device,
				context.swapChain,
				UINT64_MAX,
				presentCompleteSemaphore,
				VK_NULL_HANDLE,
				&nextImageIndex);

			// Only try to transition the image if it has not been transitioned yet
			if (!transitionedImages[nextImageIndex])
			{
				// Start recording in the setup command buffer
				vkBeginCommandBuffer(context.setupCommandBuffer, &beginInfo);

				VkImageSubresourceRange resourceRange = {};
				resourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				resourceRange.baseMipLevel = 0;
				resourceRange.levelCount = 1;
				resourceRange.baseArrayLayer = 0;
				resourceRange.layerCount = 1;

				VkImageMemoryBarrier layoutTransitionBarrier = {};
				layoutTransitionBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
				layoutTransitionBarrier.srcAccessMask = 0;
				layoutTransitionBarrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
				layoutTransitionBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
				layoutTransitionBarrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
				layoutTransitionBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
				layoutTransitionBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
				layoutTransitionBarrier.image = context.presentImages[nextImageIndex];
				layoutTransitionBarrier.subresourceRange = resourceRange;

				// Apply the image layout transition barrier
				vkCmdPipelineBarrier(
					context.setupCommandBuffer,
					VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
					VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
					0,
					0, nullptr,
					0, nullptr,
					1, &layoutTransitionBarrier);

				// Finished writing to this command buffer
				vkEndCommandBuffer(context.setupCommandBuffer);

				VkPipelineStageFlags waitStageMask[] =
				{
					VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
				};

				VkSubmitInfo submitInfo = {};
				submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
				submitInfo.waitSemaphoreCount = 1;
				submitInfo.pWaitSemaphores = &presentCompleteSemaphore;
				submitInfo.pWaitDstStageMask = waitStageMask;
				submitInfo.commandBufferCount = 1;
				submitInfo.pCommandBuffers = &context.setupCommandBuffer;
				submitInfo.signalSemaphoreCount = 0;
				submitInfo.pSignalSemaphores = nullptr;

				// Submit the commands to the setup queue
				result = vkQueueSubmit(
					context.presentQueue,
					1,
					&submitInfo,
					submitFence);

				Utility::checkVulkanResult(
					result,
					"Failed to submit present queue.");

				// Wait for the commands to finish
				vkWaitForFences(context.device, 1, &submitFence, VK_TRUE, UINT64_MAX);
				vkResetFences(context.device, 1, &submitFence);

				vkDestroySemaphore(context.device, presentCompleteSemaphore, nullptr);

				vkResetCommandBuffer(context.setupCommandBuffer, 0);

				transitionedImages[nextImageIndex] = true;
				++processedImageCount;
			}

			VkPresentInfoKHR presentInfo = {};
			presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
			presentInfo.waitSemaphoreCount = 0;
			presentInfo.pWaitSemaphores = nullptr;
			presentInfo.swapchainCount = 1;
			presentInfo.pSwapchains = &context.swapChain;
			presentInfo.pImageIndices = &nextImageIndex;

			vkQueuePresentKHR(context.presentQueue, &presentInfo);
		}

		delete[] transitionedImages;
	}

	auto *presentImageViews = new VkImageView[imageCount];
	for (uint32_t i = 0; i < imageCount; ++i)
	{
		presentImagesViewCreateInfo.image = context.presentImages[i];

		result = vkCreateImageView(
			context.device,
			&presentImagesViewCreateInfo,
			nullptr,
			&presentImageViews[i]);

		Utility::checkVulkanResult(result, "Failed to create an image view.");
	}

	// Create a depth image
	VkImageCreateInfo imageCreateInfo = {};
	imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
	imageCreateInfo.format = VK_FORMAT_D16_UNORM;
	imageCreateInfo.extent.width = context.width;
	imageCreateInfo.extent.height = context.height;
	imageCreateInfo.extent.depth = 1;
	imageCreateInfo.mipLevels = 1;
	imageCreateInfo.arrayLayers = 1;
	imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
	imageCreateInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
	imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	imageCreateInfo.queueFamilyIndexCount = 0;
	imageCreateInfo.pQueueFamilyIndices = nullptr;
	imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

	result = vkCreateImage(
		context.device,
		&imageCreateInfo,
		nullptr,
		&context.depthImage);

	Utility::checkVulkanResult(result, "Failed to create the depth image.");

	VkMemoryRequirements memoryRequirements = {};
	vkGetImageMemoryRequirements(
		context.device,
		context.depthImage,
		&memoryRequirements);

	VkMemoryAllocateInfo imageAllocateInfo = {};
	imageAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	imageAllocateInfo.allocationSize = memoryRequirements.size;

	uint32_t memoryTypeBits = memoryRequirements.memoryTypeBits;
	VkMemoryPropertyFlags desiredMemoryFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

	for (uint32_t i = 0; i < 32; ++i)
	{
		VkMemoryType memoryType = context.physicalDeviceMemoryProperties.memoryTypes[i];
		
		if (memoryTypeBits & 1)
		{
			if ((memoryType.propertyFlags & desiredMemoryFlags) == desiredMemoryFlags)
			{
				imageAllocateInfo.memoryTypeIndex = i;
				break;
			}
		}

		memoryTypeBits = memoryTypeBits >> 1;
	}

	VkDeviceMemory imageMemory = {};
	result = vkAllocateMemory(
		context.device,
		&imageAllocateInfo,
		nullptr,
		&imageMemory);

	Utility::checkVulkanResult(
		result,
		"Failed to allocate memory for the depth image.");

	result = vkBindImageMemory(
		context.device,
		context.depthImage,
		imageMemory,
		0);

	Utility::checkVulkanResult(
		result,
		"Failed to bind memory for the depth image.");

	{
		VkCommandBufferBeginInfo beginInfo = {};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

		// Begin recording commands into the setup command buffer
		vkBeginCommandBuffer(context.setupCommandBuffer, &beginInfo);

		VkImageMemoryBarrier layoutTransitionBarrier = {};
		layoutTransitionBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		layoutTransitionBarrier.srcAccessMask = 0;

		layoutTransitionBarrier.dstAccessMask =
			VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT |
			VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

		layoutTransitionBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		layoutTransitionBarrier.newLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
		layoutTransitionBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		layoutTransitionBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		layoutTransitionBarrier.image = context.depthImage;

		VkImageSubresourceRange resourceRange = {};
		resourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
		resourceRange.baseMipLevel = 0;
		resourceRange.levelCount = 1;
		resourceRange.baseArrayLayer = 0;
		resourceRange.layerCount = 1;

		layoutTransitionBarrier.subresourceRange = resourceRange;

		vkCmdPipelineBarrier(context.setupCommandBuffer,
			VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
			VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
			0,
			0,
			nullptr,
			0,
			nullptr,
			1,
			&layoutTransitionBarrier);

		vkEndCommandBuffer(context.setupCommandBuffer);

		VkPipelineStageFlags waitStageMask[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };

		VkSubmitInfo submitInfo = {};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.waitSemaphoreCount = 0;
		submitInfo.pWaitSemaphores = nullptr;
		submitInfo.pWaitDstStageMask = waitStageMask;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &context.setupCommandBuffer;
		submitInfo.signalSemaphoreCount = 0;
		submitInfo.pSignalSemaphores = nullptr;

		result = vkQueueSubmit(context.presentQueue, 1, &submitInfo, submitFence);
		Utility::checkVulkanResult(result, "Failed to submit present queue.");

		vkWaitForFences(context.device, 1, &submitFence, VK_TRUE, UINT64_MAX);
		vkResetFences(context.device, 1, &submitFence);
		vkResetCommandBuffer(context.setupCommandBuffer, 0);

		VkImageAspectFlags aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
		VkImageViewCreateInfo imageViewCreateInfo = {};
		imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		imageViewCreateInfo.image = context.depthImage;
		imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		imageViewCreateInfo.format = imageCreateInfo.format;
		imageViewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
		imageViewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
		imageViewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
		imageViewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
		imageViewCreateInfo.subresourceRange.aspectMask = aspectMask;
		imageViewCreateInfo.subresourceRange.baseMipLevel = 0;
		imageViewCreateInfo.subresourceRange.levelCount = 1;
		imageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
		imageViewCreateInfo.subresourceRange.layerCount = 1;

		result = vkCreateImageView(
			context.device,
			&imageViewCreateInfo,
			nullptr,
			&context.depthImageView);

		Utility::checkVulkanResult(result, "Failed to create depth image view.");
	}

	VkAttachmentDescription passAttachments[2] = {};
	passAttachments[0].format = surfaceColorFormat;
	passAttachments[0].samples = VK_SAMPLE_COUNT_1_BIT;
	passAttachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	passAttachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	passAttachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	passAttachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	passAttachments[0].initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	passAttachments[0].finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	passAttachments[1].format = VK_FORMAT_D16_UNORM;
	passAttachments[1].samples = VK_SAMPLE_COUNT_1_BIT;
	passAttachments[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	passAttachments[1].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	passAttachments[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	passAttachments[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	passAttachments[1].initialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	passAttachments[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkAttachmentReference colorAttachmentReference = {};
	colorAttachmentReference.attachment = 0;
	colorAttachmentReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkAttachmentReference depthAttachmentReference = {};
	depthAttachmentReference.attachment = 1;
	depthAttachmentReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpass = {};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &colorAttachmentReference;
	subpass.pDepthStencilAttachment = &depthAttachmentReference;

	VkRenderPassCreateInfo renderPassCreateInfo = {};
	renderPassCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassCreateInfo.attachmentCount = 2;
	renderPassCreateInfo.pAttachments = passAttachments;
	renderPassCreateInfo.subpassCount = 1;
	renderPassCreateInfo.pSubpasses = &subpass;

	result = vkCreateRenderPass(
		context.device,
		&renderPassCreateInfo,
		nullptr,
		&context.renderPass);

	Utility::checkVulkanResult(result, "Failed to create render pass.");

	// Create the frame buffers that are compatible with this render pass
	VkImageView frameBufferAttachments[2];
	frameBufferAttachments[1] = context.depthImageView;

	VkFramebufferCreateInfo framebufferCreateInfo = {};
	framebufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	framebufferCreateInfo.renderPass = context.renderPass;
	framebufferCreateInfo.attachmentCount = 2;
	framebufferCreateInfo.pAttachments = frameBufferAttachments;
	framebufferCreateInfo.width = context.width;
	framebufferCreateInfo.height = context.height;
	framebufferCreateInfo.layers = 1;

	// Create one framebuffer per swap chain image view
	context.framebuffers = new VkFramebuffer[imageCount];
	for (uint32_t i = 0; i < imageCount; ++i)
	{
		frameBufferAttachments[0] = presentImageViews[i];
		
		result = vkCreateFramebuffer(
			context.device,
			&framebufferCreateInfo,
			nullptr,
			&context.framebuffers[i]);

		Utility::checkVulkanResult(result, "Failed to create framebuffer.");
	}

	// Create a vertex buffer for the triangle
	VkBufferCreateInfo vertexInputBufferInfo = {};
	vertexInputBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	vertexInputBufferInfo.size = sizeof(Vertex) * 3;
	vertexInputBufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
	vertexInputBufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	result = vkCreateBuffer(
		context.device,
		&vertexInputBufferInfo,
		nullptr,
		&context.vertexInputBuffer);

	Utility::checkVulkanResult(result, "Failed to create vertex input buffer.");

	// Allocate the vertex input buffer
	VkMemoryRequirements vertexBufferMemoryRequirements = {};
	vkGetBufferMemoryRequirements(
		context.device,
		context.vertexInputBuffer,
		&vertexBufferMemoryRequirements);

	VkMemoryAllocateInfo bufferAllocateInfo = {};
	bufferAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	bufferAllocateInfo.allocationSize = vertexBufferMemoryRequirements.size;

	uint32_t vertexMemoryTypeBits = vertexBufferMemoryRequirements.memoryTypeBits;
	
	VkMemoryPropertyFlags vertexDesiredMemoryFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
	for (uint32_t i = 0; i < 32; ++i)
	{
		VkMemoryType memoryType = context.physicalDeviceMemoryProperties.memoryTypes[i];
		if (memoryTypeBits & 1)
		{
			if ((memoryType.propertyFlags & vertexDesiredMemoryFlags) == vertexDesiredMemoryFlags)
			{
				bufferAllocateInfo.memoryTypeIndex = i;
				break;
			}
		}

		vertexMemoryTypeBits = vertexMemoryTypeBits >> 1;
	}

	VkDeviceMemory vertexBufferMemory;
	result = vkAllocateMemory(
		context.device,
		&bufferAllocateInfo,
		nullptr,
		&vertexBufferMemory);

	Utility::checkVulkanResult(result, "Failed to allocate vertex buffer memory.");

	void *mapped = nullptr;
	result = vkMapMemory(context.device, vertexBufferMemory, 0, VK_WHOLE_SIZE, 0, &mapped);
	Utility::checkVulkanResult(result, "Failed to map vertex buffer memory.");

	Vertex *triangle = (Vertex *)mapped;
	Vertex vertex1 = { -1.0f, -1.0f, 0.0f };
	Vertex vertex2 = {  1.0f, -1.0f, 0.0f };
	Vertex vertex3 = {  0.0f,  1.0f, 0.0f };
	triangle[0] = vertex1;
	triangle[1] = vertex2;
	triangle[2] = vertex3;

	vkUnmapMemory(context.device, vertexBufferMemory);

	result = vkBindBufferMemory(
		context.device,
		context.vertexInputBuffer,
		vertexBufferMemory, 0);

	Utility::checkVulkanResult(result, "Failed to bind vertex buffer memmory.");
}

void Renderer::render()
{
	uint32_t nextImageIndex = 0;

	// Get the next available image ID from the swapchain
	vkAcquireNextImageKHR(
		context.device,
		context.swapChain,
		UINT64_MAX,
		VK_NULL_HANDLE,
		VK_NULL_HANDLE,
		&nextImageIndex);

	VkPresentInfoKHR presentInfo = {};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentInfo.pNext = nullptr;
	presentInfo.waitSemaphoreCount = 0;
	presentInfo.pWaitSemaphores = nullptr;
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = &context.swapChain;
	presentInfo.pImageIndices = &nextImageIndex;
	presentInfo.pResults = nullptr;

	vkQueuePresentKHR(context.presentQueue, &presentInfo);
}

void Renderer::loadExtensions()
{
	PFN_vkVoidFunction functionPointer = nullptr;
	
	functionPointer = vkGetInstanceProcAddr(
		context.instance,
		"vkCreateDebugReportCallbackEXT");
	assert(functionPointer != nullptr,
		"Failed to load the \"vkCreateDebugReportCallbackEXT\" extension.");
	fpVkCreateDebugReportCallbackEXT = reinterpret_cast<PFN_vkCreateDebugReportCallbackEXT>(functionPointer);
	functionPointer = nullptr;

	functionPointer = vkGetInstanceProcAddr(
		context.instance,
		"vkDestroyDebugReportCallbackEXT");
	assert(functionPointer != nullptr,
		"Failed to load the \"vkDestroyDebugReportCallbackEXT\" extension.");
	fpVkDestroyDebugReportCallbackEXT = reinterpret_cast<PFN_vkDestroyDebugReportCallbackEXT>(functionPointer);
	functionPointer = nullptr;

	functionPointer = vkGetInstanceProcAddr(
		context.instance,
		"vkDebugReportMessageEXT");
	assert(functionPointer != nullptr,
		"Failed to load the \"vkDebugReportMessageEXT\" extension.");
	fpVkDebugReportMessageEXT = reinterpret_cast<PFN_vkDebugReportMessageEXT>(functionPointer);
	functionPointer = nullptr;

	functionPointer = vkGetInstanceProcAddr(
		context.instance,
		"vkCreateWin32SurfaceKHR");
	assert(functionPointer != nullptr,
		"Failed to load the \"vkCreateWin32SurfaceKHR\" extension.");
	fpVkCreateWin32SurfaceKHR = reinterpret_cast<PFN_vkCreateWin32SurfaceKHR>(functionPointer);
	functionPointer = nullptr;
}
