#pragma once

#include "Handle.h"
#include "../descriptor/FrameResourceCapableDescriptorSet.h"
#include "../descriptor/SetIndex.h"
#include "../shader_builder/ShaderInfo.h"
#include "../shader_builder/ShaderIndex.h"
#include <fstream>
#include <string>
#include <vector>
#include <map>
#include <filesystem>
#include <algorithm>
#include <regex>
#include "debug.h"

namespace vulkan::task {
class SynchronousWindow;
class CharacteristicRange;
} // namespace vulkan::task

namespace vulkan::shader_builder {
class ShaderVariable;
class DeclarationContext;
class ShaderResourceBase;
class ShaderResourceDeclaration;
class ShaderResourceDeclarationContext;
} // namespace vulkan::shader_builder

NAMESPACE_DEBUG_CHANNELS_START
extern channel_ct factorygraph;
NAMESPACE_DEBUG_CHANNELS_END

namespace vulkan::pipeline {
using CharacteristicRangeIndex = utils::VectorIndex<task::CharacteristicRange>;

class PipelineFactoryGraph
{
 private:
  using PipelineFactoryIndex = Handle::PipelineFactoryIndex;
  // The same as in vulkan::Pipeline.
  using descriptor_set_per_set_index_t = utils::Vector<descriptor::FrameResourceCapableDescriptorSet, descriptor::SetIndex>;
  // Same as in PipelineFactory.
  using characteristics_container_t = utils::Vector<boost::intrusive_ptr<task::CharacteristicRange>, CharacteristicRangeIndex>;

  static std::string sanitize_for_graphviz(std::string const& input)
  {
    std::string result = input;

    for (size_t i = 0; i < result.size(); ++i)
    {
      char c = result[i];

      // Graphviz "dot" node identifiers must start with a letter or underscore
      if (i == 0 && !std::isalpha(c) && c != '_')
          result[i] = '_';
      // Other characters in the identifier must be alphanumeric or underscores
      else if (i > 0 && !std::isalnum(c) && c != '_')
        result[i] = '_';
    }

    return result;
  }

  static std::string split_label(std::string label)
  {
    std::regex pattern("([a-z])([A-Z])");
    std::string replacement = "$1\\n$2";
    return std::regex_replace(label, pattern, replacement);
  }

  static int get_fill_index(task::CharacteristicRange const* characteristic_ptr);

  struct Factory;
  struct Characteristic;
  struct DescriptorSet;

  struct ShaderResource
  {
    std::string m_glsl_id;
    shader_builder::ShaderResourceDeclarationContext const* m_shader_resource_declaration_context;

    ShaderResource(std::string glsl_id, shader_builder::ShaderResourceDeclarationContext const* shader_resource_declaration_context) :
      m_glsl_id(glsl_id), m_shader_resource_declaration_context(shader_resource_declaration_context) { }

    std::string id() const;
    std::string label() const;
  };

  struct ShaderResourceDeclaration
  {
    shader_builder::ShaderResourceDeclaration const* m_shader_resource_declaration;
    ShaderResource const* m_shader_resource;

    ShaderResourceDeclaration(
        shader_builder::ShaderResourceDeclaration const* shader_resource_declaration,
        ShaderResource const* shader_resource) :
      m_shader_resource_declaration(shader_resource_declaration), m_shader_resource(shader_resource) { }

    friend bool operator<(ShaderResourceDeclaration const& lhs, ShaderResourceDeclaration const& rhs)
    {
      return lhs.m_shader_resource_declaration < rhs.m_shader_resource_declaration;
    }
  };

  struct CharacteristicKey
  {
    Factory const* m_factory;
    size_t m_characteristic_index;

    CharacteristicKey(Factory const* factory, size_t characteristic_index) : m_factory(factory), m_characteristic_index(characteristic_index) { }

    friend bool operator<(CharacteristicKey const& lhs, CharacteristicKey const& rhs)
    {
      if (lhs.m_factory != rhs.m_factory)
        return lhs.m_factory < rhs.m_factory;
      return lhs.m_characteristic_index < rhs.m_characteristic_index;
    }
  };

