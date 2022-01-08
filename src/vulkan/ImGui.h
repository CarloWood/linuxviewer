#pragma once

namespace task {
class SynchronousWindow;
} // namespace task

namespace vulkan {

class ImGui
{
 public:
  void init(task::SynchronousWindow const* owning_window);
};

} // namespace vulkan
