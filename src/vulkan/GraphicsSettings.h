#pragma once

#include "utils/Badge.h"
#include "threadsafe/threadsafe.h"
#ifdef CWDEBUG
#include <iosfwd>
#endif

namespace vulkan {

class Application;

struct GraphicsSettingsPOD
{
  float maxAnisotropy;          // Default clamp used for texture samplers. Should be between 1 and VkPhysicalDeviceLimits::maxSamplerAnisotropy inclusive.
};

class UnlockedGraphicsSettings : private GraphicsSettingsPOD
{
 public:
  UnlockedGraphicsSettings() : GraphicsSettingsPOD{ 1.0f } { }

  // Give read access to GraphicsSettingsPOD.
  GraphicsSettingsPOD const& pod() const { return *this; }

  // Give write access to Application only; methods of Application that access this must call
  // Application::synchronize_graphics_settings() afterwards if these functions return true.

  // Return true when changed.
  bool set_max_anisotropy(utils::Badge<Application>, float max_anisotropy)
  {
    if (maxAnisotropy == max_anisotropy)
      return false;
    maxAnisotropy = max_anisotropy;
    return true;
  }

#ifdef CWDEBUG
  void print_on(std::ostream& os) const;
#endif
};

using GraphicsSettings = threadsafe::Unlocked<UnlockedGraphicsSettings, threadsafe::policy::Primitive<std::mutex>>;

} // namespace vulkan
