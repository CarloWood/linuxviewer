#pragma once

#include "descriptor/SetKey.h"
#include <vulkan/vulkan.hpp>
#include "debug.h"

#ifdef CWDEBUG
namespace vulkan {
class Ambifix;
} // namespace vulkan
#endif

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
  // Mutable because we need to set this later.
  mutable ShaderResourceDeclaration const* m_declaration{};
  descriptor::SetKey m_descriptor_set_key;

 public:
  Base(descriptor::SetKey descriptor_set_key) : m_descriptor_set_key(descriptor_set_key) { }
  Base() = default;     // Constructs a non-sensical m_descriptor_set_key that must be ignored when moving the object (in place).

  // Ignore m_descriptor_set_key when move-assigning; the target should already have the right key.
  Base& operator=(Base&& rhs)
  {
    // Aren't we moving before calling set_declaration?
    ASSERT(!rhs.m_declaration);
    // m_declaration = rhs.m_declaration;
    return *this;
  }

  // Disallow copy and move constructing.
  Base(Base const& rhs) = delete;
  Base(Base&& rhs) = delete;

  // Not really const but this is called only once.
  void set_declaration(ShaderResourceDeclaration const* declaration) const
  {
    // Should only be called once.
    ASSERT(!m_declaration);
    m_declaration = declaration;
  }

#ifdef CWDEBUG
  virtual Ambifix const& ambifix() const = 0;
#endif
  virtual void create(task::SynchronousWindow const* owning_window) = 0;
  virtual void update_descriptor_set(task::SynchronousWindow const* owning_window, vk::DescriptorSet vh_descriptor_set, uint32_t binding) = 0;

  // Accessor.
  ShaderResourceDeclaration const* declaration() const { return m_declaration; }
  descriptor::SetKey descriptor_set_key() const { return m_descriptor_set_key; }

#ifdef CWDEBUG
 public:
  virtual void print_on(std::ostream& os) const = 0;
#endif
};

} // namespace shader_resource
} // namespace vulkan::shader_builder
