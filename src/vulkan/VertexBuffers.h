#pragma once

#include "VertexBufferBindingIndex.h"
#include "shader_builder/VertexShaderInputSet.h"
#include "shader_builder/VertexAttribute.h"
#include "memory/Buffer.h"
#include "utils/Vector.h"
#include "utils/Badge.h"
#include <vector>
#include <map>
#include <string>
#include <algorithm>

namespace vulkan {

namespace task {
class SynchronousWindow;
} // namespace task
namespace shader_builder {
class VertexShaderInputSetBase;
} // namespace shader_builder
namespace handle {
class CommandBuffer;
} // namespace handle

class VertexBuffers
{
 public:
  using glsl_id_full_to_vertex_attribute_container_t = std::map<std::string, shader_builder::VertexAttribute, std::less<>>;
  using vertex_shader_input_sets_container_t = utils::Vector<shader_builder::VertexShaderInputSetBase*, VertexBufferBindingIndex>;

 private:
  utils::Vector<memory::Buffer, VertexBufferBindingIndex> m_memory;     // The actual buffers and their memory allocation.
                                                                        //
  uint32_t m_binding_count;                                             // Cache value to be used by bind().
  char* m_data;                                                         // Idem.

  glsl_id_full_to_vertex_attribute_container_t m_glsl_id_full_to_vertex_attribute;      // Map VertexAttribute::m_glsl_id_full to the
                                                                                        // VertexAttribute object that contains it.
  vertex_shader_input_sets_container_t m_vertex_shader_input_sets;                      // Existing vertex shader input sets (a 'binding' slot).

 private:
  // Add shader variable (VertexAttribute) to m_glsl_id_full_to_vertex_attribute
  // (and a pointer to that to m_shader_variables), for a non-array vertex attribute.
  template<typename ContainingClass, glsl::Standard Standard, glsl::ScalarIndex ScalarIndex, int Rows, int Cols, size_t Alignment, size_t Size,
    size_t ArrayStride, int MemberIndex, size_t MaxAlignment, size_t Offset, utils::TemplateStringLiteral GlslIdStr>
  void add_vertex_attribute(VertexBufferBindingIndex binding, shader_builder::MemberLayout<ContainingClass,
      shader_builder::BasicTypeLayout<Standard, ScalarIndex, Rows, Cols, Alignment, Size, ArrayStride>,
      MemberIndex, MaxAlignment, Offset, GlslIdStr> const& member_layout);

  // Add shader variable (VertexAttribute) to m_glsl_id_full_to_vertex_attribute
  // (and a pointer to that to m_shader_variables), for a vertex attribute array.
  template<typename ContainingClass, glsl::Standard Standard, glsl::ScalarIndex ScalarIndex, int Rows, int Cols, size_t Alignment, size_t Size,
    size_t ArrayStride, int MemberIndex, size_t MaxAlignment, size_t Offset, utils::TemplateStringLiteral GlslIdStr, size_t Elements>
  void add_vertex_attribute(VertexBufferBindingIndex binding, shader_builder::MemberLayout<ContainingClass,
      shader_builder::ArrayLayout<shader_builder::BasicTypeLayout<Standard, ScalarIndex, Rows, Cols, Alignment, Size, ArrayStride>, Elements>,
      MemberIndex, MaxAlignment, Offset, GlslIdStr> const& member_layout);

 public:
  VertexBuffers() : m_binding_count(0), m_data(nullptr) { }

  template<typename ENTRY>
  requires (std::same_as<typename shader_builder::ShaderVariableLayouts<ENTRY>::tag_type, glsl::per_vertex_data> ||
            std::same_as<typename shader_builder::ShaderVariableLayouts<ENTRY>::tag_type, glsl::per_instance_data>)
  void create_vertex_buffer(task::SynchronousWindow const* owning_window, shader_builder::VertexShaderInputSet<ENTRY>& vertex_shader_input_set);

  glsl_id_full_to_vertex_attribute_container_t const& glsl_id_full_to_vertex_attribute() const
  {
    return m_glsl_id_full_to_vertex_attribute;
  }

  vertex_shader_input_sets_container_t const& vertex_shader_input_sets() const
  {
    return m_vertex_shader_input_sets;
  }

  // Called from CommandBuffer::bindVertexBuffers.
  [[gnu::always_inline]] inline void bind(utils::Badge<handle::CommandBuffer>, handle::CommandBuffer command_buffer) const;

#ifdef CWDEBUG
  void print_on(std::ostream& os) const;
#endif
};

} // namespace vulkan

#define LV_NEEDS_VERTEX_BUFFERS_INL_H
