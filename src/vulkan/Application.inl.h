#pragma once

#include "Application.h"
#include "SynchronousWindow.h"

namespace vulkan {

template<ConceptWindowEvents WINDOW_EVENTS, ConceptSynchronousWindow SYNCHRONOUS_WINDOW, typename... SYNCHRONOUS_WINDOW_ARGS>
boost::intrusive_ptr<task::SynchronousWindow const> Application::create_window(
    std::tuple<SYNCHRONOUS_WINDOW_ARGS...>&& window_constructor_args,
    vk::Rect2D geometry, request_cookie_type request_cookie,
    std::u8string&& title, task::LogicalDevice const* logical_device_task,
    task::SynchronousWindow const* parent_window_task)
{
  DoutEntering(dc::vulkan, "vulkan::Application::create_window<" <<
      libcwd::type_info_of<WINDOW_EVENTS>().demangled_name() << ", " <<         // First template parameter (WINDOW_EVENTS).
      libcwd::type_info_of<SYNCHRONOUS_WINDOW>().demangled_name() <<            // Second template parameter (SYNCHRONOUS_WINDOW).
      ((LibcwDoutStream << ... << (std::string(", ") + libcwd::type_info_of<SYNCHRONOUS_WINDOW_ARGS>().demangled_name())), ">(") <<
                                                                                // Tuple template parameters (SYNCHRONOUS_WINDOW_ARGS...)
      window_constructor_args << ", " << geometry << ", " << std::hex << request_cookie << std::dec << ", \"" << title << "\", " << logical_device_task << ", " << parent_window_task << ")");

  // Call Application::initialize(argc, argv) immediately after constructing the Application.
  //
  // For example:
  //
  //   MyApplication application;
  //   application.initialize(argc, argv);      // <-- this is missing if you assert here.
  //   auto root_window1 = application.create_root_window<MyWindowEvents, MyRenderLoop>({1000, 800}, MyLogicalDevice::root_window_request_cookie1);
  //
  ASSERT(m_event_loop);

  boost::intrusive_ptr<task::SynchronousWindow> window_task =
    statefultask::create_from_tuple<SYNCHRONOUS_WINDOW>(
        std::tuple_cat(
          std::move(window_constructor_args),
          std::make_tuple(this COMMA_CWDEBUG_ONLY(true))
        )
    );
  window_task->create_window_events<WINDOW_EVENTS>(geometry.extent);

  // Window initialization.
  if (title.empty())
    title = application_name();
  window_task->set_title(std::move(title));
  window_task->set_offset(geometry.offset);
  window_task->set_request_cookie(request_cookie);
  window_task->set_logical_device_task(logical_device_task);
  // The key passed to set_xcb_connection_broker_and_key MUST be canonicalized!
  m_main_display_broker_key.canonicalize();
  window_task->set_xcb_connection_broker_and_key(m_xcb_connection_broker, &m_main_display_broker_key);
  // Note that in the case of creating a child window we use the logical device of the parent.
  // However, logical_device_task can be null here because this function might be called before
  // the logical device (or parent window) was created. The SynchronousWindow task takes this
  // into account in state SynchronousWindow_create: where m_logical_device_task is null and
  // m_parent_window_task isn't, it registers with m_parent_window_task->m_logical_device_index_available_event
  // to pick up the correct value of m_logical_device_task.
  window_task->set_parent_window_task(parent_window_task);

  // Create window and start rendering loop.
  window_task->run();

  // The window is returned in order to pass it to create_logical_device.
  //
  // The pointer should be passed to create_logical_device almost immediately after
  // returning from this function with a std::move.
  return window_task;
}

// Insert a new VertexAttribute into m_vertex_attributes for each MemberLayout in ShaderVariableLayouts<ENTRY>::layouts.
template<typename ContainingClass, glsl::Standard Standard, glsl::ScalarIndex ScalarIndex, int Rows, int Cols, size_t Alignment, size_t Size, size_t ArrayStride,
  int MemberIndex, size_t MaxAlignment, size_t Offset, utils::TemplateStringLiteral GlslIdStr>
auto Application::register_glsl_id_str(glsl_id_str_to_vertex_attribute_layout_t::wat const& glsl_id_str_to_vertex_attribute_layout_w,
    shaderbuilder::MemberLayout<ContainingClass, shaderbuilder::BasicTypeLayout<Standard, ScalarIndex, Rows, Cols, Alignment, Size, ArrayStride>,
    MemberIndex, MaxAlignment, Offset, GlslIdStr> const& member_layout) /*threadsafe-*/const
{
  std::string_view glsl_id_sv = static_cast<std::string_view>(member_layout.glsl_id_str);
  // These strings are made with std::to_array("stringliteral"), which includes the terminating zero,
  // but the trailing '\0' was already removed by the conversion to string_view.
  ASSERT(!glsl_id_sv.empty() && glsl_id_sv.back() != '\0');
  shaderbuilder::VertexAttributeLayout vertex_attribute_layout{
    .m_base_type = {
      .m_standard = Standard,
      .m_rows = Rows,
      .m_cols = Cols,
      .m_scalar_type = ScalarIndex,
      .m_log2_alignment = utils::log2(Alignment),
      .m_size = Size,
      .m_array_stride = ArrayStride },
    .m_glsl_id_str = glsl_id_sv.data(),
    .m_offset = Offset,
    .m_array_size = 0
  };
  Dout(dc::vulkan, "Registering \"" << glsl_id_sv << "\" with layout " << vertex_attribute_layout);
  auto res = glsl_id_str_to_vertex_attribute_layout_w->emplace(glsl_id_sv, vertex_attribute_layout);
  // The m_glsl_id_str of each ENTRY must be unique. And of course, don't register the same attribute twice.
  ASSERT(res.second);
};

// Register an array; identical to the above except that m_array_size is set to Elements instead of zero.
template<typename ContainingClass, glsl::Standard Standard, glsl::ScalarIndex ScalarIndex, int Rows, int Cols, size_t Alignment, size_t Size, size_t ArrayStride,
  int MemberIndex, size_t MaxAlignment, size_t Offset, utils::TemplateStringLiteral GlslIdStr, size_t Elements>
auto Application::register_glsl_id_str(glsl_id_str_to_vertex_attribute_layout_t::wat const& glsl_id_str_to_vertex_attribute_layout_w,
    shaderbuilder::MemberLayout<ContainingClass, shaderbuilder::ArrayLayout<shaderbuilder::BasicTypeLayout<Standard, ScalarIndex, Rows, Cols, Alignment, Size, ArrayStride>, Elements>,
    MemberIndex, MaxAlignment, Offset, GlslIdStr> const& member_layout) /*threadsafe-*/const
{
  std::string_view glsl_id_sv = static_cast<std::string_view>(member_layout.glsl_id_str);
  // These strings are made with std::to_array("stringliteral"), which includes the terminating zero,
  // but the trailing '\0' was already removed by the conversion to string_view.
  ASSERT(!glsl_id_sv.empty() && glsl_id_sv.back() != '\0');
  shaderbuilder::VertexAttributeLayout vertex_attribute_layout{
    .m_base_type = {
      .m_standard = Standard,
      .m_rows = Rows,
      .m_cols = Cols,
      .m_scalar_type = ScalarIndex,
      .m_log2_alignment = utils::log2(Alignment),
      .m_size = Size,
      .m_array_stride = ArrayStride },
    .m_glsl_id_str = glsl_id_sv.data(),
    .m_offset = Offset,
    .m_array_size = Elements
  };
  Dout(dc::vulkan, "Registering \"" << glsl_id_sv << "\" with layout " << vertex_attribute_layout);
  auto res = glsl_id_str_to_vertex_attribute_layout_w->emplace(glsl_id_sv, vertex_attribute_layout);
  // The m_glsl_id_str of each ENTRY must be unique. And of course, don't register the same attribute twice.
  ASSERT(res.second);
};

template<typename ENTRY>
requires (std::same_as<typename shaderbuilder::ShaderVariableLayouts<ENTRY>::tag_type, glsl::per_vertex_data> ||
          std::same_as<typename shaderbuilder::ShaderVariableLayouts<ENTRY>::tag_type, glsl::per_instance_data>)
void Application::register_vertex_attributes() /*threadsafe-*/const
{
  DoutEntering(dc::vulkan, "Application::register_vertex_attributes<" << libcwd::type_info_of<ENTRY>().demangled_name() << ">");

  using namespace vulkan::shaderbuilder;
#ifdef CWDEBUG
  {
    int rc;
    char const* const demangled_type = abi::__cxa_demangle(typeid(ShaderVariableLayouts<ENTRY>::layouts).name(), 0, 0, &rc);
    Dout(dc::vulkan, "The type of ShaderVariableLayouts<" << libcwd::type_info_of<ENTRY>().demangled_name() << ">::layouts is: " << demangled_type);
  }
#endif

  glsl_id_str_to_vertex_attribute_layout_t::wat glsl_id_str_to_vertex_attribute_layout_w(m_glsl_id_str_to_vertex_attribute_layout);

  // Use the specialization of ShaderVariableLayouts to get the layout of ENTRY
  // in the form of a tuple, of the vertex attributes, `layouts`.
  // Then for each member layout call insert_vertex_attribute.
  [&]<typename... MemberLayout>(std::tuple<MemberLayout...> const& layouts)
  {
    ([&, this]
    {
      {
#if 0 // Print the type MemberLayout.
        int rc;
        char const* const demangled_type = abi::__cxa_demangle(typeid(MemberLayout).name(), 0, 0, &rc);
        Dout(dc::vulkan, "We get here for type " << demangled_type);
#endif
        static constexpr int member_index = MemberLayout::member_index;
        register_glsl_id_str(glsl_id_str_to_vertex_attribute_layout_w, std::get<member_index>(layouts));
      }
    }(), ...);
  }(ShaderVariableLayouts<ENTRY>::layouts);
}

template<typename ENTRY>
requires (std::same_as<typename shaderbuilder::ShaderVariableLayouts<ENTRY>::tag_type, glsl::push_constant_std430>)
void Application::register_push_constant() /*threadsafe-*/const
{
  DoutEntering(dc::vulkan, "Application::register_push_constant<" << libcwd::type_info_of<ENTRY>().demangled_name() << ">");

  using namespace vulkan::shaderbuilder;
#ifdef CWDEBUG
  {
    int rc;
    char const* const demangled_type = abi::__cxa_demangle(typeid(ShaderVariableLayouts<ENTRY>::layouts).name(), 0, 0, &rc);
    Dout(dc::vulkan, "The type of ShaderVariableLayouts<" << libcwd::type_info_of<ENTRY>().demangled_name() << ">::layouts is: " << demangled_type);
  }
#endif
}

} // namespace vulkan
