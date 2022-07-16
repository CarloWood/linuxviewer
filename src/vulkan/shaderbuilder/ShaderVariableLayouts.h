#pragma once

namespace vulkan::shaderbuilder {

// This template struct must be specialized for each shader variable struct.
// See VertexShaderInputSet for more info and example.
template<typename ENTRY>
struct ShaderVariableLayouts
{
};

} // namespace vulkan::shaderbuilder
