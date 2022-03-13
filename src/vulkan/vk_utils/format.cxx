#include "sys.h"
#include "debug.h"
// The header file <vulkan/vk_object_types.h> was generated with this define defined.
// Therefore we must include vulkan/vulkan_core.h while the same macro is defined, or
// we end up with undefined enums.
#define VK_ENABLE_BETA_EXTENSIONS
#include <vulkan/vulkan_core.h>
// Afterwards we have to undefine the macro again because otherwise other compiler
// errors turn up!
#undef VK_ENABLE_BETA_EXTENSIONS
#include <vulkan/vk_format_utils.cpp>
