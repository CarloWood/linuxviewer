#pragma once

#include "descriptor/SetKey.h"
#include <vulkan/vulkan.hpp>
#ifdef CWDEBUG
#include "debug/DebugSetName.h"
#endif
#include "debug.h"

namespace task {
class SynchronousWindow;
} // namespace task

namespace vulkan::shader_builder {
class ShaderResourceDeclaration;

namespace shader_resource {

// Base class for shader resources.
class Base
{
 private:
  descriptor::SetKey m_descriptor_set_key;

#ifdef CWDEBUG
  Ambifix m_ambifix;

 protected:
  // Implementation of virtual function of Base.
  Ambifix const& ambifix() const
  {
    return m_ambifix;
  }

 public:
  void add_ambifix(Ambifix const& ambifix) { m_ambifix = ambifix(m_ambifix.object_name()); }
#endif

 public:
  Base(descriptor::SetKey descriptor_set_key COMMA_CWDEBUG_ONLY(char const* debug_name)) :
    m_descriptor_set_key(descriptor_set_key) COMMA_CWDEBUG_ONLY(m_ambifix(debug_name)) { }
  Base() = default;     // Constructs a non-sensical m_descriptor_set_key that must be ignored when moving the object (in place).

  // Ignore m_descriptor_set_key when move- assigning and/or copying; the target should already have the right key.
  Base(Base&& rhs)
  {
  }
  Base& operator=(Base&& rhs)
  {
    return *this;
  }

  // Disallow copy and move constructing.
  Base(Base const& rhs) = delete;

  virtual void create(task::SynchronousWindow const* owning_window) = 0;
  virtual void update_descriptor_set(task::SynchronousWindow const* owning_window, vk::DescriptorSet vh_descriptor_set, uint32_t binding) = 0;

  // Accessor.
  descriptor::SetKey descriptor_set_key() const { return m_descriptor_set_key; }

#ifdef CWDEBUG
 public:
  virtual void print_on(std::ostream& os) const = 0;
#endif
};

} // namespace shader_resource
} // namespace vulkan::shader_builder
