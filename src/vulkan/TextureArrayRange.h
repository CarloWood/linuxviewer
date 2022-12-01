#pragma once

#include "Texture.h"
#include "descriptor/ArrayElementRange.h"
#ifdef CWDEBUG
#include <iosfwd>
#endif

namespace vulkan {

class TextureArrayRange
{
 private:
  Texture const* m_texture_array;                               // The Texture array source.
  descriptor::ArrayElementRange m_array_element_range;          // The descriptor array target indexes.

 public:
  // Construct an uninitialized TextureArrayRange.
  TextureArrayRange() = default;

  // Constructor.
  TextureArrayRange(Texture const* texture_array, descriptor::ArrayElementRange array_element_range) :
    m_texture_array(texture_array), m_array_element_range(array_element_range) { }

  // Accessors.
  Texture const* texture_array() const { return m_texture_array; }
  descriptor::ArrayElementRange array_element_range() const { return m_array_element_range; }

  friend bool operator==(TextureArrayRange const& lhs, TextureArrayRange const& rhs)
  {
    return lhs.m_texture_array == rhs.m_texture_array && lhs.m_array_element_range == rhs.m_array_element_range;
  }

  bool overlaps(TextureArrayRange const& other)
  {
    return !(m_texture_array + m_array_element_range.size() <= other.m_texture_array ||
        other.m_texture_array + other.m_array_element_range.size() <= m_texture_array);
  }

#ifdef CWDEBUG
  void print_on(std::ostream& os) const;
#endif
};

} // namespace vulkan
