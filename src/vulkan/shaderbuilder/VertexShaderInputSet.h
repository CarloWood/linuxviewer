#pragma once

#include <vector>
#include <type_traits>
#include <vulkan/vulkan.hpp>

namespace vulkan::shaderbuilder {

template<typename T>
struct VertexAttributes
{
};

class VertexShaderInputSetBase
{
 private:
  vk::VertexInputRate m_input_rate;     // Per vertex (vk::VertexInputRate::eVertex) or per instance (vk::VertexInputRate::eInstance).

 public:
  VertexShaderInputSetBase(vk::VertexInputRate input_rate) : m_input_rate(input_rate) { }

  // Accessor. The input rate of this set.
  vk::VertexInputRate input_rate() const { return m_input_rate; }

  // The total number of input entries (the number of times that get_input_entry will be called).
  virtual int count() const = 0;

  // The next call to get_input_entry will initialize this many entries.
  virtual int next_batch() const = 0;

  // The size of one input entry.
  // The returned size must be a multiple of the required alignment.
  virtual uint32_t size() const = 0;

  // This is called count() times to fill in the information of one input entry, where input_entry_index
  // will run from 0 till count() (exclusive) and input_entry_ptr is incremented in steps of size().
  virtual void get_input_entry(void* input_entry_ptr) = 0;
};

// ENTRY should be a struct existing solely of types specified in math/glsl.h,
// but I don't think that can be checked with a concept :/.
//
// Also vulkan::shaderbuilder::VertexAttributes<ENTRY> must be overloaded and
// specify the static constexpr members `input_rate` and `attributes`, where
// the latter lists all of the members of ENTRY.
//
// For example, if ENTRY is `InstanceData`, which is defined as
//
// #include "math/glsl.h"
//
// struct InstanceData
// {
//   glsl::vec4 m_position;
//   glsl::mat4 m_matrix;
// };
//
// Then you also have to specify
//
// namespace vulkan::shaderbuilder {
//
// template<>
// struct VertexAttributes<InstanceData>
// {
//   static constexpr vk::VertexInputRate input_rate = vk::VertexInputRate::eInstance;     // This is per instance data.
//   static constexpr std::array<VertexAttribute, 2> attributes = {{
//     { Type::vec4, "InstanceData::m_position", offsetof(InstanceData, m_position) },
//     { Type::mat4, "InstanceData::m_matrix", offsetof(InstanceData, m_matrix) }
//   }};
// };
//
// } // namespace vulkan::shaderbuilder
//
// This can simply be done immediately below the struct.

template<typename ENTRY>
class VertexShaderInputSet : public VertexShaderInputSetBase
{
 public:
  // Constructor. Pass the input rate to the base class, extracting that info from ENTRY.
  VertexShaderInputSet() : VertexShaderInputSetBase(shaderbuilder::VertexAttributes<ENTRY>::input_rate) { }

 private:
  uint32_t size() const override final
  {
    return sizeof(ENTRY);
  }

  int next_batch() const override
  {
    // Default value.
    return 1;
  }

  virtual void create_entry(ENTRY* input_entry_ptr) = 0;

  void get_input_entry(void* input_entry_ptr) override final
  {
    create_entry(static_cast<ENTRY*>(input_entry_ptr));
  }
};

} // namespace vulkan::shaderbuilder
