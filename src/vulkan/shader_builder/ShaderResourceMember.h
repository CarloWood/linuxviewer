#pragma once

#include "BasicType.h"
#include "GlslIdFull.h"
#include "debug.h"

namespace vulkan::shader_builder {

class ShaderResourceMember : public GlslIdFull
{
 public:
  using container_t = std::vector<ShaderResourceMember>;        // For containers that map member indexes to their corresponding ShaderResourceMember object.

 private:
  BasicType m_basic_type{};             // The glsl basic type of the variable, or underlying basic type in case of an array.
  uint32_t m_offset{};                  // The offset of the variable inside its GLSL ENTRY struct.
  uint32_t m_array_size{};              // Set to zero when this is not an array.
  int m_member_index{};                 // Member index.

 public:
  static constexpr uint32_t not_an_array_magic = 0;
  // Accessors.
  BasicType basic_type() const { return m_basic_type; }
  uint32_t offset() const { return m_offset; }
  bool is_array() const { return m_array_size != not_an_array_magic; }
  uint32_t array_size() const
  {
    // Don't call array_size() when is_array returns false.
    ASSERT(m_array_size > 0);
    return m_array_size;
  }
  int member_index() const { return m_member_index; }

 public:
  ShaderResourceMember(char const* glsl_id_full) : GlslIdFull(glsl_id_full) { }

  ShaderResourceMember(char const* glsl_id_full, int member_index, BasicType basic_type, uint32_t offset, uint32_t array_size = not_an_array_magic) :
    GlslIdFull(glsl_id_full), m_basic_type(basic_type), m_offset(offset), m_array_size(array_size), m_member_index(member_index) { }

#if 0
  uint32_t size() const
  {
    uint32_t sz = m_basic_type.size();
    if (m_array_size != not_an_array_magic)
      sz = m_basic_type.array_stride() * m_array_size;
    return sz;
  }
#endif

#ifdef CWDEBUG
 public:
  void print_on(std::ostream& os) const;
#endif
};

} // namespace vulkan::shader_builder
