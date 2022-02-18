#pragma once

#include <vulkan/vulkan.hpp>
#include "debug.h"
#ifdef CWDEBUG
#include "debug/vulkan_print_on.h"
#endif

namespace vulkan::rendergraph {

class ClearValue
{
 private:
  vk::ClearValue m_data = {};
  bool m_color;

 public:
  // Construct a color clear value.
  ClearValue(vk::ClearColorValue const& value) : m_color(true) { m_data.setColor(value); }

  // Construct the clear value black.
  ClearValue() : ClearValue({ .float32 = {{ 0.f, 0.f, 0.f, 1.f }}}) { }

  // Construct a depth/stencil clear value.
  ClearValue(vk::ClearDepthStencilValue const& value) : m_color(false) { m_data.setDepthStencil(value); }

  // Construct a depth clear value - ignoring the stencil.
  ClearValue(float depth_clear_value) : ClearValue(vk::ClearDepthStencilValue{ depth_clear_value, 0 }) { }

  // Construct a clear value from an initializer list.
  ClearValue(std::initializer_list<double> ilist);

  // Assignment operators.

  // Use for example as:
  //
  //   color = { .uint32 = {{ 0, 255, 255, 255 }} };
  // or
  //   color = { .int32 = {{ -128, 127, 0, 127 }} };
  //
  ClearValue& operator=(vk::ClearColorValue const& value)
  {
    // Can not change from depth/stencil to color. Construct this variable as color clear value before assigning it.
    ASSERT(m_color);
    m_data.setColor(value);
    return *this;
  }

  // Assign a vk::ClearDepthStencilValue directly.
  ClearValue& operator=(std::same_as<vk::ClearDepthStencilValue> auto const& value)
  {
    // Can not change from color to depth/stencil. Construct this variable as depth/stencil clear value before assigning it.
    ASSERT(!m_color);
    m_data.setDepthStencil(value);
    return *this;
  }

  ClearValue& operator=(float depth_clear_value)
  {
    // Can not change from color to depth/stencil. Construct this variable as depth/stencil clear value before assigning it.
    ASSERT(!m_color);
    m_data.setDepthStencil({ depth_clear_value, 0 });
    return *this;
  }

  // Only define the assignment operator for float.
  // If you want to assign ints use color = { .uint32 = {{ 0, 1, 128, 255 }} };
  // We use double here so that you can also use it to assign a depth/stencil clear value;
  // either use:
  //
  //   color = { 1, 0, 0, 1 };          // Red (floats)
  //   depth = { 1, 0xffeeddcc };       // depth / stencil.
  //
  // You are only allowed to assign four floats (between 0 and 1),
  // or a float depth clear value (between 0 and 1) and an unsigned 32-bit int (stencil).
  //
  ClearValue& operator=(std::initializer_list<double> ilist);

  // Accessor.
  operator vk::ClearValue const&() const { return m_data; }

#ifdef CWDEBUG
  void print_on(std::ostream& os) const;
#endif
};

} // namespace vulkan::rendergraph
