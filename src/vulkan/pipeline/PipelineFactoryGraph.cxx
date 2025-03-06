#include "sys.h"
#include "PipelineFactoryGraph.h"
#include "CharacteristicRange.h"
#include "../SynchronousWindow.h"

NAMESPACE_DEBUG_CHANNELS_START
channel_ct factorygraph("FACTORYGRAPH");
NAMESPACE_DEBUG_CHANNELS_END

namespace vulkan::pipeline {

//static
int PipelineFactoryGraph::get_fill_index(task::CharacteristicRange const* characteristic_ptr)
{
  return characteristic_ptr->fill_index();
}

//-----------------------------------------------------------------------------
// DescriptorSet

FrameResourceIndex PipelineFactoryGraph::DescriptorSet::number_of_frame_resources() const
{
  return m_window->number_of_frame_resources();
}

std::string PipelineFactoryGraph::DescriptorSet::id() const
{
  std::ostringstream oss;
  oss << "DescriptorSet" << m_frc_descriptor_set.as_key().vh_descriptor_set({});
  return oss.str();
}

std::string PipelineFactoryGraph::DescriptorSet::label() const
{
  std::string id = "DescriptorSet";
  if (m_frc_descriptor_set.is_frame_resource())
    for (FrameResourceIndex fri{0}; fri != number_of_frame_resources(); ++fri)
    {
      std::ostringstream oss;
      oss << "\n" << m_frc_descriptor_set[fri];
      id += oss.str();
    }
  else
  {
    std::ostringstream oss;
    oss << "\n" << m_frc_descriptor_set;
    id += oss.str();
  }
  return id;
}

//-----------------------------------------------------------------------------
// Pipeline

std::string PipelineFactoryGraph::Pipeline::id() const
{
  std::string id = "Pipeline" + std::to_string(m_pipeline_index.get_value());
  // Add the factory index to the id, otherwise it might not be unique.
  id += m_owning_factory->id_appendix();
  return id;
}

std::string PipelineFactoryGraph::Pipeline::label() const
{
  std::string label = "Pipeline" + std::to_string(m_pipeline_index.get_value());
  label += "_" + std::to_string(m_owning_factory->m_index.get_value());
  return label;
}

//-----------------------------------------------------------------------------
// Characteristic

std::string PipelineFactoryGraph::Characteristic::id(PipelineFactoryGraph const* graph) const
{
  std::string id = m_characteristic_type.substr(1 + m_characteristic_type.rfind(':'));
  // If the characteristic is used by more than one factory, add the factory index to the id.
  auto iter = graph->m_characteristic_type_to_factories.find(m_characteristic_type);
  ASSERT(iter != graph->m_characteristic_type_to_factories.end());
  auto const& factories = iter->second;
  if (factories.size() > 1)
    id += m_factory->id_appendix();
  return id;
}

std::string PipelineFactoryGraph::Characteristic::label() const
{
  std::string label = m_characteristic_type.substr(1 + m_characteristic_type.rfind(':'));
  label += "_" + std::to_string(m_factory->m_index.get_value());
  return label.length() > 15 ? split_label(label) : label;
}

//-----------------------------------------------------------------------------
// Factory

std::string PipelineFactoryGraph::Factory::id_appendix() const
{
  std::ostringstream oss;
  oss << "_" << m_index.get_value() << "_" << m_window;
  return oss.str();
}

std::string PipelineFactoryGraph::Factory::id() const
{
  // Factory_1_0x12345678, where '1' is the factory index and 0x12345678 the window pointer.
  return "Factory" + id_appendix();
}

std::string PipelineFactoryGraph::Factory::label(PipelineFactoryGraph const* graph) const
{
  std::string label = "Factory " + std::to_string(m_index.get_value());
  // If there are more than one window, append the window title to the label.
  if (graph->m_window_to_index_to_factory.size() > 1)
    label += " ("  + m_window_title + ")";
  // If there are more than one windows with the same title, add the window pointer too.
  auto iter = graph->m_window_to_title.find(m_window);
  // The window must be known.
  ASSERT(iter != graph->m_window_to_title.end());
  int count = 0;
  for (auto kvp : graph->m_window_to_title)
    if (kvp.second == iter->second)
      ++count;
  ASSERT(count > 0);
  if (count > 1)
  {
    std::ostringstream oss;
    oss << label << " " << m_window;
    label = oss.str();
  }
  return label;
}

size_t PipelineFactoryGraph::Factory::get_factory_pipeline_index(Index pipeline_index)
{
  for (size_t i = 0; i < m_pipelines.size(); ++i)
    if (m_pipelines[i].m_pipeline_index == pipeline_index)
      return i;
  ASSERT(false);
  AI_NEVER_REACHED
}

//-----------------------------------------------------------------------------
// DescriptorSetLayout

std::string PipelineFactoryGraph::DescriptorSetLayout::id() const
{
  std::ostringstream oss;
  oss << "DescriptorSetLayout_" << m_vh_descriptor_set_layout;
  return oss.str();
}

namespace {

std::string descriptor_type_abbreviation(vk::DescriptorType descriptor_type)
{
  switch (descriptor_type)
  {
    case vk::DescriptorType::eCombinedImageSampler:
      return "CIS";
    case vk::DescriptorType::eUniformBuffer:
      return "UB";
    default:
      ASSERT(false);
  }
  AI_NEVER_REACHED
}

void write_binding_type_stage_to(std::ostream& os, vk::DescriptorType descriptor_type,
    uint32_t descriptor_count, vk::ShaderStageFlags stage_flags)
{
  os << descriptor_type_abbreviation(descriptor_type);
  if (descriptor_count != 1)
    os << '[' << descriptor_count << "]";
  os << ' ';
  if ((stage_flags & vk::ShaderStageFlagBits::eVertex))
    os << 'V';
  if ((stage_flags & vk::ShaderStageFlagBits::eTessellationControl))
    os << "Tc";
  if ((stage_flags & vk::ShaderStageFlagBits::eTessellationEvaluation))
    os << "Te";
  if ((stage_flags & vk::ShaderStageFlagBits::eGeometry))
    os << 'G';
  if ((stage_flags & vk::ShaderStageFlagBits::eFragment))
    os << 'F';
  if ((stage_flags & vk::ShaderStageFlagBits::eCompute))
    os << 'C';
}

std::string set_index_color(descriptor::SetIndex set_index)
{
  if (set_index.get_value() == 0)
    return "color=blue";
  else if (set_index.get_value() == 1)
    return "color=cyan";
  else if (set_index.get_value() == 2)
    return "color=red";
  return {};
}

} // namespace

std::string PipelineFactoryGraph::DescriptorSetLayout::label() const
{
  std::ostringstream oss;
  oss << "DescriptorSetLayout\n" << m_vh_descriptor_set_layout;
  for (vk::DescriptorSetLayoutBinding descriptor_set_layout_binding : m_sorted_descriptor_set_layout_bindings)
  {
    oss << '\n' << descriptor_set_layout_binding.binding << ": ";
    write_binding_type_stage_to(oss, descriptor_set_layout_binding.descriptorType, descriptor_set_layout_binding.descriptorCount,
        descriptor_set_layout_binding.stageFlags);
  }
  return oss.str();
}

//-----------------------------------------------------------------------------
// ShaderTemplateCode

std::string PipelineFactoryGraph::ShaderTemplateCode::id() const
{
  return "ShaderTemplateCode" + std::to_string(m_hash);
}

std::string max_width(int width, std::string const& input)
{
  // If it fits already, just return the input.
  if (input.length() <= width)
    return input;

  enum SawLast { digit, lower, upper, neither };
  SawLast saw_last = neither;

  // Fill `weights` with a value of 10000 when breaking at that length break between two alpha characters,
  // and a value of 3 when it breaks between two digits, 0 otherwise.
  std::vector<int> weights;
  int const break_between_alpha_penalty = 10000;
  int const break_between_lower_and_upper_case = 1;
  int const break_between_digits_penalty = 3;
  for (int i = 0; i < input.length(); ++i)
  {
    char c = input[i];
    if (std::islower(c))
    {
      weights.push_back((saw_last == lower || saw_last == upper) ? break_between_alpha_penalty : 0);
      saw_last = lower;
    }
    else if (std::isupper(c))
    {
      weights.push_back(saw_last == upper ? break_between_alpha_penalty : ((saw_last == lower) ? break_between_lower_and_upper_case : 0));
      saw_last = upper;
    }
    else if (std::isdigit(c))
    {
      weights.push_back(saw_last == digit ? break_between_digits_penalty : 0);
      saw_last = digit;
    }
    else
    {
      weights.push_back(0);
      saw_last = neither;
    }
  }
  // Cutting after the last character also gets a penalty of zero.
  weights.push_back(0);

  // Break after pos characters. Find the lowest 'score'.
  int pos = width;
  int score = weights[width];

  if (score > 0)
  {
    // Try larger lengths, where each extra character beyond 'width' gives a penalty of 2.
    int dist = 1;
    for (int i = width + 1; i < weights.size(); ++i)
    {
      if (weights[i] + 2 * dist < score)
      {
        score = weights[i] + 2 * dist;
        pos = i;
        if (score <= 2 * (dist + 1))    // Score is already as low as it can go?
          break;
      }
      ++dist;
    }
    // Try smaller lengths, where each character less than 'width' gives a penalty of 1.
    dist = 1;
    for (int i = width - 1; i > 0; --i)
    {
      if (weights[i] + dist < score)
      {
        score = weights[i] + dist;
        pos = i;
        if (score <= dist + 1)
          break;
      }
      ++dist;
    }
  }

  // If we should break after the last character, then we're done.
  if (pos == input.size())
    return input;

  // Insert two backslashes and an 'n' after 'pos' characters,
  // and iterate for the remaining part.
  return input.substr(0, pos) + "\\n" + max_width(width, input.substr(pos));
}

std::string PipelineFactoryGraph::ShaderTemplateCode::label() const
{
  std::ostringstream oss;
  oss << max_width(15, m_shader_info.name()) << "\\n" << m_vh_shader_module;
  return oss.str();
}

std::string PipelineFactoryGraph::ShaderTemplateCode::fill_index_ranges(CharacteristicKey characteristic_key) const
{
  std::vector<int>& fill_indexes = m_fill_indexes_per_characteristic_index[characteristic_key];
  // Can this ever happen?
  ASSERT(!fill_indexes.empty());
  // Don't print '-1'.
  if (fill_indexes.size() == 1 && fill_indexes[0] == -1)
    return {};
  // I think it is already sorted... but well.
  std::sort(fill_indexes.begin(), fill_indexes.end());
  // Print something like 3-6,8-10,12,15
  std::stringstream oss;
  char const* prefix = "";
  int range_start = 0;
  int last_fill_index = -42;
  for (int fill_index : fill_indexes)
  {
    if (fill_index == last_fill_index + 1)
    {
      last_fill_index = fill_index;
      continue;
    }
    if (last_fill_index > range_start)
      oss << '-' << last_fill_index;
    oss << prefix << fill_index;
    prefix = ",";
    range_start = fill_index;
    last_fill_index = fill_index;
  }
  if (last_fill_index > range_start)
    oss << '-' << last_fill_index;
  return oss.str();
}

//-----------------------------------------------------------------------------
// ShaderResource

#if 0 // FIXME: doesn't compile
std::string PipelineFactoryGraph::ShaderResource::label() const
{
  std::ostringstream oss;
  oss << max_width(15, m_glsl_id) << "\\n";
  write_binding_type_stage_to(oss, m_shader_resource_declaration->descriptor_type(),
      (uint32_t)std::abs(m_shader_resource_declaration->descriptor_array_size()), m_shader_resource_declaration->stage_flags());
  return oss.str();
}

std::string PipelineFactoryGraph::ShaderResource::id() const
{
  return sanitize_for_graphviz(m_shader_resource_declaration->glsl_id());
}
#endif

//-----------------------------------------------------------------------------
// PipelineFactoryGraph

void PipelineFactoryGraph::add_factory(task::SynchronousWindow const* window, std::u8string const& window_title, PipelineFactoryIndex index)
{
  DoutEntering(dc::factorygraph, "add_factory(" << window << ", " << window_title << ", " << index << ")");

  // Update m_window_to_index_to_factory.
  auto ibp = m_window_to_index_to_factory.try_emplace(window);
  std::map<PipelineFactoryIndex, Factory>& index_to_factory = ibp.first->second;
  auto ibp2 = index_to_factory.try_emplace(index, window, window_title, index);
  // Don't add a factory with the same index twice (for the same window).
  ASSERT(ibp2.second);

  // Update m_window_to_title.
  std::string const& title = ibp2.first->second.m_window_title;
  auto ibp3 = m_window_to_title.try_emplace(window, title);
  // If window was seen before, it must have the same title as before.
  ASSERT(ibp3.first->second == title);
  // Different windows are not allowed to have the same title.
  int count = 0;
  for (auto const& p : m_window_to_title)
    if (p.second == title)
      ++count;
  ASSERT(count == 1);
}

PipelineFactoryGraph::Factory* PipelineFactoryGraph::get_factory(task::SynchronousWindow const* window, PipelineFactoryIndex index)
{
  auto index_to_factory_iter = m_window_to_index_to_factory.find(window);
  // Add the factory associated with {window, index} before using this function.
  ASSERT(index_to_factory_iter != m_window_to_index_to_factory.end());
  auto factory_iter = index_to_factory_iter->second.find(index);
  return &factory_iter->second;
}

void PipelineFactoryGraph::add_characteristic(task::SynchronousWindow const* window, PipelineFactoryIndex index,
    task::CharacteristicRange const* characteristic_ptr, std::string characteristic_type)
{
  DoutEntering(dc::factorygraph, "add_characteristic(" << window << ", " << index <<
      ", " << characteristic_ptr << ", " <<  characteristic_type << ")");

  Factory* factory = get_factory(window, index);

  // Update m_characteristic_type_to_factories.
  auto ibp = m_characteristic_type_to_factories.try_emplace(characteristic_type);
  std::vector<Factory const*>& factories = ibp.first->second;
  // Don't insert the same characteristic twice.
  ASSERT(std::find(factories.begin(), factories.end(), factory) == factories.end());
  factories.push_back(factory);

  // Update m_characteristics.
  factory->m_characteristics.emplace_back(factory, characteristic_type);
  m_characteristic_ptr_to_characteristic_index[characteristic_ptr] = factory->m_characteristics.size() - 1;
}

void PipelineFactoryGraph::add_pipeline(task::SynchronousWindow const* window, PipelineFactoryIndex index,
    Index pipeline_index, characteristics_container_t const& characteristics)
{
  DoutEntering(dc::factorygraph, "add_pipeline(" << window << ", " << index << ", " << pipeline_index << ", " << characteristics << ")");

  Factory* factory = get_factory(window, index);

  // Update m_pipelines.
  size_t factory_pipeline_index = factory->m_pipelines.size();
  factory->m_pipelines.emplace_back(factory, pipeline_index);

  for (auto const& characteristic_intrusive_ptr : characteristics)
  {
    task::CharacteristicRange const* characteristic_ptr = characteristic_intrusive_ptr.get();
    auto iter = m_characteristic_ptr_to_characteristic_index.find(characteristic_ptr);
    ASSERT(iter != m_characteristic_ptr_to_characteristic_index.end());
    size_t characteristic_index = iter->second;
    Characteristic& characteristic = factory->m_characteristics[characteristic_index];
    int fill_index = get_fill_index(characteristic_ptr);
    characteristic.add_pipeline(factory_pipeline_index, fill_index);
  }
}

void PipelineFactoryGraph::add_descriptor_sets(
    task::SynchronousWindow const* window,
    std::vector<descriptor::FrameResourceCapableDescriptorSet> const& descriptor_sets,
    std::vector<vk::DescriptorSetLayout> const& missing_descriptor_set_layouts,
    std::vector<std::pair<descriptor::SetIndex, bool>> const& set_index_has_frame_resource_pairs,
    std::vector<uint32_t> const& missing_descriptor_set_unbounded_descriptor_array_sizes)
{
  DoutEntering(dc::factorygraph, "add_descriptor_sets(" << descriptor_sets << ", " << missing_descriptor_set_layouts << ", " <<
      set_index_has_frame_resource_pairs << ", " << missing_descriptor_set_unbounded_descriptor_array_sizes << ")");

  size_t n = descriptor_sets.size();
  ASSERT(missing_descriptor_set_layouts.size() == n);
  ASSERT(set_index_has_frame_resource_pairs.size() == n);
  ASSERT(missing_descriptor_set_unbounded_descriptor_array_sizes.empty() || missing_descriptor_set_unbounded_descriptor_array_sizes.size() == n);

  for (int i = 0; i < n; ++i)
  {
    // Add the new DescriptorSet.
    auto ibp = m_descriptor_sets.insert({descriptor_sets[i], window});
    // This function is called for descriptor set handles that were just created,
    // therefore it is impossible that the handle was already added before.
    ASSERT(ibp.second);
    DescriptorSet const* descriptor_set = &*ibp.first;

    // Find existing DescriptorSetLayout with the same handle.
    auto descriptor_set_layout_iter =
      std::find_if(m_descriptor_set_layouts.begin(), m_descriptor_set_layouts.end(),
          [&](DescriptorSetLayout const& descriptor_set_layout) {
              return descriptor_set_layout.m_vh_descriptor_set_layout == missing_descriptor_set_layouts[i]; });
    // The DescriptorSetLayout should already have been added with add_descriptor_set_layout.
    ASSERT(descriptor_set_layout_iter != m_descriptor_set_layouts.end());
    // This function should be called for descriptor set handles that were just created.
    // Therefore it is impossible that the handle was already added before.
    ASSERT(std::find(descriptor_set_layout_iter->m_descriptor_sets.begin(), descriptor_set_layout_iter->m_descriptor_sets.end(),
          descriptor_set) == descriptor_set_layout_iter->m_descriptor_sets.end());

    // Add the pointer to the DescriptorSet to the corresponding DescriptorSetLayout.
    descriptor_set_layout_iter->m_descriptor_sets.push_back(descriptor_set);
  }
}

void PipelineFactoryGraph::add_descriptor_sets(task::SynchronousWindow const* window, PipelineFactoryIndex index, Index pipeline_index,
    descriptor_set_per_set_index_t const& descriptor_sets)
{
  DoutEntering(dc::factorygraph, "add_descriptor_sets(" << window << ", " << index << ", " << pipeline_index <<
      ", " << descriptor_sets << ")");

  Factory* factory = get_factory(window, index);
  Dout(dc::factorygraph, "factory = " << factory);
  size_t factory_pipeline_index = factory->get_factory_pipeline_index(pipeline_index);
  Dout(dc::factorygraph, "factory_pipeline_index = " << factory_pipeline_index);

  for (descriptor::SetIndex set_index = descriptor_sets.ibegin(); set_index != descriptor_sets.iend(); ++set_index)
  {
    descriptor::FrameResourceCapableDescriptorSet const& frc_descriptor_set = descriptor_sets[set_index];

    // Update m_descriptor_set_to_pipeline_set_index_objects.
    auto descriptor_set_iter = m_descriptor_sets.find(frc_descriptor_set);
    // The frc_descriptor_set should already have been added in the above add_descriptor_sets.
    ASSERT(descriptor_set_iter != m_descriptor_sets.end());
    descriptor_set_iter->m_pipeline_set_index_objects.emplace_back(factory, factory_pipeline_index, set_index);
  }
}

void PipelineFactoryGraph::add_descriptor_set_layout(vk::DescriptorSetLayout vh_descriptor_set_layout,
    std::vector<vk::DescriptorSetLayoutBinding> const& sorted_descriptor_set_layout_bindings)
{
  DoutEntering(dc::factorygraph, "add_descriptor_set_layout(" << vh_descriptor_set_layout <<
      ", " << sorted_descriptor_set_layout_bindings << ")");

#ifdef CW_DEBUG
  auto descriptor_set_layout_iter = std::find_if(m_descriptor_set_layouts.begin(), m_descriptor_set_layouts.end(),
      [&](DescriptorSetLayout const& descriptor_set_layout){
          return descriptor_set_layout.m_vh_descriptor_set_layout == vh_descriptor_set_layout; });
  // Don't call this function twice for the same vh_descriptor_set_layout.
  ASSERT(descriptor_set_layout_iter == m_descriptor_set_layouts.end());
#endif
  // Update m_descriptor_set_layouts.
  m_descriptor_set_layouts.emplace_back(vh_descriptor_set_layout, sorted_descriptor_set_layout_bindings);
}

void PipelineFactoryGraph::add_shader_template_codes(
    std::vector<shader_builder::ShaderInfo> const& new_shader_info_list,
    std::vector<std::size_t> const& hashes,
    std::vector<shader_builder::ShaderIndex> const& shader_indexes)
    /* threadsafe-*/const
{
  DoutEntering(dc::factorygraph, "add_shader_template_codes(" << new_shader_info_list << ", " << hashes << ")");

  size_t const number_of_new_shaders = new_shader_info_list.size();
  ASSERT(hashes.size() == number_of_new_shaders && shader_indexes.size() == number_of_new_shaders);
  for (int i = 0; i < number_of_new_shaders; ++i)
  {
    // Update m_hash_to_shader_template_code.
    auto ibp = m_hash_to_shader_template_code.try_emplace(hashes[i], new_shader_info_list[i], shader_indexes[i], hashes[i]);
    if (!ibp.second)
    {
      Dout(dc::factorygraph, "Duplicate hash: " << hashes[i] << " while adding " << new_shader_info_list[i] <<
          "; previously added " << ibp.first->second.m_shader_info);
    }
  }
}

void PipelineFactoryGraph::add_shader_module(vk::ShaderModule vh_shader_module, shader_builder::ShaderIndex shader_index)
{
  DoutEntering(dc::factorygraph, "add_shader_module(" << vh_shader_module << ", " << shader_index << ")");

  bool added = false;
  for (auto& hash_to_shader_template_code_pair : m_hash_to_shader_template_code)
  {
    ShaderTemplateCode& shader_template_code = hash_to_shader_template_code_pair.second;
    // This should never happen.
    ASSERT(shader_template_code.m_vh_shader_module != vh_shader_module);
    if (shader_template_code.m_shader_index == shader_index)
    {
      shader_template_code.m_vh_shader_module = vh_shader_module;
      ASSERT(!added);
      added = true;
    }
  }
  ASSERT(added);
}

PipelineFactoryGraph::ShaderTemplateCode* PipelineFactoryGraph::get_shader_template_code(shader_builder::ShaderIndex shader_index)
{
  ShaderTemplateCode* shader_template_code = nullptr;
  for (auto& hash_to_shader_template_code_pair : m_hash_to_shader_template_code)
  {
    shader_template_code = &hash_to_shader_template_code_pair.second;
    if (shader_template_code->m_shader_index == shader_index)
      break;
  }
  ASSERT(shader_template_code);
  return shader_template_code;
}

void PipelineFactoryGraph::add_characteristic_to_shader_edge(
    shader_builder::ShaderIndex shader_index, task::CharacteristicRange const* characteristic_range)
{
  DoutEntering(dc::factorygraph, "add_characteristic_to_shader_edge(" << shader_index << ", " << characteristic_range << ")");

  ShaderTemplateCode* shader_template_code = get_shader_template_code(shader_index);
  task::SynchronousWindow const* window = characteristic_range->owning_window();
  PipelineFactoryIndex pipeline_factory_index = characteristic_range->get_owning_factory()->pipeline_factory_index();
  Factory* factory = get_factory(window, pipeline_factory_index);
  size_t characteristic_index = m_characteristic_ptr_to_characteristic_index[characteristic_range];
  Characteristic& characteristic = factory->m_characteristics[characteristic_index];
  characteristic.m_shader_template_code_list.push_back(shader_template_code);
  auto ibp = shader_template_code->m_fill_indexes_per_characteristic_index.try_emplace({factory, characteristic_index});
  Dout(dc::factorygraph, "Adding fill_index " << characteristic_range->fill_index() << " to characteristic " << characteristic_index <<
      " of factory " << factory);
  //ASSERT(ibp.first->second.empty());
  ibp.first->second.push_back(characteristic_range->fill_index());
}

void PipelineFactoryGraph::add_shader_template_code_to_shader_resource_edge(
    shader_builder::ShaderIndex shader_index,
    shader_builder::ShaderVariable const* shader_variable,
    shader_builder::DeclarationContext const* declaration_context)
{
  DoutEntering(dc::factorygraph, "add_shader_template_code_to_shader_resource_edge(" <<
      shader_index << ", " << vk_utils::print_pointer(shader_variable) <<
      ", " << declaration_context << ")");

  shader_builder::ShaderResourceDeclarationContext const* shader_resource_declaration_context =
    dynamic_cast<shader_builder::ShaderResourceDeclarationContext const*>(declaration_context);

  if (shader_resource_declaration_context)
  {
    shader_builder::ShaderResourceVariable const* shader_resource1 =
      dynamic_cast<shader_builder::ShaderResourceVariable const*>(shader_variable);
    // If declaration_context is a shader_builder::ShaderResourceDeclarationContext then
    // shader_variable must be a shader_builder::ShaderResourceVariable.
    ASSERT(shader_resource1);

    // Create a ShaderResource that can be looked up by glsl_id.

    // For some reason the glsl_id string is stored in the shader_builder::ShaderResourceDeclaration
    // instead of shader_builder::ShaderResourceDeclarationContext.
    std::string glsl_id = shader_resource1->shader_resource_declaration_ptr()->glsl_id();
    // Update m_glsl_id_to_shader_resource.
#if 1 // FIXME: doesn't compile
    auto ibp = m_glsl_id_to_shader_resource.try_emplace(glsl_id, glsl_id, shader_resource_declaration_context);
    ShaderResource const* shader_resource = &ibp.first->second;

    // Create a ShaderResourceDeclaration and add it to the corresponding ShaderTemplateCode.

    ShaderTemplateCode* shader_template_code = get_shader_template_code(shader_index);
    // Update m_shader_resource_declarations.
    auto ibp2 = shader_template_code->m_shader_resource_declarations.emplace(
        shader_resource1->shader_resource_declaration_ptr(), shader_resource);
    if (!ibp2.second)
    {
      Dout(dc::notice, "Skipping duplicate.");
      return;
    }
    //ShaderResourceDeclaration const& shader_resource_declaration = *ibp2.first;
#endif
  }
}

void PipelineFactoryGraph::updated_descriptor_set(
    shader_builder::ShaderResourceDeclaration const* shader_resource_declaration,
    descriptor::FrameResourceCapableDescriptorSet const& frc_descriptor_set)
{
  DoutEntering(dc::factorygraph,
      "updated_descriptor_set(" << vk_utils::print_pointer(shader_resource_declaration) << ", " << frc_descriptor_set << ")");

  auto glsl_id_to_shader_resource_iter = m_glsl_id_to_shader_resource.find(shader_resource_declaration->glsl_id());
  // Should have been inserted by add_shader_template_code_to_shader_resource_edge.
  ASSERT(glsl_id_to_shader_resource_iter != m_glsl_id_to_shader_resource.end());
  ShaderResource const& shader_resource = glsl_id_to_shader_resource_iter->second;
  auto descriptor_set_iter = m_descriptor_sets.find(frc_descriptor_set);
  // Should have been inserted by add_descriptor_sets.
  ASSERT(descriptor_set_iter != m_descriptor_sets.end());
  DescriptorSet const& descriptor_set = *descriptor_set_iter;
  // Update m_shader_resources.
  descriptor_set.m_shader_resources.push_back(&shader_resource);
}

void PipelineFactoryGraph::generate_dot_file(std::filesystem::path const& filepath)
{
  std::vector<std::string> factory_ids;
  std::vector<std::string> characteristic_ids;
  std::vector<std::string> pipeline_ids;
  std::vector<std::string> descriptor_set_ids;
  std::vector<std::string> descriptor_set_layout_ids;
  std::vector<std::string> shader_template_code_ids;
  std::vector<std::string> shader_resource_ids;

  // Generate nodes and edges.
  struct Node
  {
    std::string id;
    std::string label;
    std::string attributes;
  };

  struct Edge
  {
    std::string from;
    std::string to;
    std::string label;
    std::string attributes;
  };
  std::vector<Node> nodes;
  std::vector<Edge> edges;

  for (auto const& window_to_index_to_factory_pair : m_window_to_index_to_factory)
    for (auto const& index_to_factory_pair : window_to_index_to_factory_pair.second)
    {
      Factory const& factory = index_to_factory_pair.second;
      std::string factory_id = factory.id();
      factory_ids.push_back(factory_id);
      std::string factory_label = factory.label(this);
      nodes.push_back({factory_id, factory_label, "shape=diamond"});

      for (size_t characteristic_index = 0; characteristic_index != factory.m_characteristics.size(); ++characteristic_index)
      {
        Characteristic const& characteristic = factory.m_characteristics[characteristic_index];
        // Factory ---> Characteristic
        edges.push_back({factory_id, characteristic.id(this)});
        for (ShaderTemplateCode const* shader_template_code : characteristic.m_shader_template_code_list)
          // Characteristic ---> ShaderTemplateCode
          edges.push_back({characteristic.id(this), shader_template_code->id(),
              shader_template_code->fill_index_ranges({&factory, characteristic_index}),
              "color=\"purple\", weight=0"});
      }

      for (auto const& characteristic : factory.m_characteristics)
      {
        std::string characteristic_id = characteristic.id(this);
        characteristic_ids.push_back(characteristic_id);
        std::string characteristic_label = characteristic.label();
        nodes.push_back({characteristic_id, characteristic_label, "shape=octagon, margin=0"});

        for (auto const& factory_pipeline_index_fill_index_pair : characteristic.m_factory_pipeline_index_fill_index_pairs)
        {
          size_t factory_pipeline_index = factory_pipeline_index_fill_index_pair.first;
          int fill_index = factory_pipeline_index_fill_index_pair.second;
          Pipeline const& pipeline = factory.m_pipelines[factory_pipeline_index];
          // Characteristic ---> Pipeline
          edges.push_back({characteristic_id, pipeline.id(), std::to_string(fill_index)});
        }
      }

      for (auto const& pipeline : factory.m_pipelines)
      {
        std::string pipeline_id = pipeline.id();
        pipeline_ids.push_back(pipeline_id);
        std::string pipeline_label = pipeline.label();
        nodes.push_back({pipeline_id, pipeline_label, "shape=cylinder, style=filled, fillcolor=orange"});
      }
    }
  for (DescriptorSet const& descriptor_set : m_descriptor_sets)
  {
    std::string descriptor_set_id = descriptor_set.id();
    descriptor_set_ids.push_back(descriptor_set_id);
    std::string descriptor_set_label = descriptor_set.label();
    nodes.push_back({descriptor_set_id, descriptor_set_label, "shape=folder"});

    std::set<descriptor::SetIndex> incoming_set_indices;

    for (PipelineSetIndex const& pipeline_set_index : descriptor_set.m_pipeline_set_index_objects)
    {
      Pipeline const& pipeline = pipeline_set_index.m_factory->m_pipelines[pipeline_set_index.m_factory_pipeline_index];
      std::ostringstream label;
      label << pipeline_set_index.m_set_index;
      // Pipeline ---> DescriptorSet
      edges.push_back({pipeline.id(), descriptor_set_id, label.str(), set_index_color(pipeline_set_index.m_set_index)});
      incoming_set_indices.insert(pipeline_set_index.m_set_index);
    }

#if 0 // FIXME : doesn't compile
    for (ShaderResource const* shader_resource : descriptor_set.m_shader_resources)
    {
      descriptor::SetIndex set_index = shader_resource->m_shader_resource_declaration_context->convert(
          shader_resource->m_shader_resource_declaration->set_index_hint());
      // DescriptorSet ---> ShaderResource.
      edges.push_back({descriptor_set.id(), shader_resource->id()});
    }
#endif
  }
  for (DescriptorSetLayout const& descriptor_set_layout : m_descriptor_set_layouts)
  {
    std::string descriptor_set_layout_id = descriptor_set_layout.id();
    descriptor_set_layout_ids.push_back(descriptor_set_layout_id);
    std::string descriptor_set_layout_label = descriptor_set_layout.label();
    nodes.push_back({descriptor_set_layout_id, descriptor_set_layout_label, "shape=tab, style=filled, fillcolor=green"});

    for (DescriptorSet const* descriptor_set : descriptor_set_layout.m_descriptor_sets)
      // DescriptorSetLayout ---> DescriptorSet
      edges.push_back({descriptor_set_layout.id(), descriptor_set->id(), "", "color=\"green\""});
  }
#if 0 // FIXME: doesn't compile
  for (auto const& hash_to_shader_template_code_pair : m_hash_to_shader_template_code)
  {
    ShaderTemplateCode const& shader_template_code = hash_to_shader_template_code_pair.second;
    std::string shader_template_code_id = shader_template_code.id();
    shader_template_code_ids.push_back(shader_template_code_id);
    std::string shader_template_code_label = shader_template_code.label();
    nodes.push_back({shader_template_code_id, shader_template_code_label, "shape=rectangle, color=blue, style=filled, fillcolor=yellow"});
    for (ShaderResource const* shader_resource : shader_template_code.m_shader_resources)
    {
      descriptor::SetIndex set_index = shader_resource->m_shader_resource_declaration_context->convert(
          shader_resource->m_shader_resource_declaration->set_index_hint());
      uint32_t binding = shader_resource->m_shader_resource_declaration_context->binding(
          shader_resource->m_shader_resource_declaration);
      std::ostringstream label;
      label << set_index << '.' << binding;
      // ShaderResource ---> ShaderTemplateCode
      edges.push_back({shader_resource->id(), shader_template_code.id(), label.str(), set_index_color(set_index)});
    }
  }
  for (auto const& glsl_id_to_shader_resource_pair : m_glsl_id_to_shader_resource)
  {
    ShaderResource const& shader_resource = glsl_id_to_shader_resource_pair.second;
    std::string shader_resource_id = shader_resource.id();
    shader_resource_ids.push_back(shader_resource_id);
    std::string shader_resource_label = shader_resource.label();
    nodes.push_back({shader_resource_id, shader_resource_label, "shape=ellipse, style=filled, fillcolor=gray"});
  }
#endif

  std::ofstream dot_file(filepath);
  dot_file << "digraph G {\n";

  // Write nodes.
  for (auto const& node : nodes)
  {
    dot_file << "    " << node.id << " [label=\"" << node.label << "\"";
    if (!node.attributes.empty())
      dot_file << ", " << node.attributes;
    dot_file << "]\n";
  }

  if (factory_ids.size() > 1)
  {
    // Write ranks.
    // { rank=same; Factory1; Factory2 }
    dot_file << "    " << "{ rank=min";
    for (auto const& factory_id : factory_ids)
      dot_file << "; " << factory_id;
    dot_file << " }\n";
  }
  if (characteristic_ids.size() > 1)
  {
    dot_file << "    " << "{ rank=100";
    for (auto const& characteristic_id : characteristic_ids)
      dot_file << "; " << characteristic_id;
    dot_file << " }\n";
  }
  if (pipeline_ids.size() > 1)
  {
    dot_file << "    " << "{ rank=200";
    for (auto const& pipeline_id : pipeline_ids)
      dot_file << "; " << pipeline_id;
    dot_file << " }\n";
  }
  if (descriptor_set_layout_ids.size() > 1)
  {
    dot_file << "    " << "{ rank=200";
    for (auto const& descriptor_set_layout_id : descriptor_set_layout_ids)
      dot_file << "; " << descriptor_set_layout_id;
    dot_file << " }\n";
  }
  if (descriptor_set_ids.size() > 1)
  {
    dot_file << "    " << "{ rank=300";
    for (auto const& descriptor_set_id : descriptor_set_ids)
      dot_file << "; " << descriptor_set_id;
    dot_file << " }\n";
  }
  if (shader_resource_ids.size() > 1)
  {
    dot_file << "    " << "{ rank=400";
    for (auto const& shader_resource_id : shader_resource_ids)
      dot_file << "; " << shader_resource_id;
    dot_file << " }\n";
  }
  if (shader_template_code_ids.size() > 1)
  {
    dot_file << "    " << "{ rank=max";
    for (auto const& shader_template_code_id : shader_template_code_ids)
      dot_file << "; " << shader_template_code_id;
    dot_file << " }\n";
  }

  // Write edges.
  for (auto const& edge : edges)
  {
    dot_file << "    " << edge.from << " -> " << edge.to;
    if (!edge.label.empty() || !edge.attributes.empty())
    {
      dot_file << " [";
      char const* prefix = "";
      if (!edge.label.empty())
      {
        dot_file << "label=\"" << edge.label << "\"";
        prefix = ", ";
      }
      if (!edge.attributes.empty())
        dot_file << prefix << edge.attributes;
      dot_file << "]";
    }
    dot_file << "\n";
  }

  dot_file << "}\n";
  dot_file.close();
}

} // namespace vulkan::pipeline
