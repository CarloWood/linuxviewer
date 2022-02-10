#include "sys.h"
#include "debug.h"
// The header file <vulkan/vk_object_types.h> was generated with this define defined.
// Therefore we must include vk_format_utils.cpp while the same macro is defined, or
// we end up with undefined enums.
#define VK_ENABLE_BETA_EXTENSIONS
#include <vulkan/vk_format_utils.cpp>
