#include "sys.h"
#include "ClearValue.h"
#ifdef CWDEBUG
#include "debug/debug_ostream_operators.h"
#include <iostream>
#endif

namespace vulkan::rendergraph {

ClearValue& ClearValue::operator=(std::initializer_list<double> ilist)
{
  std::size_t n = ilist.size();
  // Only use this to assign four floats for a color clear value:
  //   color = { 1.f, 0.5f, 0.f, 1.f };         // Size is 4.
  // or to assign a depth/stencil clear value, passing a float and an uint32_t:
  //   depth_stencil = { 1.f, 0xffffffff };     // Size is 2.
  ASSERT(n == 2 || n == 4);

  auto iter = ilist.begin();
  if (n == 2)
  {
    // Can not change from color to depth/stencil. Construct this variable as depth/stencil clear value before assigning it.
    // Either:
    //   ClearValue cv = 1.f;
    // or
    //   ClearValue cv({1.f, 0xffffffff});
    ASSERT(!m_color);
    vk::ClearDepthStencilValue v;
    v.depth = *iter++;
    // The depth clear value must be in the range [0, 1].
    ASSERT(0 <= v.depth && v.depth <= 1.f);
    v.stencil = *iter;
    // The second element (stencil clear value) must fit in an unsigned 32bit int.
    ASSERT(v.stencil == *iter);
    m_data.setDepthStencil(v);
  }
  else
  {
    // Can not change from depth/stencil to color. Construct this variable as color clear value before assigning it.
    ASSERT(m_color);
    vk::ClearColorValue v{};
    for (int i = 0; i < 4; ++i)
      v.float32[i] = *iter++;
    m_data.setColor(v);
  }

  return *this;
}

ClearValue::ClearValue(std::initializer_list<double> ilist)
{
  std::size_t n = ilist.size();
  // Only use this to construct a ClearValue with four floats for a color:
  //   ClearValue color({ 1.f, 0.5f, 0.f, 1.f });       // Size is 4.
  // or to construct a ClearValue for a depth/stencil, passing a float and an uint32_t:
  //   ClearValue depth_stencil({ 1.f, 0xffffffff });   // Size is 2.
  ASSERT(n == 2 || n == 4);

  auto iter = ilist.begin();
  if (n == 2)
  {
    m_color = false;
    vk::ClearDepthStencilValue v;
    v.depth = *iter++;
    // The depth clear value must be in the range [0, 1].
    ASSERT(0 <= v.depth && v.depth <= 1.f);
    v.stencil = *iter;
    // The second element (stencil clear value) must fit in an unsigned 32bit int.
    ASSERT(v.stencil == *iter);
    m_data.setDepthStencil(v);
  }
  else
  {
    m_color = true;
    vk::ClearColorValue v{};
    for (int i = 0; i < 4; ++i)
      v.float32[i] = *iter++;
    m_data.setColor(v);
  }
}

#ifdef CWDEBUG
void ClearValue::print_on(std::ostream& os) const
{
  os << '{';
  if (m_color)
    os << "color:" << m_data.color;
  else
    os << "depthStencil:" << m_data.depthStencil;
  os << '}';
}
#endif

} // namespace vulkan::rendergraph
