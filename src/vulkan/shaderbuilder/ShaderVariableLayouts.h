#pragma once

#include "ShaderVariableLayout.h"
#include <concepts>

namespace vulkan::shaderbuilder {

// All user data that is to be transfered to the GPU (vertex data with Vertex Attributes,
// PushConstants, UniformData) must be derived from one of the following:
// See the comment above VertexShaderInputSet for more info.
template<typename ENTRY>
concept ConceptEntry =
    std::derived_from<ENTRY, glsl::per_vertex_data> ||
    std::derived_from<ENTRY, glsl::per_instance_data> ||
    std::derived_from<ENTRY, glsl::uniform_std140> ||
    std::derived_from<ENTRY, glsl::push_constant_std430>;

// Every specialized ShaderVariableLayouts must be derived from this class.
template<ConceptEntry ENTRY>
struct ShaderVariableLayoutsTraits : ENTRY::tag_type
{
};

// This template struct must be specialized for each shader variable struct.
// See VertexShaderInputSet for more info and example.
template<typename ENTRY>
struct ShaderVariableLayouts
{
};

} // namespace vulkan::shaderbuilder
