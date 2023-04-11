#pragma once
#undef LV_NEEDS_VERTEX_BUFFERS_INL_H

#include "SynchronousWindow.h"
#include "CommandBuffer.h"
#include "queues/CopyDataToBuffer.h"

namespace vulkan {

void VertexBuffers::bind(utils::Badge<handle::CommandBuffer>, handle::CommandBuffer command_buffer) const
{
  command_buffer.vk::CommandBuffer::bindVertexBuffers(0 /* uint32_t first_binding */,
      m_binding_count,
      reinterpret_cast<vk::Buffer*>(m_data),
      reinterpret_cast<vk::DeviceSize*>(m_data + m_binding_count * sizeof(vk::Buffer)));
}

template<typename ContainingClass, glsl::Standard Standard, glsl::ScalarIndex ScalarIndex, int Rows, int Cols, size_t Alignment, size_t Size,
  size_t ArrayStride, int MemberIndex, size_t MaxAlignment, size_t Offset, utils::TemplateStringLiteral GlslIdStr>
void VertexBuffers::add_vertex_attribute(VertexBufferBindingIndex binding, shader_builder::MemberLayout<ContainingClass,
    shader_builder::BasicTypeLayout<Standard, ScalarIndex, Rows, Cols, Alignment, Size, ArrayStride>,
    MemberIndex, MaxAlignment, Offset, GlslIdStr> const& member_layout)
{
  std::string_view glsl_id_sv = static_cast<std::string_view>(member_layout.glsl_id_full);
  // These strings are made with std::to_array("stringliteral"), which includes the terminating zero,
  // but the trailing '\0' was already removed by the conversion to string_view.
  ASSERT(!glsl_id_sv.empty() && glsl_id_sv.back() != '\0');
  shader_builder::VertexAttribute vertex_attribute_tmp(
    { .m_standard = Standard,
      .m_rows = Rows,
      .m_cols = Cols,
      .m_scalar_type = ScalarIndex,
      .m_log2_alignment = utils::log2(Alignment),
      .m_size = Size,
      .m_array_stride = ArrayStride },
    glsl_id_sv.data(),
    Offset,
    0 /* array_size */,
    binding
  );
  Dout(dc::vulkan, "Registering \"" << glsl_id_sv << "\" with vertex attribute " << vertex_attribute_tmp);
  auto ibp = m_glsl_id_full_to_vertex_attribute.insert(std::pair{glsl_id_sv, vertex_attribute_tmp});
  // The m_glsl_id_full of each ENTRY must be unique. And of course, don't register the same attribute twice.
  ASSERT(ibp.second);
}

template<typename ContainingClass, glsl::Standard Standard, glsl::ScalarIndex ScalarIndex, int Rows, int Cols, size_t Alignment, size_t Size,
  size_t ArrayStride, int MemberIndex, size_t MaxAlignment, size_t Offset, utils::TemplateStringLiteral GlslIdStr, size_t Elements>
void VertexBuffers::add_vertex_attribute(VertexBufferBindingIndex binding, shader_builder::MemberLayout<ContainingClass,
    shader_builder::ArrayLayout<shader_builder::BasicTypeLayout<Standard, ScalarIndex, Rows, Cols, Alignment, Size, ArrayStride>, Elements>,
    MemberIndex, MaxAlignment, Offset, GlslIdStr> const& member_layout)
{
  std::string_view glsl_id_sv = static_cast<std::string_view>(member_layout.glsl_id_full);
  // These strings are made with std::to_array("stringliteral"), which includes the terminating zero,
  // but the trailing '\0' was already removed by the conversion to string_view.
  ASSERT(!glsl_id_sv.empty() && glsl_id_sv.back() != '\0');
  shader_builder::VertexAttribute vertex_attribute_tmp(
    { .m_standard = Standard,
      .m_rows = Rows,
      .m_cols = Cols,
      .m_scalar_type = ScalarIndex,
      .m_log2_alignment = utils::log2(Alignment),
      .m_size = Size,
      .m_array_stride = ArrayStride },
    glsl_id_sv.data(),
    Offset,
    Elements,
    binding
  );
  Dout(dc::vulkan, "Registering \"" << glsl_id_sv << "\" with vertex attribute " << vertex_attribute_tmp);
  auto ibp = m_glsl_id_full_to_vertex_attribute.insert(std::pair{glsl_id_sv, vertex_attribute_tmp});
  // The m_glsl_id_full of each ENTRY must be unique. And of course, don't register the same attribute twice.
  ASSERT(ibp.second);
}

template<typename ENTRY>
requires (std::same_as<typename shader_builder::ShaderVariableLayouts<ENTRY>::tag_type, glsl::per_vertex_data> ||
          std::same_as<typename shader_builder::ShaderVariableLayouts<ENTRY>::tag_type, glsl::per_instance_data>)
void VertexBuffers::create_vertex_buffer(
    task::SynchronousWindow const* owning_window,
    shader_builder::VertexShaderInputSet<ENTRY>& vertex_shader_input_set)
{
  DoutEntering(dc::vulkan, "VertexBuffers::create_vertex_buffer<" << libcwd::type_info_of<ENTRY>().demangled_name() << ">(" <<
      owning_window << ", &" << &vertex_shader_input_set << ")" << this << "]");
  using namespace shader_builder;

  VertexBufferBindingIndex binding = m_vertex_shader_input_sets.iend();

  // Use the specialization of ShaderVariableLayouts to get the layout of ENTRY
  // in the form of a tuple, of the vertex attributes, `layouts`.
  // Then for each member layout call insert_vertex_attribute.
  [&]<typename... MemberLayout>(std::tuple<MemberLayout...> const& layouts)
  {
    ([&, this]
    {
      {
        static constexpr int member_index = MemberLayout::member_index;
        add_vertex_attribute(binding, std::get<member_index>(layouts));
      }
    }(), ...);
  }(typename decltype(ShaderVariableLayouts<ENTRY>::struct_layout)::members_tuple{});

  // Keep track of all VertexShaderInputSetBase objects.
  m_vertex_shader_input_sets.push_back(&vertex_shader_input_set);

  VertexShaderInputSetBase const& input_set = vertex_shader_input_set;
  size_t entry_size = input_set.chunk_size();
  int count = input_set.chunk_count();
  size_t buffer_size = count * entry_size;

  // This is the case for ImGUI.
  if (buffer_size == 0)
    return;

  m_memory.push_back(memory::Buffer{owning_window->logical_device(), buffer_size,
      { .usage = vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eVertexBuffer,
        .properties = vk::MemoryPropertyFlagBits::eDeviceLocal }
      COMMA_CWDEBUG_ONLY(owning_window->debug_name_prefix("m_memory[" + std::to_string(m_memory.size()) + "]"))});
  vk::Buffer new_buffer = m_memory.back().m_vh_buffer;

  // The memory layout of the m_data memory block is:
  //               .------------------.
  //     m_data -->| vk::Buffer       | ⎫
  //               |   .              | ⎬ m_binding_count * sizeof(vk::Buffer) bytes.
  //               |   .              | ⎮
  //               | vk::Buffer       | ⎭
  //               | vk::DeviceSize   | ⎫
  //               |   .              | ⎬ m_binding_count * sizeof(vk::DeviceSize) bytes.
  //               |   .              | ⎮
  //               | vk::DeviceSize   | ⎭
  //               `------------------'
  // We're going to append one vk::Buffer and one vk::Device.
  ++m_binding_count;
  m_data = (char*)std::realloc(m_data, m_binding_count * (sizeof(vk::Buffer) + sizeof(vk::DeviceSize)));
  char* new_device_size_start = m_data + m_binding_count * sizeof(vk::Buffer);
  // After the realloc we need to move the vk::DeviceSize's in memory one up to make place for the new vk::Buffer.
  std::memmove(new_device_size_start, new_device_size_start - sizeof(vk::Buffer), (m_binding_count - 1) * sizeof(vk::DeviceSize));
  reinterpret_cast<vk::Buffer*>(m_data)[m_binding_count - 1] = new_buffer;
  reinterpret_cast<vk::DeviceSize*>(new_device_size_start)[m_binding_count - 1] = 0;  // All offsets are zero at the moment.

  auto copy_data_to_buffer = statefultask::create<task::CopyDataToBuffer>(owning_window->logical_device(), buffer_size, new_buffer, 0,
      vk::AccessFlags(0), vk::PipelineStageFlagBits::eTopOfPipe, vk::AccessFlagBits::eVertexAttributeRead,
      vk::PipelineStageFlagBits::eVertexInput COMMA_CWDEBUG_ONLY(Application::instance().debug_CopyDataToBuffer()));

  copy_data_to_buffer->set_resource_owner(owning_window);       // Wait for this task to finish before destroying this window,
                                                                // because this window owns the buffer (m_vertex_buffers.back()),
                                                                // as well as vertex_shader_input_set (or at least, it should).
  copy_data_to_buffer->set_data_feeder(
      std::make_unique<VertexShaderInputSetFeeder>(&vertex_shader_input_set));

  copy_data_to_buffer->run(Application::instance().low_priority_queue());
}

} // namespace vulkan
