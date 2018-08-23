#pragma once

#include <cstdint>
#include "vulkan/vulkan.hpp"

class Renderer
{
public:
	Renderer();
	~Renderer();

	void initialize(uint32_t width, uint32_t height);

private:

};