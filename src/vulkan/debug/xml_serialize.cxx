#include "sys.h"

#ifdef CWDEBUG

#include "xml_serialize.h"
#include "xml/Bridge.h"
#include "pipeline/FlatCreateInfo.h"
#include "pipeline/FlatCreateInfo.inl.h"
#include <iomanip>

namespace xml {

template<>
void write_to_stream(std::ostream& os, VkShaderModule_T* const& shader_module)
{
  os << shader_module;
}

template<>
void read_from_stream(std::istream& is, VkShaderModule_T*& shader_module)
{
  is >> std::hex >> reinterpret_cast<size_t&>(shader_module);
}

// Instantiation of specializations of the serialize template function from xml/Bridge.h.

template<typename T>
void serialize(vk::Flags<T>& obj, Bridge& xml)
{
  // This specialization should be used for containers.
  xml.node_name("element");
  xml.text_stream(obj);
}

template<ConceptVkEnumWithToString T>
void serialize(T& obj, Bridge& xml)
{
  // This specialization should be used for containers.
  xml.node_name("element");
  xml.text_stream(obj);
}

template<typename T>
void serialize(std::vector<T>& obj, Bridge& xml)
{
  // This specialization should be used for containers.
  xml.node_name("element");
  xml.children("vector", obj);
}

template<>
void serialize(vk::ShaderModule& shader_module, Bridge& xml)
{
  xml.node_name("shader_module");

  VkShaderModule handle = shader_module;
  if (xml.writing())
    xml.child_stream("handle", handle);
}

template<>
void serialize(vk::StencilOpState& obj, Bridge& xml)
{
  xml.node_name("StencilOpState");

  xml.child_stream("failOp",      obj.failOp,      vk::StencilOp::eKeep,  false);
  xml.child_stream("passOp",      obj.passOp,      vk::StencilOp::eKeep,  false);
  xml.child_stream("depthFailOp", obj.depthFailOp, vk::StencilOp::eKeep,  false);
  xml.child_stream("compareOp",   obj.compareOp,   vk::CompareOp::eNever, false);
  xml.child_stream("compareMask", obj.compareMask, {},                    false);
  xml.child_stream("writeMask",   obj.writeMask,   {},                    false);
  xml.child_stream("reference",   obj.reference,   {},                    false);
}

template<>
void serialize(vk::PipelineInputAssemblyStateCreateInfo& obj, Bridge& xml)
{
  xml.node_name("PipelineInputAssemblyStateCreateInfo");

  // Not implemented.
  ASSERT(!obj.pNext);
  xml.child_stream("flags",                  obj.flags,                  {}, false);
  xml.child_stream("topology",               obj.topology);
  xml.child_stream("primitiveRestartEnable", obj.primitiveRestartEnable, {}, false);
}

template<>
void serialize(vk::PipelineViewportStateCreateInfo& obj, Bridge& xml)
{
  xml.node_name("PipelineViewportStateCreateInfo");

  // Not implemented.
  ASSERT(!obj.pNext);
  xml.child_stream("flags",         obj.flags,         {},          false);
  xml.child_stream("viewportCount", obj.viewportCount, uint32_t{1}, false);
  // Not implemented, we're assuming dynamic state.
  ASSERT(!obj.pViewports);
  xml.child_stream("scissorCount",  obj.scissorCount,  uint32_t{1}, false);
  // Not implemented, we're assuming dynamic state.
  ASSERT(!obj.pScissors);
}

template<>
void serialize(vk::PipelineRasterizationStateCreateInfo& obj, Bridge& xml)
{
  xml.node_name("PipelineRasterizationStateCreateInfo");

  xml.child_stream("depthClampEnable",            obj.depthClampEnable,        VK_FALSE,                         false);
  xml.child_stream("rasterizerDiscardEnable",     obj.rasterizerDiscardEnable, VK_FALSE,                         false);
  xml.child_stream("polygonMode",                 obj.polygonMode,             vk::PolygonMode::eFill,           false);
  xml.child_stream<vk::CullModeFlags>("cullMode", obj.cullMode,                vk::CullModeFlagBits::eBack,      false);
  xml.child_stream("frontFace",                   obj.frontFace,               vk::FrontFace::eCounterClockwise, false);
  xml.child_stream("depthBiasEnable",             obj.depthBiasEnable,         VK_FALSE,                         false);
  xml.child_stream("depthBiasConstantFactor",     obj.depthBiasConstantFactor, 0.0f,                             false);
  xml.child_stream("depthBiasClamp",              obj.depthBiasClamp,          0.0f,                             false);
  xml.child_stream("depthBiasSlopeFactor",        obj.depthBiasSlopeFactor,    0.0f,                             false);
  xml.child_stream("lineWidth",                   obj.lineWidth,               1.0f,                             false);
}

template<>
void serialize(vk::PipelineMultisampleStateCreateInfo& obj, Bridge& xml)
{
  xml.node_name("PipelineMultisampleStateCreateInfo");

  xml.child_stream("rasterizationSamples",  obj.rasterizationSamples,  vk::SampleCountFlagBits::e1, false);
  xml.child_stream("sampleShadingEnable",   obj.sampleShadingEnable,   VK_FALSE,                    false);
  xml.child_stream("minSampleShading",      obj.minSampleShading,      1.0f,                        false);
  // Not implemented.
  ASSERT(!obj.pSampleMask);
  xml.child_stream("alphaToCoverageEnable", obj.alphaToCoverageEnable, VK_FALSE,                    false);
  xml.child_stream("alphaToOneEnable",      obj.alphaToOneEnable,      VK_FALSE,                    false);
}

template<>
void serialize(vk::PipelineDepthStencilStateCreateInfo& obj, Bridge& xml)
{
  xml.node_name("PipelineDepthStencilStateCreateInfo");

  xml.child_stream("depthTestEnable",       obj.depthTestEnable,       VK_TRUE,                     false);
  xml.child_stream("depthWriteEnable",      obj.depthWriteEnable,      VK_TRUE,                     false);
  xml.child_stream("depthCompareOp",        obj.depthCompareOp,        vk::CompareOp::eLessOrEqual, false);
  xml.child_stream("depthBoundsTestEnable", obj.depthBoundsTestEnable, {},                          false);
  xml.child_stream("stencilTestEnable",     obj.stencilTestEnable,     {},                          false);
  xml.child(obj.front);
  xml.child(obj.back);
  xml.child_stream("minDepthBounds",        obj.minDepthBounds,        {},                          false);
  xml.child_stream("maxDepthBounds",        obj.maxDepthBounds,        {},                          false);
}

template<>
void serialize(vk::PipelineColorBlendStateCreateInfo& obj, Bridge& xml)
{
  xml.node_name("PipelineColorBlendStateCreateInfo");
  xml.child_stream("logicOpEnable",     obj.logicOpEnable, VK_FALSE,           false);
  xml.child_stream("logicOp",           obj.logicOp,       vk::LogicOp::eCopy, false);
  xml.children_stream("blendConstants", obj.blendConstants, assign);
}

template<>
void serialize(vk::PipelineShaderStageCreateInfo& obj, Bridge& xml)
{
  xml.node_name("PipelineShaderStageCreateInfo");

  xml.child_stream("stage", obj.stage, vk::ShaderStageFlagBits{}, false);
  xml.child(obj.module);
  if (xml.writing())
  {
    std::string name(obj.pName);
    xml.child_stream<std::string>("pName", name, "main", false);
  }
  else
  {
    std::string name;
    xml.child_stream("pName", name);
    // Can't pass the name back so it has to be equal the default.
    ASSERT(name == obj.pName);
  }
  // Not implemented.
  ASSERT(!obj.pSpecializationInfo);
}

template<>
void serialize(vk::VertexInputBindingDescription& obj, Bridge& xml)
{
  xml.node_name("VertexInputBindingDescription");

  xml.child_stream("binding",   obj.binding);
  xml.child_stream("stride",    obj.stride);
  xml.child_stream("inputRate", obj.inputRate);
}

template<>
void serialize(vk::VertexInputAttributeDescription& obj, Bridge& xml)
{
  xml.node_name("VertexInputAttributeDescription");

  xml.child_stream("location", obj.location);
  xml.child_stream("binding",  obj.binding);
  xml.child_stream("format",   obj.format);
  xml.child_stream("offset",   obj.offset);
}

template<>
void serialize(vk::PipelineColorBlendAttachmentState& obj, Bridge& xml)
{
  xml.node_name("PipelineColorBlendAttachmentState");

  xml.child_stream("blendEnable",         obj.blendEnable,         VK_FALSE,               false);
  xml.child_stream("srcColorBlendFactor", obj.srcColorBlendFactor, vk::BlendFactor::eOne,  false);
  xml.child_stream("dstColorBlendFactor", obj.dstColorBlendFactor, vk::BlendFactor::eZero, false);
  xml.child_stream("colorBlendOp",        obj.colorBlendOp,        vk::BlendOp::eAdd,      false);
  xml.child_stream("srcAlphaBlendFactor", obj.srcAlphaBlendFactor, vk::BlendFactor::eOne,  false);
  xml.child_stream("dstAlphaBlendFactor", obj.dstAlphaBlendFactor, vk::BlendFactor::eZero, false);
  xml.child_stream("alphaBlendOp",        obj.alphaBlendOp,        vk::BlendOp::eAdd,      false);
  xml.child_stream("colorWriteMask",      obj.colorWriteMask,      vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA, false);
}

} // namespace xml

