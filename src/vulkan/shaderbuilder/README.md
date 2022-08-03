### Shader variable layouts ###

The user must define a template specialization of ShaderVariableLayouts
that reflects the layout of custom structs, before defining those structs.

Such definitions make use of predefined types for all basic types (scalars,
vectors and matrices and defined by GLSL). The exact definition of those
types depend on the chosen standard (scalar, std140 or std430).

One might think that it would be possible to include a namespace prior to
declaring the layout specialization, but since those are defined in a header
it is a bad idea to begin with a `using namespace std140;` or something
like that, and it is not allowed to do that inside a struct or class.

Therefore, the standard is declared by deriving from a base class
that defines the required types, but otherwise has a size of zero:

    struct MyFoo;
    namespace vulkan::shaderbuilder {

    template<>
    struct ShaderVariableLayouts<MyFoo> : standards::std140
    {
      MEMBER(Float, m_f),
      MEMBER(mat4, m_matrix)
    };

    } // namespace vulkan::shaderbuilder

where the class `standards::std140` defined the types `Float` and `mat4`
that were used. `MEMBER` is a macro that hides gory details.

Note that neither `Float` nor `mat4` are types that can contain a float or
a matrix of floats respectively. Instead, they are types that encode things
like alignment, size, array stride and offset (the latter if used in a
struct - like is the case here (`struct MyFoo`)).

The reason this information is encoded in a type is because it is used
to perform calculation in constexpr functions, and you can't pass constexpr
as arguments to functions. This means that the types need to be specializations
of template class, and specializations can only be added in the namespace
that defined it. In other words, the "returned" types cannot be defined in
their own namespace; they have to be specializations by standard themselves.

This is why -say- a `Float` (as used above) is really (derived from) a specialization of
`vulkan::shaderbuilder::standards::Layout`:

    namespace vulkan::shaderbuilder::standards {

    template<Standard standard, uint32_t alignment, uint32_t size, uint32_t array_stride, uint32_t offset>
    struct Layout;

And the basic types are specializations of

    template<ScalarIndex scalar_type, int rows, int cols,
             Standard standard, uint32_t alignment, uint32_t size, uint32_t array_stride, uint32_t offset>
    struct BasicType : Layout<standard, alignment, size, array_stride, offset>
    {
    };

### Shader preprocessing ###

The vulkan engine can generate shader variable declarations and then compile
the resulting shader. Before this preprocessing, the shader code is called
"template code".

Preprocessing only takes place if the template code contains one or more
identifiers with a double colon in it; such identifiers must have the form

    Class::variable

They must be registered by the application, or an error is thrown and the
application will terminate.

### Registration of shader variables ###

Shader variables are always members of a C++ struct, even if those struct
only have a single member. All members much have types as defined in "math/glsl.h".

For example,

    struct Class {
      glsl::Float var1;
      glsl::vec4  var2;
      glsl::mat3  var2;
    };

and a specialization of ShaderVariableLayouts must be added with the layout
of each member; for example,

    namespace vulkan::shaderbuilder {

    template<>
    struct ShaderVariableLayouts<Class>
    {
      static constexpr std::array<ShaderVariableLayout, 3> layouts = {{
        { Type::Float, "Class::var1", offsetof(Class, var1) },
        { Type::vec4,  "Class::var2", offsetof(Class, var2) },
        { Type::mat3,  "Class::var3", offsetof(Class, var2) }
      }};
    };

If struct contains vertex or instance attributes (suggested class names `VertexData`
and `InstanceData` respectivily) then the `ShaderVariableLayouts` specialization
also must contain a member `input_rate` that specifies the input rate: per vertex
or per instance.

For example,

    static constexpr vk::VertexInputRate input_rate = vk::VertexInputRate::eVertex;

for per vertex data, or

    static constexpr vk::VertexInputRate input_rate = vk::VertexInputRate::eInstance;

for per instance data.

### Registration of shader template code ###

Shader (template) code can be loaded from a file, or provided through a `string_view`
by passing it to the member function `vulkan::shaderbuilder::ShaderInfo::load`.

Registration of the resulting ShaderInfo objects must happen early; in most cases from an
override of the SynchronousWindow virtual function `register_shader_templates`.

For each shader a `vulkan::shaderbuilder::ShaderInfo` object must be constructed passing
a `vk::ShaderStageFlagBits` stage for which the shader will be used. A name for the shader,
for diagnostics, can be passed to the constructor or set after construction using the member
function `ShaderInfo::set_name`. Likewise, compiler flags may be passed to the constructor
or can be set after construction using the member function `ShaderInfo::set_compiler_options`.
Finally `ShaderInfo::load` must be called to set the template code.

After that the ShaderInfo object(s) can be registered with the application.

For example,

    vulkan::shaderbuilder::ShaderIndex m_shader_vert;
    vulkan::shaderbuilder::ShaderIndex m_shader_frag;

    void register_shader_templates() override
    {
      using namespace vulkan::shaderbuilder;
      std::vector<ShaderInfo> shader_info = {
        { vk::ShaderStageFlagBits::eVertex,   "uniform_buffer_controlled_triangle.vert.glsl" },
        { vk::ShaderStageFlagBits::eFragment, "uniform_buffer_controlled_triangle.frag.glsl" }
      };   
      shader_info[0].load(uniform_buffer_controlled_triangle_vert_glsl);
      shader_info[1].load(uniform_buffer_controlled_triangle_frag_glsl);
      auto indices = application().register_shaders(std::move(shader_info));
      m_shader_vert = indices[0];
      m_shader_frag = indices[1];
    }

The returned `ShaderIndex`s should be passed to `Pipeline::build_shader` when compiling
the shaders as part of a pipeline.