  struct ShaderTemplateCode
  {
    shader_builder::ShaderInfo m_shader_info;
    shader_builder::ShaderIndex m_shader_index;
    size_t m_hash;
    vk::ShaderModule m_vh_shader_module;
    mutable std::map<CharacteristicKey, std::vector<int>> m_fill_indexes_per_characteristic_index;
    // This is a std::set in order to avoid duplicate entries because preprocess1 can be called more than once for the same shader.
    std::set<ShaderResourceDeclaration> m_shader_resource_declarations;

    ShaderTemplateCode(shader_builder::ShaderInfo const& shader_info, shader_builder::ShaderIndex shader_index, size_t hash) :
      m_shader_info(shader_info), m_shader_index(shader_index), m_hash(hash) { }

    std::string id() const;
    std::string label() const;
    std::string fill_index_ranges(CharacteristicKey characteristic_key) const;
  };

  struct DescriptorSetLayout
  {
    vk::DescriptorSetLayout m_vh_descriptor_set_layout;
    std::vector<vk::DescriptorSetLayoutBinding> m_sorted_descriptor_set_layout_bindings;
    std::vector<DescriptorSet const*> m_descriptor_sets;

    DescriptorSetLayout(vk::DescriptorSetLayout vh_descriptor_set_layout,
        std::vector<vk::DescriptorSetLayoutBinding> const& sorted_descriptor_set_layout_bindings) :
      m_vh_descriptor_set_layout(vh_descriptor_set_layout), m_sorted_descriptor_set_layout_bindings(sorted_descriptor_set_layout_bindings)
    {
      // Re-sort m_sorted_descriptor_set_layout_bindings by binding number.
      std::sort(m_sorted_descriptor_set_layout_bindings.begin(), m_sorted_descriptor_set_layout_bindings.end(),
          [](vk::DescriptorSetLayoutBinding const& lhs, vk::DescriptorSetLayoutBinding const& rhs){ return lhs.binding < rhs.binding; });
    }

    friend bool operator<(DescriptorSetLayout const& lhs, DescriptorSetLayout const& rhs)
    {
      return lhs.m_vh_descriptor_set_layout < rhs.m_vh_descriptor_set_layout;
    }

    std::string id() const;
    std::string label() const;
  };

  struct PipelineSetIndex
  {
    Factory const* m_factory;           // The Factory that contains the vector for which...
    size_t m_factory_pipeline_index;    // ...this index gives us the Pipeline.
    descriptor::SetIndex m_set_index;   // The SetIndex used by that Pipeline.

    PipelineSetIndex(Factory const* factory, size_t factory_pipeline_index, descriptor::SetIndex set_index) :
      m_factory(factory), m_factory_pipeline_index(factory_pipeline_index), m_set_index(set_index) { }
  };

  struct DescriptorSet
  {
    descriptor::FrameResourceCapableDescriptorSet m_frc_descriptor_set;
    task::SynchronousWindow const* m_window;
    mutable std::vector<PipelineSetIndex> m_pipeline_set_index_objects;
    mutable std::vector<ShaderResource const*> m_shader_resources;

    DescriptorSet(descriptor::FrameResourceCapableDescriptorSet const& frc_descriptor_set, task::SynchronousWindow const* window) :
      m_frc_descriptor_set(frc_descriptor_set), m_window(window) { }

    // Construct a key object to be used to search m_descriptor_sets.
    DescriptorSet(descriptor::FrameResourceCapableDescriptorSet const& frc_descriptor_set) :
      m_frc_descriptor_set(frc_descriptor_set), m_window(nullptr) { }

    friend bool operator<(DescriptorSet const& lhs, DescriptorSet const& rhs)
    {
      return lhs.m_frc_descriptor_set.as_key() < rhs.m_frc_descriptor_set.as_key();
    }

    FrameResourceIndex number_of_frame_resources() const;

    std::string id() const;
    std::string label() const;
  };

  struct Pipeline
  {
    Factory const* m_owning_factory;
    Index m_pipeline_index;

    Pipeline(Factory const* owning_factory, Index pipeline_index) :
      m_owning_factory(owning_factory), m_pipeline_index(pipeline_index) { }

    std::string id() const;
    std::string label() const;
  };

  struct Characteristic
  {
    Factory const* m_factory;
    std::string m_characteristic_type;
    std::vector<std::pair<size_t, int>> m_factory_pipeline_index_fill_index_pairs;    // factory_pipeline_index, fill_index pair.
    std::vector<ShaderTemplateCode const*> m_shader_template_code_list;

    Characteristic(Factory const* factory, std::string characteristic_type) :
      m_factory(factory), m_characteristic_type(characteristic_type) { }

