#include "sys.h"
#include "print_chain.h"
#include "Defaults.h"
#include <iostream>

namespace vk_utils {

std::ostream& operator<<(std::ostream& os, PNextChain const* chain)
{
  os << '{';
  switch (chain->sType)
  {
    case vk::StructureType::ePhysicalDeviceVulkan11Features:
    {
      vk::PhysicalDeviceVulkan11Features const* feature = reinterpret_cast<vk::PhysicalDeviceVulkan11Features const*>(chain);
      static_cast<vk_defaults::PhysicalDeviceVulkan11Features const*>(feature)->print_members(os, "⛓");
      break;
    }
    case vk::StructureType::ePhysicalDeviceVulkan12Features:
    {
      vk::PhysicalDeviceVulkan12Features const* feature = reinterpret_cast<vk::PhysicalDeviceVulkan12Features const*>(chain);
      static_cast<vk_defaults::PhysicalDeviceVulkan12Features const*>(feature)->print_members(os, "⛓");
      break;
    }
    default:
      os << to_string(chain->sType);
  }
  os << '}';

  if (chain->pNext)
    os << " ⛓ " << print_chain(chain->pNext);

  return os;
}

} // namespace vk_utils