namespace vulkan::pipeline {

void FlatCreateInfo::xml(xml::Bridge& xml)
{
  xml.node_name("FlatCreateInfo");
  xml.children("m_pipeline_shader_stage_create_infos_list", *pipeline_shader_stage_create_infos_list_t::wat(m_pipeline_shader_stage_create_infos_list));
  xml.children("m_vertex_input_binding_descriptions_list", *vertex_input_binding_descriptions_list_t::wat(m_vertex_input_binding_descriptions_list));
  xml.children("m_vertex_input_attribute_descriptions_list", *vertex_input_attribute_descriptions_list_t::wat(m_vertex_input_attribute_descriptions_list));
  xml.children("m_pipeline_color_blend_attachment_states_list", *pipeline_color_blend_attachment_states_list_t::wat(m_pipeline_color_blend_attachment_states_list));
  xml.children("m_dynamic_states_list", *dynamic_states_list_t::wat(m_dynamic_states_list));
  xml.children("m_push_constant_ranges_list", *push_constant_ranges_list_t::wat(m_push_constant_ranges_list));
  // Serialize vulkan types with calls to specialized xml::serialize.
  xml.child(m_pipeline_input_assembly_state_create_info);
  xml.child(m_viewport_state_create_info);
  xml.child(m_rasterization_state_create_info);
  xml.child(m_multisample_state_create_info);
  xml.child(m_depth_stencil_state_create_info);
  xml.child(m_color_blend_state_create_info);
}

} // namespace vulkan::pipeline

#endif // CWDEBUG
