#ifndef VULKAN_PIPELINE_FACTORY_HANDLE_H
#define VULKAN_PIPELINE_FACTORY_HANDLE_H

#include "Concepts.h"
#include "FactoryCharacteristicId.h"
#include "utils/Vector.h"
#include <boost/intrusive_ptr.hpp>

namespace vulkan::task {
class PipelineFactory;
class SynchronousWindow;
} // namespace vulkan::task

namespace vulkan::pipeline {

class FactoryHandle
{
 public:
  using PipelineFactoryIndex = utils::VectorIndex<boost::intrusive_ptr<task::PipelineFactory>>;

 private:
  PipelineFactoryIndex m_factory_index;

 public:
  FactoryHandle() = default;
  FactoryHandle(PipelineFactoryIndex factory_index) : m_factory_index(factory_index) { }

  template<ConceptPipelineCharacteristic CHARACTERISTIC, typename... ARGS>
  FactoryCharacteristicId add_characteristic(task::SynchronousWindow const* owning_window, ARGS&&... args);

  void generate(task::SynchronousWindow const* owning_window);

  friend bool operator==(FactoryHandle h1, FactoryHandle h2)
  {
    return h1.m_factory_index == h2.m_factory_index;
  }

  // Accessor.
  PipelineFactoryIndex factory_index() const { return m_factory_index; }
};

} // namespace vulkan::pipeline

#endif // VULKAN_PIPELINE_FACTORY_HANDLE_H

#ifndef VULKAN_SYNCHRONOUS_WINDOW_H
#include "SynchronousWindow.h"
#endif

#ifndef PIPELINE_PIPELINE_FACTORY_H
#include "PipelineFactory.h"
#endif

#ifndef VULKAN_PIPELINE_FACTORY_HANDLE_H_definitions
#define VULKAN_PIPELINE_FACTORY_HANDLE_H_definitions

namespace vulkan::pipeline {

template<ConceptPipelineCharacteristic CHARACTERISTIC, typename... ARGS>
FactoryCharacteristicId FactoryHandle::add_characteristic(task::SynchronousWindow const* owning_window, ARGS&&... args)
{
  DoutEntering(dc::vulkan, "vulkan::pipeline::FactoryHandle::add_characteristic<" <<
      ::NAMESPACE_DEBUG::type_name_of<CHARACTERISTIC>() <<
      ((LibcwDoutStream << ... << (std::string(", ") + ::NAMESPACE_DEBUG::type_name_of<ARGS>())), ">(") <<
      owning_window << ", " << join(", ", args...) << ")");
  CHARACTERISTIC* ptr = new CHARACTERISTIC(owning_window, std::forward<ARGS>(args)...);
  ptr->set_needs_signals();
  task::PipelineFactory* pipeline_factory = owning_window->pipeline_factory(m_factory_index);
  return pipeline_factory->add_characteristic(ptr);
}

} // namespace vulkan::pipeline

#endif // VULKAN_PIPELINE_FACTORY_HANDLE_H_definitions
