#include "sys.h"
#include "Window.h"
#include "Application.h"
#include <vulkan/vulkan.hpp>

void Window::create_surface()
{
  DoutEntering(dc::vulkan, "Window::create_surface() [" << (void*)this << "]");
  vk::Instance vulkan_instance{static_cast<Application&>(application()).vulkan_instance()};
  m_surface = get_glfw_window().createSurface(vulkan_instance);
}

Window::~Window()
{
  DoutEntering(dc::vulkan, "Window::~Window() [" << (void*)this << "]");
  if (m_surface)
  {
    vk::Instance vulkan_instance{static_cast<Application&>(application()).vulkan_instance()};
    vulkan_instance.destroySurfaceKHR(m_surface);
  }
}
