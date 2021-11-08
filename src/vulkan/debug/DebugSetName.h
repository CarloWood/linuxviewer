#ifdef CWDEBUG

#ifndef DEBUG_SET_NAME_DECLARATION_H
#define DEBUG_SET_NAME_DECLARATION_H

#include "Concepts.h"
#include <string>
#include <cstddef>

class AIStatefulTask;

namespace task {
class VulkanWindow;
} // namespace task

namespace vulkan {

class LogicalDevice;

class Ambifix
{
 protected:
  std::string m_prefix;
  std::string m_postfix;

 public:
  Ambifix() { }
  Ambifix(std::string&& prefix) : m_prefix(std::move(prefix)) { }
  Ambifix(std::nullptr_t, std::string&& postfix) : m_postfix(std::move(postfix)) { }
  Ambifix(std::string&& prefix, std::string&& postfix) : m_prefix(std::move(prefix)), m_postfix(std::move(postfix)) { }

  // Append prefix.
  friend Ambifix operator+(std::string const& prefix, Ambifix ambifix)
  {
    ambifix.m_prefix += prefix;
    return ambifix;
  }

  // Prepend postfix.
  friend Ambifix operator+(Ambifix ambifix, std::string postfix)
  {
    postfix += ambifix.m_postfix;
    ambifix.m_postfix = std::move(postfix);
    return ambifix;
  }

  // Accessors.
  std::string const& prefix() const { return m_prefix; }
  std::string const& postfix() const { return m_postfix; }

  std::string operator()(std::string infix) const
  {
    return m_prefix + infix + m_postfix;
  }
};

std::string as_postfix(AIStatefulTask const* task);

class AmbifixOwner : public Ambifix
{
  task::VulkanWindow const* m_owning_window;
  LogicalDevice const* m_logical_device;

 public:
  AmbifixOwner(task::VulkanWindow const* owning_window, std::string prefix);

  // Accessors.
  LogicalDevice const* logical_device() const { return m_logical_device; }

  // No infix.
  std::string object_name() const { return prefix() + postfix(); }

  // With infix.
  AmbifixOwner operator()(std::string infix) const
  {
    AmbifixOwner result(*this);
    // Cheat a bit here. Just add it to the prefix;
    result.m_prefix += infix;
    return result;
  }
};

template<ConceptVulkanHandle ObjectType>
void debug_set_object_name(ObjectType const& object, std::string const& name, LogicalDevice const* device);

template<ConceptVulkanHandle ObjectType>
void debug_set_object_name(ObjectType const& object, AmbifixOwner const& ambifix);

template<ConceptUniqueVulkanHandle UniqueObjectType>
void debug_set_object_name(UniqueObjectType const& object, AmbifixOwner const& ambifix);

} // namespace vulkan

#define DebugSetName(...) do { using ::vulkan::as_postfix; using std::to_string; ::vulkan::debug_set_object_name(__VA_ARGS__); } while(0)

#include "LogicalDevice.h"
#endif // DEBUG_SET_NAME_DECLARATION_H

#ifndef DEBUG_SET_NAME_DEFINITIONS_H
#define DEBUG_SET_NAME_DEFINITIONS_H

namespace vulkan {

template<ConceptVulkanHandle ObjectType>
void debug_set_object_name(ObjectType const& object, std::string const& name, LogicalDevice const* device)
{
  vk::DebugUtilsObjectNameInfoEXT name_info = {
    .objectType = object.objectType,
    .objectHandle = reinterpret_cast<uint64_t>(static_cast<typename ObjectType::CType>(object)),
    .pObjectName = name.c_str()
  };
  device->set_debug_name(name_info);
}

template<ConceptVulkanHandle ObjectType>
void debug_set_object_name(ObjectType const& object, AmbifixOwner const& ambifix)
{
  debug_set_object_name(object, ambifix.object_name(), ambifix.logical_device());
}

template<ConceptUniqueVulkanHandle UniqueObjectType>
void debug_set_object_name(UniqueObjectType const& object, AmbifixOwner const& ambifix)
{
  debug_set_object_name(*object, ambifix.object_name(), ambifix.logical_device());
}

} // namespace vulkan

#endif // DEBUG_SET_NAME_DEFINITIONS_H

#else // CWDEBUG
#define DebugSetName(...) do { } while(0)
#endif // CWDEBUG