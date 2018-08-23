#include <assert.h>

#include "vulkan/vulkan.hpp"
#include "LearningVulkan/Utility.hpp"

void Utility::checkVulkanResult(VkResult & result, char * message)
{
	assert(result == VK_SUCCESS, message);
}

Utility::Utility()
{
}

Utility::~Utility()
{
}