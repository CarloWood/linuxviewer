#ifndef VULKAN_SHADERBUILDER_VERTEX_SHADER_INPUT_SET_H
#define VULKAN_SHADERBUILDER_VERTEX_SHADER_INPUT_SET_H

#include "ShaderVariableLayout.h"
#include "memory/DataFeeder.h"
#include <vulkan/vulkan.hpp>
#include <boost/intrusive_ptr.hpp>
#include <vector>
#include <type_traits>
#include "debug.h"

// Forward declaration
namespace vulkan::pipeline {
class CharacteristicRange;
} // namespace vulkan::pipeline

namespace vulkan::shaderbuilder {

class VertexShaderInputSetBase : public DataFeeder
{
 private:
  vk::VertexInputRate m_input_rate;     // Per vertex (vk::VertexInputRate::eVertex) or per instance (vk::VertexInputRate::eInstance).

 public:
  VertexShaderInputSetBase(vk::VertexInputRate input_rate) : m_input_rate(input_rate) { }

  // Accessor. The input rate of this set.
  vk::VertexInputRate input_rate() const { return m_input_rate; }
};

class VertexShaderInputSetFeeder final : public DataFeeder
{
 private:
  VertexShaderInputSetBase* m_input_set;                                                // The actual data feeder.
  boost::intrusive_ptr<vulkan::pipeline::CharacteristicRange const> m_input_set_owner;  // Must keep this alive. The input set pointed to by m_input_set
                                                                                        // will be owned by the object derived from this.
 public:
  [[gnu::always_inline]] inline VertexShaderInputSetFeeder(VertexShaderInputSetBase* input_set, vulkan::pipeline::CharacteristicRange const* input_set_owner);

  uint32_t chunk_size() const override { return m_input_set->chunk_size(); }
  int chunk_count() const override { return m_input_set->chunk_count(); }
  int next_batch() override { return m_input_set->next_batch(); }
  void get_chunks(unsigned char* chunk_ptr) override { m_input_set->get_chunks(chunk_ptr); }
};

// ENTRY should be a struct existing solely of types specified in math/glsl.h,
// but I don't think that can be checked with a concept :/.
//
// Also vulkan::shaderbuilder::ShaderVariableLayouts<ENTRY> must be overloaded and
// specify the static constexpr members `input_rate` and `layouts`, where
// the latter lists all of the members of ENTRY.
//
// For example, if ENTRY is `InstanceData`, one has to specify the layout in advance:
//
// #include "ShaderVariableLayout.h"
//
// struct InstanceData;
//
// LAYOUT_DECLARATION(InstanceData, per_instance_data)
// {
//   static constexpr auto layouts = make_layouts(
//     MEMBER(vec4, m_position),
//     MEMBER(mat4, m_matrix)
//   );
// };
//
// This can simply be done immediately before the struct itself.
//
// struct InstanceData
// {
//   glsl::vec4 m_position;
//   glsl::mat4 m_matrix;
// };
//

template<typename ENTRY>
class VertexShaderInputSet : public VertexShaderInputSetBase
{
 public:
  // Constructor. Pass the input rate to the base class, extracting that info from ENTRY.
  VertexShaderInputSet() : VertexShaderInputSetBase(shaderbuilder::ShaderVariableLayouts<ENTRY>::input_rate) { }

 private:
  uint32_t chunk_size() const override final
  {
    return sizeof(ENTRY);
  }

  int next_batch() override
  {
    // Default value.
    return 1;
  }

  virtual void create_entry(ENTRY* input_entry_ptr) = 0;

  void get_chunks(unsigned char* chunk_ptr) override final
  {
    ASSERT(reinterpret_cast<size_t>(chunk_ptr) % alignof(ENTRY) == 0);
    create_entry(reinterpret_cast<ENTRY*>(chunk_ptr));
  }
};

} // namespace vulkan::shaderbuilder

#endif // VULKAN_SHADERBUILDER_VERTEX_SHADER_INPUT_SET_H

#ifndef VULKAN_PIPELINE_CHARACTERISTIC_RANGE_H
#include "pipeline/CharacteristicRange.h"
#endif

#ifndef VULKAN_SHADERBUILDER_VERTEX_SHADER_INPUT_SET_H_definitions
#define VULKAN_SHADERBUILDER_VERTEX_SHADER_INPUT_SET_H_definitions

namespace vulkan::shaderbuilder {

//inline
VertexShaderInputSetFeeder::VertexShaderInputSetFeeder(VertexShaderInputSetBase* input_set, vulkan::pipeline::CharacteristicRange const* input_set_owner) :
  m_input_set(input_set), m_input_set_owner(input_set_owner) { }

} // namespace vulkan::shaderbuilder

#endif // VULKAN_SHADERBUILDER_VERTEX_SHADER_INPUT_SET_H_definitions
