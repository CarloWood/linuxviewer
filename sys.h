#include "cwds/sys.h"

// When vulkan/vulkan.hpp is too old, then this is missing:
#if defined( VULKAN_HPP_NO_CONSTRUCTORS )
#  if !defined( VULKAN_HPP_NO_STRUCT_CONSTRUCTORS )
#    define VULKAN_HPP_NO_STRUCT_CONSTRUCTORS
#  endif
#  if !defined( VULKAN_HPP_NO_UNION_CONSTRUCTORS )
#    define VULKAN_HPP_NO_UNION_CONSTRUCTORS
#  endif
#endif

#ifdef TRACY_ENABLE
#define TRACY_ONLY(x) x
#define COMMA_TRACY_ONLY(x) , x
#else
#define TRACY_ONLY(x)
#define COMMA_TRACY_ONLY(x)
#endif
