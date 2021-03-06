#pragma once

#include <cstdint>
#include <Windows.h>
#include "vulkan/vulkan.hpp"

struct Vertex
{
	float x, y, z;
};

struct VulkanContext
{
	uint32_t width;
	uint32_t height;
	uint32_t presentQueueIndex;

	VkInstance instance;

	VkDevice device;
	VkPhysicalDevice physicalDevice;
	VkPhysicalDeviceProperties physicalDeviceProperties;
	VkPhysicalDeviceMemoryProperties physicalDeviceMemoryProperties;
	VkImage *presentImages;
	VkImage depthImage;
	VkImageView depthImageView;
	
	VkFramebuffer *framebuffers;
	VkRenderPass renderPass;

	VkBuffer vertexInputBuffer;
	VkQueue presentQueue;
	
	VkCommandBuffer setupCommandBuffer;
	VkCommandBuffer drawCommandBuffer;
	
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
	void render();

private:
	void loadExtensions();

private:
	VulkanContext context;
};