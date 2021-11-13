#include <vulkan/vulkan.hpp>
#include <system_error>

namespace VULKAN_HPP_NAMESPACE {

std::ostream& operator<<(std::ostream& os, Result code);
inline char const* get_domain(Result) { return "vulkan:Result.Error"; }

} // namespace VULKAN_HPP_NAMESPACE
