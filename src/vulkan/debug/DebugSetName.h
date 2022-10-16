#ifndef DEBUG_SET_NAME_DECLARATION_H
#define DEBUG_SET_NAME_DECLARATION_H

#ifdef CWDEBUG
#include "Concepts.h"
#include <string>
#include <cstddef>

class AIStatefulTask;

namespace task {
class SynchronousWindow;
} // namespace task

namespace vulkan {

class LogicalDevice;

template<typename Ambifix>
concept ConceptAmbifix = std::same_as<std::string, decltype(Ambifix::m_prefix)> && std::same_as<std::string, decltype(Ambifix::m_postfix)>;

class Ambifix
{
 protected:
  std::string m_prefix;
  std::string m_postfix;

 public:
  Ambifix() = default;
  Ambifix(std::string&& prefix) : m_prefix(std::move(prefix)) { }
  Ambifix(std::nullptr_t, std::string&& postfix) : m_postfix(std::move(postfix)) { }
  Ambifix(std::string&& prefix, std::string&& postfix) : m_prefix(std::move(prefix)), m_postfix(std::move(postfix)) { }

  // Append prefix.
  template<ConceptAmbifix Ambifix>
  friend Ambifix operator+(std::string const& prefix, Ambifix ambifix)
  {
    ambifix.m_prefix += prefix;
    return ambifix;
  }

  // Prepend postfix.
  template<ConceptAmbifix Ambifix>
  friend Ambifix operator+(Ambifix ambifix, std::string postfix)
  {
    postfix += ambifix.m_postfix;
    ambifix.m_postfix = std::move(postfix);
    return ambifix;
  }

  // Accessors.

  std::string const& prefix() const { return m_prefix; }
  std::string const& postfix() const { return m_postfix; }
  // Add infix and return string.
  std::string object_name(std::string infix) const { return m_prefix + infix + m_postfix; }
  // Catenate prefix and postfix and return string.
  std::string object_name() const { return prefix() + postfix(); }
};

std::string as_postfix(void const* task, char const* open_bracket = "[", char const* close_bracket = "]");

class AmbifixOwner : public Ambifix
{
  LogicalDevice const* m_logical_device;

 public:
  AmbifixOwner(task::SynchronousWindow const* owning_window, std::string prefix);
  AmbifixOwner(vulkan::LogicalDevice const* owning_logical_device, std::string prefix);

  // Accessors.
  LogicalDevice const* logical_device() const { return m_logical_device; }
};

template<ConceptVulkanHandle ObjectType>
void debug_set_object_name(ObjectType const& object, std::string const& name, LogicalDevice const* logical_device);

template<ConceptVulkanHandle ObjectType>
void debug_set_object_name(ObjectType const& object, AmbifixOwner const& ambifix);

template<ConceptVulkanHandle ObjectType>
void debug_set_object_name(ObjectType const& object, Ambifix const& ambifix, LogicalDevice const* logical_device);

template<ConceptUniqueVulkanHandle UniqueObjectType>
void debug_set_object_name(UniqueObjectType const& object, AmbifixOwner const& ambifix);

template<ConceptUniqueVulkanHandle UniqueObjectType>
void debug_set_object_name(UniqueObjectType const& object, Ambifix const& ambifix, LogicalDevice const* logical_device);

} // namespace vulkan

#define DebugSetName(...) do { using ::vulkan::as_postfix; using std::to_string; ::vulkan::debug_set_object_name(__VA_ARGS__); } while(0)

#else // CWDEBUG
#define DebugSetName(...) do { } while(0)
#endif // CWDEBUG

#endif // DEBUG_SET_NAME_DECLARATION_H

#ifdef CWDEBUG
#ifndef VULKAN_LOGICAL_DEVICE_H
#include "LogicalDevice.h"
#endif

#ifndef DEBUG_SET_NAME_DEFINITIONS_H
#define DEBUG_SET_NAME_DEFINITIONS_H

namespace vulkan {

template<ConceptVulkanHandle ObjectType>
void debug_set_object_name(ObjectType const& object, std::string const& name, LogicalDevice const* logical_device)
{
  vk::DebugUtilsObjectNameInfoEXT name_info = {
    .objectType = object.objectType,
    .objectHandle = reinterpret_cast<uint64_t>(static_cast<typename ObjectType::CType>(object)),
    .pObjectName = name.c_str()
  };
  logical_device->set_debug_name(name_info);
  Dout(dc::vulkan, "Created object \"" << name << "\" with handle 0x" << std::hex << name_info.objectHandle << " and type " << libcwd::type_info_of<ObjectType>().demangled_name());
  if (name.substr(0, 26) == "m_imgui.m_descriptor_set [")
    Debug(attach_gdb());
}

template<ConceptVulkanHandle ObjectType>
void debug_set_object_name(ObjectType const& object, AmbifixOwner const& ambifix)
{
  debug_set_object_name(object, ambifix.object_name(), ambifix.logical_device());
}

template<ConceptVulkanHandle ObjectType>
void debug_set_object_name(ObjectType const& object, Ambifix const& ambifix, LogicalDevice const* logical_device)
{
  debug_set_object_name(object, ambifix.object_name(), logical_device);
}

template<ConceptUniqueVulkanHandle UniqueObjectType>
void debug_set_object_name(UniqueObjectType const& object, AmbifixOwner const& ambifix)
{
  debug_set_object_name(*object, ambifix.object_name(), ambifix.logical_device());
}

template<ConceptUniqueVulkanHandle UniqueObjectType>
void debug_set_object_name(UniqueObjectType const& object, Ambifix const& ambifix, LogicalDevice const* logical_device)
{
  debug_set_object_name(*object, ambifix.object_name(), logical_device);
}

} // namespace vulkan

#endif // DEBUG_SET_NAME_DEFINITIONS_H
#endif // CWDEBUG
