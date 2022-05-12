gitache_config(
  GIT_REPOSITORY
    "https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator.git"
  GIT_TAG
    "v3.0.0"
  CMAKE_ARGS
    "-DCMAKE_BUILD_TYPE=Release -DVMA_STATIC_VULKAN_FUNCTIONS:BOOL=OFF -DVMA_DYNAMIC_VULKAN_FUNCTIONS:BOOL=ON"
)
