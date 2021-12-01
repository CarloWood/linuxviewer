#include "sys.h"
#include "WindowEvents.h"

namespace vulkan {

void WindowEvents::on_window_size_changed(uint32_t width, uint32_t height)
{
  DoutEntering(dc::vulkan, "WindowEvents::on_window_size_changed(" << width << ", " << height << ")");

  // Update m_extent so that get_extent() (see handle_window_size_changed() below) will return the new value.
  set_extent({width, height});  // This call will cause a call to handle_window_size_changed() from the render loop if the extent really changed.
}

} // namespace vulkan
