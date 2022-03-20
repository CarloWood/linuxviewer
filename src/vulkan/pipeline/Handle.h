#ifndef VULKAN_PIPELINE_HANDLE_H
#define VULKAN_PIPELINE_HANDLE_H

#include "Concepts.h"
#include "utils/Vector.h"
#include <boost/intrusive_ptr.hpp>

namespace task {
class PipelineFactory;
class SynchronousWindow;
} // namespace task

namespace vulkan::pipeline {

class Handle
{
 public:
  using PipelineFactoryIndex = utils::VectorIndex<boost::intrusive_ptr<task::PipelineFactory>>;

 private:
  PipelineFactoryIndex m_factory_index;

 public:
  Handle(PipelineFactoryIndex factory_index) : m_factory_index(factory_index) { }

  template<ConceptPipelineCharacteristic CHARACTERISTIC, typename... ARGS>
  void add_characteristic(task::SynchronousWindow const* owning_window, ARGS&&... args);

  void generate(task::SynchronousWindow const* owning_window);
};

} // namespace vulkan::pipeline

#include "SynchronousWindow.h"
#endif // VULKAN_PIPELINE_HANDLE_H

#ifndef VULKAN_PIPELINE_HANDLE_defs_H
#define VULKAN_PIPELINE_HANDLE_defs_H

namespace vulkan::pipeline {

template<ConceptPipelineCharacteristic CHARACTERISTIC, typename... ARGS>
void Handle::add_characteristic(task::SynchronousWindow const* owning_window, ARGS&&... args)
{
  DoutEntering(dc::vulkan, "vulkan::pipeline::Handle::add_characteristic<" <<
      ::NAMESPACE_DEBUG::type_name_of<CHARACTERISTIC>() <<
      ((LibcwDoutStream << ... << (std::string(", ") + ::NAMESPACE_DEBUG::type_name_of<ARGS>())), ">(") <<
      owning_window << ", " << join(", ", args...) << ")");
  owning_window->pipeline_factory(m_factory_index)->add(new CHARACTERISTIC(std::forward<ARGS>(args)...));
}

} // namespace vulkan::pipeline

#endif // VULKAN_PIPELINE_HANDLE_defs_H
