#pragma once

#include "glm.h"

struct Light
{
	glm::vec3 pos;
	glm::vec3 color;
	glm::vec3 brightness_specscale_shinniness;
};