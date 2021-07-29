#include "sys.h"
#include "DynamicLoader.h"

#if VULKAN_HPP_DISPATCH_LOADER_DYNAMIC == 1

namespace vulkan {

} // namespace vulkan

// Storage space for the dynamic loader.
VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE

#endif // VULKAN_HPP_DISPATCH_LOADER_DYNAMIC == 1
