#pragma once

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <cassert>
#include <exception>
#include <stdexcept>
#include <chrono>
#include <thread>

#include "vk_mem_alloc.h"

#define GLM_FORCE_RADIANS 1
#define GLM_FORCE_LEFT_HANDED 1
#define GLM_FORCE_DEPTH_ZERO_TO_ONE 1
#include <glm/glm.hpp>
#include <glm/gtc/color_space.hpp>
#include <glm/gtx/color_space.hpp>
#include <glm/gtc/constants.hpp>

#define GET_ARRAY_SIZE(x) sizeof(x)/sizeof(x[0])
