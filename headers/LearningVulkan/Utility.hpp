#pragma once

#include "vulkan/vulkan.hpp"

class Utility
{
public:
	static void checkVulkanResult(VkResult &result, char *message);

private:
	Utility();
	~Utility();
};