#ifndef VULKAN_PIPELINE_FACTORY_HANDLE_H
#define VULKAN_PIPELINE_FACTORY_HANDLE_H

#include "Concepts.h"
#include "utils/Vector.h"
#include <boost/intrusive_ptr.hpp>

namespace task {
class PipelineFactory;
class SynchronousWindow;
} // namespace task

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
  void add_characteristic(task::SynchronousWindow const* owning_window, ARGS&&... args);

  void generate(task::SynchronousWindow const* owning_window);

  friend bool operator==(FactoryHandle h1, FactoryHandle h2)
  {
    return h1.m_factory_index == h2.m_factory_index;
  }
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
void FactoryHandle::add_characteristic(task::SynchronousWindow const* owning_window, ARGS&&... args)
{
  DoutEntering(dc::vulkan, "vulkan::pipeline::FactoryHandle::add_characteristic<" <<
      ::NAMESPACE_DEBUG::type_name_of<CHARACTERISTIC>() <<
      ((LibcwDoutStream << ... << (std::string(", ") + ::NAMESPACE_DEBUG::type_name_of<ARGS>())), ">(") <<
      owning_window << ", " << join(", ", args...) << ")");
  owning_window->pipeline_factory(m_factory_index)->add(new CHARACTERISTIC(std::forward<ARGS>(args)...));
}

} // namespace vulkan::pipeline

#endif // VULKAN_PIPELINE_FACTORY_HANDLE_H_definitions
