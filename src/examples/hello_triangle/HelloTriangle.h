#pragma once

#include <vulkan/Application.h>

class HelloTriangle : public vulkan::Application
{
 public:
  std::u8string application_name() const override
  {
    return u8"Hello Triangle";
  }
};
