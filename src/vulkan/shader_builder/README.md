### Shader variable layouts ###

The user must define a template specialization of ShaderVariableLayouts
that reflects the layout of custom structs, before defining those structs.

Such definitions make use of predefined types for all basic types (scalars,
vectors and matrices, defined by GLSL). The memory layout of those structs
depend on the chosen standard (scalar, std140 or std430; note that scalar
is part of GL_EXT_scalar_block_layout).

One might think that it would be possible to include a namespace prior to
declaring the layout specialization, but since those are defined in a header
it is a bad idea to begin with a `using namespace std140;` or something
like that, and it is not allowed to do that inside a struct or class.

Therefore, the standard is declared by deriving from a base class
that defines the required types, but otherwise has a size of zero:

    struct MyFoo;

    template<>
    struct vulkan_shader_builder_specialization_classes::ShaderVariableLayoutsBase<classname>
    {
      using containing_class = classname;
      using glsl_standard = glsl::standard;
      static constexpr ...
        .
        . (other static constexpr's)
        .
    };

    template<>
    vulkan_shader_builder_specialization_classes::ShaderVariableLayouts<MyFoo> : glsl::uniform_std140,
        vulkan_shader_builder_specialization_classes::ShaderVariableLayoutsBase<classname>
    {
      static constexpr auto struct_layout = make_struct_layout(
        LAYOUT(Float[7], m_f),
        LAYOUT(mat4, m_matrix)
      );
    };

where the class `standards::uniform_std140` defined the types `Float` and `mat4`
that were used. `MEMBER` is a macro that hides gory details.

The above declaration is actually done with the macro `LAYOUT_DECLARATION`,
so that the above declaration will look something like:

    LAYOUT_DECLARATION(MyFoo, uniform_std140)
    {
      static constexpr auto struct_layout = make_struct_layout(
        LAYOUT(Float[7], m_f),  // Declare an array of 7 floats.
        LAYOUT(mat4, m_matrix)
      );
    }

Note that neither `Float` nor `mat4` are types that contain a float or
a matrix of floats respectively; instead, they are types that encode the
number of rows, columns and underlying scalar type (float) all in a 32bit
bit pattern. The variable `struct_layout` also contains information about
alignment, size, array stride and offset within the struct.

The reason this information is encoded in a type is because it is used
to perform calculation in constexpr functions, and you can't pass constexpr
as arguments to functions. This means that the types need to be specializations
of a template class, and specializations can only be added in the namespace
that defined it. In other words, the "returned" types, encoded as the std::tuple
type of `struct_layout`, cannot be defined in their own namespace; they
have to be specializations by standard themselves.

This is why, for example, the above information for each basic type is encoded
as a specialization of `vulkan::shaderbuilder::BasicTypeLayout`:

    namespace vulkan::shaderbuilder {

    template<glsl::Standard Standard, glsl::ScalarIndex ScalarIndex, int Rows, int Cols, size_t Alignment, size_t Size, size_t ArrayStride>
    struct BasicTypeLayout;

### Shader preprocessing ###

The vulkan engine can generate shader variable declarations and then compile
the resulting shader. Before this preprocessing, the shader code is called
"template code".

Preprocessing only takes place if the template code contains one or more
identifiers with a double colon in it; such identifiers must have the form

    "Class::member"

which are called GLSL ID strings (`glsl_id_full`) and where "Class" is called
the 'prefix'. They must be registered by the application, or an error is thrown
and the application will terminate.

### Registration of shader variables ###

Shader variables are always members of a C++ struct, even if those struct
only have a single member. All members must have types as defined in "math/glsl.h",
previous defined structs, and/or arrays thereof.

For example,

first the used struct needs to be forward declared, followed by its `layout`:

    struct Foo;
    LAYOUT_DECLARATION(Foo, uniform_std140)
    {
      static constexpr auto struct_layout = make_struct_layout(
        LAYOUT(Float[3], var1),
        LAYOUT(vec4, var2),
        LAYOUT(mat3[7], var3)
    };

    struct MyData;
    LAYOUT_DECLARATION(MyData, uniform_std140)
    {
      static constexpr auto struct_layout = make_struct_layout(
        LAYOUT(dmat4x3, var4),
        LAYOUT(Foo[13], var5),
        LAYOUT(vec3, var6)
      );
    }

after which the actual declaration of a struct may follow:

    STRUCT_DECLARATION(Foo)
    {
      MEMBER(0, Float[3], var1);
      MEMBER(1, vec4, var2);
      MEMBER(2, mat3[7], var3);
    };

    STRUCT_DECLARATION(MyData)
    {
      MEMBER(0, dmat4x3, var4);
      MEMBER(1, Foo[13], var5);
      MEMBER(2, vec3, var6);
    };

The above uses `uniform_std140` as example. Other possibilities currently are
`push_constant_std430`, `uniform_scalar`, `push_constant_scalar`,
`per_vertex_data` and `per_instance_data`.

The latter two are intended to be used for structs that contain vertex or instance
attributes (suggested class names `VertexData` and `InstanceData` respectivily).

For example,

    LAYOUT_DECLARATION(VertexData, per_vertex_data)
    {
      etc.

The reason for the `uniform_` and `push_constant_` prefixes are solely to
make it easy to remember that those std's are the default for those.
You can also use `uniform_std430` but then an extension is required and is
currently not supported by the engine.

### Registration of shader input variables ###

Typically shader input variables should be defined in a Window class (derived from
`vulkan::SynchronousWindow`); for example:

    using namespace vulkan::shader_resource;

    Texture m_sample_texture{"m_sample_texture"};
    UniformBuffer<MyData> m_my_data{"m_my_data"};

No object instantiation is required for push constant structs (see MyPushConstant below).

Shader input variables are then registered with the vulkan engine when defining a pipeline
(in the initialize() function of a class derived from vulkan::pipeline::Characteristic)
as follows:

    shader_input_data().add_vertex_input_binding(m_my_per_vertex_object);
    shader_input_data().add_vertex_input_binding(m_my_per_instance_object);
    shader_input_data().add_push_constant<MyPushConstant>();
    shader_input_data().add_texture(window->m_sample_texture);
    shader_input_data().add_uniform_buffer(window->m_my_data [, ...]);

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

The returned `ShaderIndex`s should be passed to `Application::get_shader_info` and
`Pipeline::build_shader` when compiling the shaders as part of a pipeline.

### Compiling the shader code

Compiling the shader code is done in two steps: first the shader template code
needs to be preprocessed, and only then compiled.

Also this happens in the initialize() function of a class derived from
vulkan::pipeline::Characteristic, after the shader input variables are added (see above).

For example,

    shader_input_data().preprocess1(window->application().get_shader_info(m_shader_vert));
    shader_input_data().preprocess1(window->application().get_shader_info(m_shader_frag));

where m_shader_vert and m_shader_frag are the `ShaderIndex`s returned by register_shaders,
see above.

