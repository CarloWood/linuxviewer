#pragma once

#include "../Defaults.h"

namespace vulkan {

class ApplicationInfo : protected vk_defaults::ApplicationInfo
{
 private:
  std::u8string m_application_name;

 public:
  // Only provide non-mutable access to the base class.
  vk::ApplicationInfo const& read_access() const
  {
    return *this;
  }

  void set_application_name(std::u8string const& application_name)
  {
    m_application_name = application_name;
    setPApplicationName(reinterpret_cast<char const*>(m_application_name.c_str()));
  }

  void set_application_version(uint32_t encoded_version)
  {
    setApplicationVersion(encoded_version);
  }
};

} // namespace vulkan