    void add_pipeline(size_t factory_pipeline_index, int fill_index)
    {
      m_factory_pipeline_index_fill_index_pairs.emplace_back(factory_pipeline_index, fill_index);
    }

    std::string id(PipelineFactoryGraph const* graph) const;
    std::string label() const;
  };

  struct Factory
  {
    task::SynchronousWindow const* m_window;
    PipelineFactoryIndex m_index;
    std::string m_window_title;
    std::vector<Characteristic> m_characteristics;
    std::vector<Pipeline> m_pipelines;

    Factory(task::SynchronousWindow const* window, std::u8string const& window_title, PipelineFactoryIndex index) :
      m_window(window), m_index(index),
      // Brute force hack to convert title into a normal string.
      m_window_title(window_title.cbegin(), window_title.cend()) { }

    std::string id_appendix() const;
    std::string id() const;
    std::string label(PipelineFactoryGraph const* graph) const;

    size_t get_factory_pipeline_index(Index pipeline_index);
  };

  std::map<task::SynchronousWindow const*, std::map<PipelineFactoryIndex, Factory>> m_window_to_index_to_factory;
  std::map<task::SynchronousWindow const*, std::string> m_window_to_title;
  std::map<std::string, std::vector<Factory const*>> m_characteristic_type_to_factories;
  std::map<task::CharacteristicRange const*, size_t> m_characteristic_ptr_to_characteristic_index;
  std::set<DescriptorSet> m_descriptor_sets;
  std::deque<DescriptorSetLayout> m_descriptor_set_layouts;
  mutable std::map<std::size_t, ShaderTemplateCode> m_hash_to_shader_template_code;
  std::map<std::string, ShaderResource> m_glsl_id_to_shader_resource;

 public:
  void add_factory(task::SynchronousWindow const* window, std::u8string const& window_title, PipelineFactoryIndex index);
  Factory* get_factory(task::SynchronousWindow const* window, PipelineFactoryIndex index);
  void add_characteristic(task::SynchronousWindow const* window, PipelineFactoryIndex index,
      task::CharacteristicRange const* characteristic_ptr, std::string characteristic_type);
  void add_pipeline(task::SynchronousWindow const* window, PipelineFactoryIndex index,
      Index pipeline_index, characteristics_container_t const& characteristics);
  void add_descriptor_sets(task::SynchronousWindow const* window,
      std::vector<descriptor::FrameResourceCapableDescriptorSet> const& descriptor_sets,
      std::vector<vk::DescriptorSetLayout> const& missing_descriptor_set_layouts,
      std::vector<std::pair<descriptor::SetIndex, bool>> const& set_index_has_frame_resource_pairs,
      std::vector<uint32_t> const& missing_descriptor_set_unbounded_descriptor_array_sizes);
  void add_descriptor_sets(task::SynchronousWindow const* window, PipelineFactoryIndex index, Index pipeline_index,
      descriptor_set_per_set_index_t const& descriptor_sets);
  void add_descriptor_set_layout(vk::DescriptorSetLayout vh_descriptor_set_layout,
      std::vector<vk::DescriptorSetLayoutBinding> const& sorted_descriptor_set_layout_bindings);
  void add_shader_template_codes(std::vector<shader_builder::ShaderInfo> const& new_shader_info_list,
      std::vector<std::size_t> const& hashes, std::vector<shader_builder::ShaderIndex> const& shader_indexes) /* threadsafe-*/const;
  void add_shader_module(vk::ShaderModule vh_shader_module, shader_builder::ShaderIndex shader_index);
  ShaderTemplateCode* get_shader_template_code(shader_builder::ShaderIndex shader_index);
  void add_characteristic_to_shader_edge(shader_builder::ShaderIndex shader_index, task::CharacteristicRange const* characteristic_range);
  void add_shader_template_code_to_shader_resource_edge(
      shader_builder::ShaderIndex shader_index,
      shader_builder::ShaderVariable const* shader_variable,
      shader_builder::DeclarationContext const* declaration_context);
  void updated_descriptor_set(shader_builder::ShaderResourceDeclaration const* shader_resource_declaration,
      descriptor::FrameResourceCapableDescriptorSet const& descriptor_set);
  void generate_dot_file(std::filesystem::path const& filepath);
};

} // namespace vulkan::pipeline
