#include "sys.h"
#include "ImGui.h"
#include "SynchronousWindow.h"
#include "LogicalDevice.h"
#include "Application.h"
#include "ImageKind.h"
#include "InputEvent.h"
#include "debug.h"
#include "pipeline/Pipeline.h"
#include "vk_utils/print_flags.h"
#include <imgui.h>
#include <xkbcommon/xkbcommon-keysyms.h>

#if defined(CWDEBUG) && !defined(DOXYGEN)
NAMESPACE_DEBUG_CHANNELS_START
channel_ct imgui("IMGUI");
NAMESPACE_DEBUG_CHANNELS_END
#endif

static void check_version()
{
  // This won't work inside namespace vulkan.
  IMGUI_CHECKVERSION();
}

// Pointer to current ImGui context.
// Must be updated at the start of each code block that calls imgui functions and that could be a new thread; meaning at the start of the render loop.
thread_local ImGuiContext* lvImGuiTLS;

// Use imgui_ns because the class inside vulkan is already called ImGui.
namespace imgui_ns = ImGui;

namespace vulkan {

// vulkan::Window::convert should map XCB codes to our codes, which in turn must be equal to what imgui uses.
static_assert(
    ModifierMask::Ctrl  == ImGuiKeyModFlags_Ctrl &&
    ModifierMask::Shift == ImGuiKeyModFlags_Shift &&
    ModifierMask::Alt   == ImGuiKeyModFlags_Alt &&
    ModifierMask::Super == ImGuiKeyModFlags_Super, "Modifier mapping needs fixing.");

//inline
LogicalDevice const& ImGui::logical_device() const
{
  return m_owning_window->logical_device();
}

// Just this compilation unit.
using namespace imgui_ns;

void ImGui::create_frame_resources(FrameResourceIndex number_of_frame_resources
    COMMA_CWDEBUG_ONLY(AmbifixOwner const& ambifix))
{
  m_frame_resources_list.resize(number_of_frame_resources.get_value());
  for (FrameResourceIndex i = m_frame_resources_list.ibegin(); i != m_frame_resources_list.iend(); ++i)
  {
#ifdef CWDEBUG
    AmbifixOwner const list_ambifix = ambifix(".m_frame_resources_list[" + to_string(i) + "]");
#endif
  }
}

void ImGui::create_descriptor_set(CWDEBUG_ONLY(AmbifixOwner const& ambifix))
{
  DoutEntering(dc::vulkan, "ImGui::create_descriptor_set()");

  std::vector<vk::DescriptorSetLayoutBinding> layout_bindings = {
    {
      .binding = 0,
      .descriptorType = vk::DescriptorType::eCombinedImageSampler,
      .descriptorCount = 1U,
      .stageFlags = vk::ShaderStageFlagBits::eFragment,
      .pImmutableSamplers = nullptr
    }
  };
  std::vector<vk::DescriptorPoolSize> pool_sizes = {
    {
      .type = vk::DescriptorType::eCombinedImageSampler,
      .descriptorCount = 1
    }
  };
  m_descriptor_set = logical_device().create_descriptor_resources(layout_bindings, pool_sizes
      COMMA_CWDEBUG_ONLY(ambifix(".m_descriptor_set")));
}

void ImGui::create_pipeline_layout(CWDEBUG_ONLY(AmbifixOwner const& ambifix))
{
  DoutEntering(dc::vulkan, "ImGui::create_pipeline_layout()");

  vk::PushConstantRange push_constant_ranges{
    .stageFlags = vk::ShaderStageFlagBits::eVertex,
    .offset = 0,
    .size = 4 * sizeof(float)
  };
  m_pipeline_layout = logical_device().create_pipeline_layout({ *m_descriptor_set.m_layout }, { push_constant_ranges }
      COMMA_CWDEBUG_ONLY(ambifix(".m_pipeline_layout")));
}

static constexpr std::string_view imgui_vert_glsl = R"glsl(
layout(push_constant) uniform uPushConstant { vec2 uScale; vec2 uTranslate; } pc;
out gl_PerVertex { vec4 gl_Position; };
layout(location = 0) out struct { vec4 Color; vec2 UV; } Out;
void main()
{
  Out.Color = ImDrawVert::col;
  Out.UV = ImDrawVert::uv;
  gl_Position = vec4(ImDrawVert::pos * pc.uScale + pc.uTranslate, 0, 1);
}
)glsl";

static constexpr std::string_view imgui_frag_glsl = R"glsl(
#version 450 core
layout(location = 0) out vec4 fColor;
layout(set=0, binding=0) uniform sampler2D sTexture;
layout(location = 0) in struct { vec4 Color; vec2 UV; } In;
void main()
{
  fColor = In.Color * texture(sTexture, In.UV.st);
}
)glsl";

void ImGui::create_graphics_pipeline(vk::SampleCountFlagBits MSAASamples COMMA_CWDEBUG_ONLY(AmbifixOwner const& ambifix))
{
  DoutEntering(dc::vulkan, "ImGui::create_graphics_pipeline(" << MSAASamples << ")");

  // The pipeline needs to know who owns it.
  pipeline::Pipeline pipeline;

  // Define the pipeline.
  pipeline.add_vertex_input_binding(m_ui);

  {
    using namespace vulkan::shaderbuilder;

    ShaderInfo shader_vert(vk::ShaderStageFlagBits::eVertex, "imgui.vert.glsl");
    ShaderInfo shader_frag(vk::ShaderStageFlagBits::eFragment, "imgui.frag.glsl");

    shader_vert.load(imgui_vert_glsl);
    shader_frag.load(imgui_frag_glsl);

    ShaderCompiler compiler;

    pipeline.build_shader(m_owning_window, shader_vert, compiler
        COMMA_CWDEBUG_ONLY({ m_owning_window, "ImGui::create_graphics_pipeline()::pipeline" }));
    pipeline.build_shader(m_owning_window, shader_frag, compiler
        COMMA_CWDEBUG_ONLY({ m_owning_window, "ImGui::create_graphics_pipeline()::pipeline" }));
  }

  auto vertex_binding_descriptions = pipeline.vertex_binding_descriptions();
  auto vertex_attribute_descriptions = pipeline.vertex_attribute_descriptions();

  vk::PipelineVertexInputStateCreateInfo vertex_input_state_create_info{
    .flags = {},
    .vertexBindingDescriptionCount = static_cast<uint32_t>(vertex_binding_descriptions.size()),
    .pVertexBindingDescriptions = vertex_binding_descriptions.data(),
    .vertexAttributeDescriptionCount = static_cast<uint32_t>(vertex_attribute_descriptions.size()),
    .pVertexAttributeDescriptions = vertex_attribute_descriptions.data()
  };

  vk::PipelineInputAssemblyStateCreateInfo input_assembly_state_create_info{
    .flags = {},
    .topology = vk::PrimitiveTopology::eTriangleList
  };

  vk::Extent2D swapchain_extent = m_owning_window->swapchain().extent();

  vk::Viewport viewport{
    .x = 0.0f,
    .y = 0.0f,
    .width = (float)swapchain_extent.width,
    .height = (float)swapchain_extent.height,
    .minDepth = 0.0f,
    .maxDepth = 1.0f
  };

  vk::Rect2D scissor{
    .offset = {0, 0},
    .extent = swapchain_extent
  };

  vk::PipelineViewportStateCreateInfo viewport_state_create_info{
    .viewportCount = 1,
    .pViewports = &viewport,
    .scissorCount = 1,
    .pScissors = &scissor
  };

  vk::PipelineRasterizationStateCreateInfo rasterization_state_create_info{
    .depthClampEnable = VK_FALSE,
    .rasterizerDiscardEnable = VK_FALSE,
    .polygonMode = vk::PolygonMode::eFill,
    .cullMode = vk::CullModeFlagBits::eNone,
    .frontFace = vk::FrontFace::eCounterClockwise,
    .depthBiasEnable = VK_FALSE,
    .depthBiasConstantFactor = 0.0f,
    .depthBiasClamp = 0.0f,
    .depthBiasSlopeFactor = 0.0f,
    .lineWidth = 1.0f
  };

  vk::PipelineMultisampleStateCreateInfo multisample_state_create_info{
    .rasterizationSamples = MSAASamples,
    .sampleShadingEnable = VK_FALSE,
    .minSampleShading = 0.0f,
    .pSampleMask = nullptr,
    .alphaToCoverageEnable = VK_FALSE,
    .alphaToOneEnable = VK_FALSE
  };

  vk::PipelineDepthStencilStateCreateInfo depth_stencil_state_create_info{};

  vk::PipelineColorBlendAttachmentState color_blend_attachment_state{
    .blendEnable = VK_TRUE,
    .srcColorBlendFactor = vk::BlendFactor::eSrcAlpha,
    .dstColorBlendFactor = vk::BlendFactor::eOneMinusSrcAlpha,
    .colorBlendOp = vk::BlendOp::eAdd,
    .srcAlphaBlendFactor = vk::BlendFactor::eOne,
    .dstAlphaBlendFactor = vk::BlendFactor::eOneMinusSrcAlpha,
    .alphaBlendOp = vk::BlendOp::eAdd,
    .colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA
  };

  vk::PipelineColorBlendStateCreateInfo color_blend_state_create_info{
    .attachmentCount = 1,
    .pAttachments = &color_blend_attachment_state,
  };

  std::vector<vk::DynamicState> dynamic_states = {
    vk::DynamicState::eViewport,
    vk::DynamicState::eScissor
  };

  vk::PipelineDynamicStateCreateInfo dynamic_state_create_info{
    .dynamicStateCount = static_cast<uint32_t>(dynamic_states.size()),
    .pDynamicStates = dynamic_states.data()
  };

  auto const& shader_stage_create_infos = pipeline.shader_stage_create_infos();

  vk::GraphicsPipelineCreateInfo pipeline_create_info{
    .stageCount = static_cast<uint32_t>(shader_stage_create_infos.size()),
    .pStages = shader_stage_create_infos.data(),
    .pVertexInputState = &vertex_input_state_create_info,
    .pInputAssemblyState = &input_assembly_state_create_info,
    .pViewportState = &viewport_state_create_info,
    .pRasterizationState = &rasterization_state_create_info,
    .pMultisampleState = &multisample_state_create_info,
    .pDepthStencilState = &depth_stencil_state_create_info,
    .pColorBlendState = &color_blend_state_create_info,
    .pDynamicState = &dynamic_state_create_info,
    .layout = *m_pipeline_layout,
    .renderPass = m_owning_window->vh_imgui_render_pass(),
    .basePipelineHandle = vk::Pipeline{},
    .basePipelineIndex = -1
  };

  m_graphics_pipeline = logical_device().create_graphics_pipeline(vk::PipelineCache{}, pipeline_create_info
      COMMA_CWDEBUG_ONLY(ambifix(".m_graphics_pipeline")));
}

void ImGui::init(task::SynchronousWindow const* owning_window, vk::SampleCountFlagBits MSAASamples
    COMMA_CWDEBUG_ONLY(AmbifixOwner const& ambifix))
{
  DoutEntering(dc::vulkan, "ImGui::init(" << owning_window << ")");
  check_version();

  // Remember which window is owning us.
  m_owning_window = owning_window;

  using dt = vk::DescriptorType;

  // 1: Create descriptor pool for imgui.
  // The size of the pool is very oversize, but this is what imgui demo uses.
  std::vector<vk::DescriptorPoolSize> const pool_sizes = {
    { dt::eSampler, 1000 },
    { dt::eCombinedImageSampler, 1000 },
    { dt::eSampledImage, 1000 },
    { dt::eStorageImage, 1000 },
    { dt::eUniformTexelBuffer, 1000 },
    { dt::eStorageTexelBuffer, 1000 },
    { dt::eUniformBuffer, 1000 },
    { dt::eStorageBuffer, 1000 },
    { dt::eUniformBufferDynamic, 1000 },
    { dt::eStorageBufferDynamic, 1000 },
    { dt::eInputAttachment, 1000 }
  };

  auto imgui_pool = logical_device().create_descriptor_pool(pool_sizes, 1000
      COMMA_CWDEBUG_ONLY({owning_window, "ImGui::init()::imguiPool"}));

  // 2: Initialize imgui library.

  // This initializes the core structures of imgui.
  SetCurrentContext(nullptr);           // Otherwise CreateContext() will not replace it.
  m_context = CreateContext();

  // Set initial framebuffer size.
  on_window_size_changed(owning_window->swapchain().extent());

  // Setting configuration flags.
  ImGuiIO& io = GetIO();
  // For all flags see https://github.com/ocornut/imgui/blob/master/imgui.h#L1530 (enum ImGuiConfigFlags_).
//  io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Master keyboard navigation enable flag. NewFrame() will automatically fill io.NavInputs[] based on io.AddKeyEvent() calls.

  m_ini_filename = owning_window->application().path_of(Directory::state) / "imgui.ini";
  io.IniFilename = m_ini_filename.c_str();
  Dout(dc::notice, "io.IniFilename = \"" << io.IniFilename << "\".");

  // Set up ImGui style to use.
  StyleColorsDark();
  ImGuiStyle& gui_style = GetStyle();
  gui_style.Colors[ImGuiCol_TitleBg] = ImVec4( 0.16f, 0.29f, 0.48f, 0.9f );
  gui_style.Colors[ImGuiCol_TitleBgActive] = ImVec4( 0.16f, 0.29f, 0.48f, 0.9f );
  gui_style.Colors[ImGuiCol_WindowBg] = ImVec4( 0.06f, 0.07f, 0.08f, 0.8f );
  gui_style.Colors[ImGuiCol_PlotHistogram] = ImVec4( 0.20f, 0.40f, 0.60f, 1.0f );
  gui_style.Colors[ImGuiCol_PlotHistogramHovered] = ImVec4( 0.20f, 0.45f, 0.90f, 1.0f );

  // Create imgui descriptor set and layout. This must be done before calling upload_texture below.
  create_descriptor_set(CWDEBUG_ONLY(ambifix));

  // Build and load the texture atlas into a texture.
  uint32_t width;
  uint32_t height;
  std::byte const* texture_data;
  {
    int w, h;
    unsigned char* d = nullptr;
    io.Fonts->GetTexDataAsRGBA32(&d, &w, &h);
    width = w;
    height = h;
    texture_data = reinterpret_cast<std::byte*>(d);
  }
  ImageKind const imgui_font_image_kind({
    .format = vk::Format::eR8G8B8A8Unorm,       // This must be obviously a 32bit format, since we called GetTexDataAsRGBA32.
    .usage = vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled
  });
  ImageViewKind const imgui_font_image_view_kind(imgui_font_image_kind, {});
  SamplerKind const imgui_font_sampler_kind(&logical_device(), {});
  m_font_texture = owning_window->upload_texture(texture_data, width, height, 0, imgui_font_image_view_kind, imgui_font_sampler_kind, *m_descriptor_set.m_handle
      COMMA_CWDEBUG_ONLY(ambifix(".m_font_texture")));
  // Store a VkDescriptorSet (which is a pointer to an opague struct) as "texture ID".
  ASSERT(sizeof(ImTextureID) == sizeof(void*));
  io.Fonts->SetTexID(reinterpret_cast<ImTextureID>(static_cast<VkDescriptorSet>(*m_descriptor_set.m_handle)));
  // Free memory allocated by GetTexDataAsRGBA32.
  io.Fonts->ClearTexData();

  // Short cuts.
  Application const& application = owning_window->application();
  Queue const& queue = owning_window->presentation_surface().graphics_queue();

  // Create imgui pipeline layout.
  create_pipeline_layout(CWDEBUG_ONLY(ambifix));
  // Create imgui pipeline.
  create_graphics_pipeline(MSAASamples COMMA_CWDEBUG_ONLY(ambifix));
}

// Called from SynchronousWindow::handle_window_size_changed, so no need for locking.
void ImGui::on_window_size_changed(vk::Extent2D extent)
{
  DoutEntering(dc::imgui, "ImGui::on_window_size_changed(" << extent << ")");
  ImGuiIO& io = GetIO();
  io.DisplaySize.x = extent.width;
  io.DisplaySize.y = extent.height;
  Dout(dc::imgui, "io.DisplaySize set to (" << io.DisplaySize.x << ", " << io.DisplaySize.y << ")");
}

void ImGui::on_focus_changed(bool in_focus) const
{
  DoutEntering(dc::imgui, "ImGui::on_focus_changed(" << in_focus << ")");
  ImGuiIO& io = GetIO();
  io.AddFocusEvent(in_focus);
}

void ImGui::on_mouse_move(int x, int y)
{
  DoutEntering(dc::imgui(dc::vkframe.is_on() && (x != m_last_x || y != m_last_y)), "ImGui::on_mouse_move(" << x << ", " << y << ")");
#ifdef CWDEBUG
  m_last_x = x;
  m_last_y = y;
#endif
  ImGuiIO& io = GetIO();
  io.AddMousePosEvent(x, y);
}

void ImGui::on_mouse_wheel_event(float delta_x, float delta_y) const
{
  DoutEntering(dc::imgui, "ImGui::on_mouse_wheel_event(" << delta_x << ", " << delta_y << ")");
  ImGuiIO& io = GetIO();
  io.AddMouseWheelEvent(-delta_x, -delta_y);
}

void ImGui::on_mouse_click(uint8_t button, bool pressed) const
{
  DoutEntering(dc::imgui, "ImGui::on_mouse_click(" << (int)button << ", " << pressed << ")");
  ImGuiIO& io = GetIO();
  // Only call for the first three buttons.
  ASSERT(button <= 2);
  io.AddMouseButtonEvent((3 - button) % 3, pressed);  // Swap button 1 and 2.
}

void ImGui::on_mouse_enter(int x, int y, bool entered) const
{
  DoutEntering(dc::imgui, "ImGui::on_mouse_enter(" << x << ", " << y << ", " << entered << ")");
  ImGuiIO& io = GetIO();
  if (entered)
    io.AddMousePosEvent(x, y);
  else
    io.AddMousePosEvent(-FLT_MAX, -FLT_MAX);
}

void ImGui::on_key_event(uint32_t keysym, bool pressed) const
{
  DoutEntering(dc::imgui, "ImGui::on_key_event(" << keysym << ", " << pressed << ")");
  ImGuiIO& io = GetIO();
  if (8 < keysym && keysym <= 127)
  {
    if (pressed)
      io.AddInputCharacter(keysym);
  }
  else
  {
    ImGuiKey key;
    switch (keysym)
    {
      case XKB_KEY_BackSpace:
        key = ImGuiKey_Backspace;
        break;
      case XKB_KEY_Tab:
        key = ImGuiKey_Tab;
        break;
#if 0
      case XKB_KEY_Linefeed:
        key = ;
        break;
      case XKB_KEY_Clear:
        key = ;
        break;
#endif
      case XKB_KEY_Return:
        key = ImGuiKey_Enter;
        break;
      case XKB_KEY_Pause:
        key = ImGuiKey_Pause;
        break;
      case XKB_KEY_Scroll_Lock:
        key = ImGuiKey_ScrollLock;
        break;
#if 0
      case XKB_KEY_Sys_Req:
        key = ;
        break;
#endif
      case XKB_KEY_Escape:
        key = ImGuiKey_Escape;
        break;
      case XKB_KEY_Delete:
        key = ImGuiKey_Delete;
        break;
      case XKB_KEY_Home:
        key = ImGuiKey_Home;
        break;
      case XKB_KEY_Left:
        key = ImGuiKey_LeftArrow;
        break;
      case XKB_KEY_Up:
        key = ImGuiKey_UpArrow;
        break;
      case XKB_KEY_Right:
        key = ImGuiKey_RightArrow;
        break;
      case XKB_KEY_Down:
        key = ImGuiKey_DownArrow;
        break;
#if 0
      case XKB_KEY_Prior:
        key = ;
        break;
#endif
      case XKB_KEY_Page_Up:
        key = ImGuiKey_PageUp;
        break;
#if 0
      case XKB_KEY_Next:
        key = ;
        break;
#endif
      case XKB_KEY_Page_Down:
        key = ImGuiKey_PageDown;
        break;
      case XKB_KEY_End:
        key = ImGuiKey_End;
        break;
#if 0
      case XKB_KEY_Begin:
        key =;
        break;
      case XKB_KEY_Select:
        key = ;
        break;
#endif
      case XKB_KEY_Print:
        key = ImGuiKey_PrintScreen;
        break;
#if 0
      case XKB_KEY_Execute:
        key = ;
        break;
#endif
      case XKB_KEY_Insert:
        key = ImGuiKey_Insert;
        break;
#if 0
      case XKB_KEY_Undo:
        key = ;
        break;
      case XKB_KEY_Redo:
        key = ;
        break;
#endif
      case XKB_KEY_Menu:
        key = ImGuiKey_Menu;
        break;
#if 0
      case XKB_KEY_Find:
        key = ;
        break;
      case XKB_KEY_Cancel:
        key = ;
        break;
      case XKB_KEY_Help:
        key = ;
        break;
      case XKB_KEY_Break:
        key = ;
        break;
      case XKB_KEY_Mode_switch:
        key = ;
        break;
      case XKB_KEY_script_switch:
        key = ;
        break;
#endif
      case XKB_KEY_Num_Lock:
        key = ImGuiKey_NumLock;
        break;
#if 0
      case XKB_KEY_KP_Space:
        key = ;
        break;
      case XKB_KEY_KP_Tab:
        key = ;
        break;
#endif
      case XKB_KEY_KP_Enter:
        key = ImGuiKey_KeypadEnter;
        break;
#if 0
      case XKB_KEY_KP_F1:
        key = ;
        break;
      case XKB_KEY_KP_F2:
        key = ;
        break;
      case XKB_KEY_KP_F3:
        key = ;
        break;
      case XKB_KEY_KP_F4:
        key = ;
        break;
      case XKB_KEY_KP_Home:
        key = ;
        break;
      case XKB_KEY_KP_Left:
        key = ;
        break;
      case XKB_KEY_KP_Up:
        key = ;
        break;
      case XKB_KEY_KP_Right:
        key = ;
        break;
      case XKB_KEY_KP_Down:
        key = ;
        break;
      case XKB_KEY_KP_Prior:
        key = ;
        break;
      case XKB_KEY_KP_Page_Up:
        key = ;
        break;
      case XKB_KEY_KP_Next:
        key = ;
        break;
      case XKB_KEY_KP_Page_Down:
        key = ;
        break;
      case XKB_KEY_KP_End:
        key = ;
        break;
      case XKB_KEY_KP_Begin:
        key = ;
        break;
      case XKB_KEY_KP_Insert:
        key = ;
        break;
      case XKB_KEY_KP_Delete:
        key = ;
        break;
#endif
      case XKB_KEY_KP_Equal:
        key = ImGuiKey_KeypadEqual;
        break;
      case XKB_KEY_KP_Multiply:
        key = ImGuiKey_KeypadMultiply;
        break;
      case XKB_KEY_KP_Add:
        key = ImGuiKey_KeypadAdd;
        break;
#if 0
      case XKB_KEY_KP_Separator:
        key = ;
        break;
#endif
      case XKB_KEY_KP_Subtract:
        key = ImGuiKey_KeypadSubtract;
        break;
      case XKB_KEY_KP_Decimal:
        key = ImGuiKey_KeypadDecimal;
        break;
      case XKB_KEY_KP_Divide:
        key = ImGuiKey_KeypadDivide;
        break;
      case XKB_KEY_KP_0:
        key = ImGuiKey_Keypad0;
        break;
      case XKB_KEY_KP_1:
        key = ImGuiKey_Keypad1;
        break;
      case XKB_KEY_KP_2:
        key = ImGuiKey_Keypad2;
        break;
      case XKB_KEY_KP_3:
        key = ImGuiKey_Keypad3;
        break;
      case XKB_KEY_KP_4:
        key = ImGuiKey_Keypad4;
        break;
      case XKB_KEY_KP_5:
        key = ImGuiKey_Keypad5;
        break;
      case XKB_KEY_KP_6:
        key = ImGuiKey_Keypad6;
        break;
      case XKB_KEY_KP_7:
        key = ImGuiKey_Keypad7;
        break;
      case XKB_KEY_KP_8:
        key = ImGuiKey_Keypad8;
        break;
      case XKB_KEY_KP_9:
        key = ImGuiKey_Keypad9;
        break;
      case XKB_KEY_F1:
        key = ImGuiKey_F1;
        break;
      case XKB_KEY_F2:
        key = ImGuiKey_F2;
        break;
      case XKB_KEY_F3:
        key = ImGuiKey_F3;
        break;
      case XKB_KEY_F4:
        key = ImGuiKey_F4;
        break;
      case XKB_KEY_F5:
        key = ImGuiKey_F5;
        break;
      case XKB_KEY_F6:
        key = ImGuiKey_F6;
        break;
      case XKB_KEY_F7:
        key = ImGuiKey_F7;
        break;
      case XKB_KEY_F8:
        key = ImGuiKey_F8;
        break;
      case XKB_KEY_F9:
        key = ImGuiKey_F9;
        break;
      case XKB_KEY_F10:
        key = ImGuiKey_F10;
        break;
      case XKB_KEY_F11:
        key = ImGuiKey_F11;
        break;
      case XKB_KEY_F12:
        key = ImGuiKey_F12;
        break;
      case XKB_KEY_Shift_L:
        key = ImGuiKey_LeftShift;
        break;
      case XKB_KEY_Shift_R:
        key = ImGuiKey_RightShift;
        break;
      case XKB_KEY_Control_L:
        key = ImGuiKey_LeftCtrl;
        break;
      case XKB_KEY_Control_R:
        key = ImGuiKey_RightCtrl;
        break;
      case XKB_KEY_Caps_Lock:
        key = ImGuiKey_CapsLock;
        break;
#if 0
      case XKB_KEY_Shift_Lock:
        key = ;
        break;
      case XKB_KEY_Meta_L:
        key = ;
        break;
      case XKB_KEY_Meta_R:
        key = ;
        break;
#endif
      case XKB_KEY_Alt_L:
        key = ImGuiKey_LeftAlt;
        break;
      case XKB_KEY_Alt_R:
        key = ImGuiKey_RightAlt;
        break;
      case XKB_KEY_Super_L:
        key = ImGuiKey_LeftSuper;
        break;
      case XKB_KEY_Super_R:
        key = ImGuiKey_RightSuper;
        break;
#if 0
      case XKB_KEY_Hyper_L:
        key = ;
        break;
      case XKB_KEY_Hyper_R:
        key = ;
        break;
#endif
      default:
        Dout(dc::warning, "Ignoring unhandled key input with value 0x" << std::hex << keysym);
        return;
    }
    io.AddKeyEvent(key, pressed);
  }
}

void ImGui::update_modifiers(int modifiers) const
{
  DoutEntering(dc::imgui, "ImGui::update_modifiers(" << modifiers << ")");
  ImGuiIO& io = GetIO();
  //FIXME: is this call necessary?
//  io.AddKeyEvent(modifiers);
}

bool ImGui::want_capture_keyboard() const
{
  ImGuiIO& io = GetIO();
  bool res = io.WantCaptureKeyboard;
  Dout(dc::imgui, "ImGui::want_capture_keyboard() = " << res);
  return res;
}

bool ImGui::want_capture_mouse() const
{
  ImGuiIO& io = GetIO();
  bool res = io.WantCaptureMouse;
  Dout(dc::imgui(dc::vkframe.is_on()), "ImGui::want_capture_mouse() = " << res);
  return res;
}

void ImGui::start_frame(float delta_s)
{
  ImGuiIO& io = GetIO();
  io.DeltaTime = delta_s;               // Time elapsed since the previous frame (in seconds).
  NewFrame();
}

void ImGui::setup_render_state(CommandBufferWriteAccessType<pool_type>& command_buffer_w, void* draw_data_void_ptr, ImGui_FrameResourcesData& frame_resources, vk::Viewport const& viewport)
{
  // I did not want to forward declare ImDrawData in a header in global namespace.
  ImDrawData* draw_data = reinterpret_cast<ImDrawData*>(draw_data_void_ptr);

  // Bind the pipeline.
  command_buffer_w->bindPipeline(vk::PipelineBindPoint::eGraphics, *m_graphics_pipeline);

  // Bind vertex and index buffer.
  command_buffer_w->bindVertexBuffers(0, { *frame_resources.m_vertex_buffer.m_buffer }, { 0 });
  command_buffer_w->bindIndexBuffer(*frame_resources.m_index_buffer.m_buffer, 0, sizeof(ImDrawIdx) == 2 ? vk::IndexType::eUint16 : vk::IndexType::eUint32);

  // Set viewport again (is this really needed?).
  command_buffer_w->setViewport(0, { viewport });

  // Setup scale and translation.
  std::array<float, 2> scale = { 2.0f / draw_data->DisplaySize.x, 2.0f / draw_data->DisplaySize.y };
  std::array<float, 2> translate = { -1.0f - draw_data->DisplayPos.x * scale[0], -1.0f - draw_data->DisplayPos.y * scale[1] };
  command_buffer_w->pushConstants(*m_pipeline_layout, vk::ShaderStageFlagBits::eVertex, sizeof(float) * 0, sizeof(float) * scale.size(), scale.data());
  command_buffer_w->pushConstants(*m_pipeline_layout, vk::ShaderStageFlagBits::eVertex, sizeof(float) * 2, sizeof(float) * translate.size(), translate.data());
}

void ImGui::render_frame(CommandBufferWriteAccessType<pool_type>& command_buffer_w, FrameResourceIndex index
    COMMA_CWDEBUG_ONLY(AmbifixOwner const& ambifix))
{
  EndFrame();
  Render();
  ImDrawData* draw_data = GetDrawData();
  ImGui_FrameResourcesData& frame_resources = m_frame_resources_list[index];
  LogicalDevice const& device = logical_device();

  size_t vertex_size = draw_data->TotalVtxCount * sizeof(ImDrawVert);
  size_t index_size = draw_data->TotalIdxCount * sizeof(ImDrawIdx);

  bool initial_buffer_creation = !frame_resources.m_vertex_buffer.m_buffer;

  if (AI_UNLIKELY(initial_buffer_creation) && draw_data->TotalVtxCount == 0)
    vertex_size = index_size = 1;       // We're not allowed to create buffers with zero size.

  if (AI_LIKELY(draw_data->TotalVtxCount > 0) || initial_buffer_creation)
  {
    // Create or resize the vertex buffer.
    if (vertex_size > frame_resources.m_vertex_buffer.m_size)
      frame_resources.m_vertex_buffer = device.create_buffer(
          vertex_size,
          vk::BufferUsageFlagBits::eVertexBuffer,
          vk::MemoryPropertyFlagBits::eHostVisible
          COMMA_CWDEBUG_ONLY(ambifix(".m_frame_resources_list[" + std::to_string(index.get_value()) + "].m_vertex_buffer")));

    // Create or resize the index buffer.
    if (index_size > frame_resources.m_index_buffer.m_size)
      frame_resources.m_index_buffer = device.create_buffer(
          index_size,
          vk::BufferUsageFlagBits::eIndexBuffer,
          vk::MemoryPropertyFlagBits::eHostVisible
          COMMA_CWDEBUG_ONLY(ambifix(".m_frame_resources_list[" + std::to_string(index.get_value()) + "].m_index_buffer")));
  }

  if (draw_data->TotalVtxCount > 0)
  {
    // Do not write the debug output to dc::vulkan (it is still written to dc::vkframe).
    Debug(dc::vulkan.off());

    // Upload vertex and index data each into a single contiguous GPU buffer.
    ImDrawVert* vtx_dst = static_cast<ImDrawVert*>(device.map_memory(*frame_resources.m_vertex_buffer.m_memory, 0, frame_resources.m_vertex_buffer.m_size));
    ImDrawIdx* idx_dst = static_cast<ImDrawIdx*>(device.map_memory(*frame_resources.m_index_buffer.m_memory, 0, frame_resources.m_index_buffer.m_size));
    for (int n = 0; n < draw_data->CmdListsCount; ++n)
    {
      ImDrawList const* cmd_list = draw_data->CmdLists[n];
      memcpy(vtx_dst, cmd_list->VtxBuffer.Data, cmd_list->VtxBuffer.Size * sizeof(ImDrawVert));
      memcpy(idx_dst, cmd_list->IdxBuffer.Data, cmd_list->IdxBuffer.Size * sizeof(ImDrawIdx));
      vtx_dst += cmd_list->VtxBuffer.Size;
      idx_dst += cmd_list->IdxBuffer.Size;
    }

    std::array<vk::MappedMemoryRange, 2> mapped_memory_ranges = {
      vk::MappedMemoryRange{ .memory = *frame_resources.m_vertex_buffer.m_memory, .size = VK_WHOLE_SIZE },
      vk::MappedMemoryRange{ .memory = *frame_resources.m_index_buffer.m_memory, .size = VK_WHOLE_SIZE }
    };
    device.flush_mapped_memory_ranges(mapped_memory_ranges);

    device.unmap_memory(*frame_resources.m_vertex_buffer.m_memory);
    device.unmap_memory(*frame_resources.m_index_buffer.m_memory);
    Debug(dc::vulkan.on());
  }

  auto swapchain_extent = m_owning_window->swapchain().extent();
  vk::Viewport viewport{
    .x = 0,
    .y = 0,
    .width = static_cast<float>(swapchain_extent.width),
    .height = static_cast<float>(swapchain_extent.height),
    .minDepth = 0.0f,
    .maxDepth = 1.0f
  };
  setup_render_state(command_buffer_w, draw_data, frame_resources, viewport);

  // Will project scissor/clipping rectangles into framebuffer space
  ImVec2 clip_off = draw_data->DisplayPos;         // (0,0) unless using multi-viewports
  ImVec2 clip_scale = draw_data->FramebufferScale; // (1,1) unless using retina display which are often (2,2); Note: A clip_scale other than (1,1) is currently NOT supported (elsewhere).

  // Render command lists

  // Because we merged all buffers into a single one, we maintain our own offset into them.
  int global_vtx_offset = 0;
  int global_idx_offset = 0;

  for (int n = 0; n < draw_data->CmdListsCount; ++n)
  {
    ImDrawList const* cmd_list = draw_data->CmdLists[n];
    for (int cmd_i = 0; cmd_i < cmd_list->CmdBuffer.Size; ++cmd_i)
    {
      ImDrawCmd const* pcmd = &cmd_list->CmdBuffer[cmd_i];
      if (pcmd->UserCallback != nullptr)
      {
        // User callback, registered via ImDrawList::AddCallback().

        // ImDrawCallback_ResetRenderState is a special callback value used by the user to request the renderer to reset render state.
        if (pcmd->UserCallback == ImDrawCallback_ResetRenderState)
          setup_render_state(command_buffer_w, draw_data, frame_resources, viewport);
        else
          pcmd->UserCallback(cmd_list, pcmd);
      }
      else
      {
        // Project scissor/clipping rectangles into framebuffer space.
        ImVec2 clip_min((pcmd->ClipRect.x - clip_off.x) * clip_scale.x, (pcmd->ClipRect.y - clip_off.y) * clip_scale.y);
        ImVec2 clip_max((pcmd->ClipRect.z - clip_off.x) * clip_scale.x, (pcmd->ClipRect.w - clip_off.y) * clip_scale.y);

        // Clamp to viewport as vkCmdSetScissor() won't accept values that are off bounds.
        if (clip_min.x < 0.0f) { clip_min.x = 0.0f; }
        if (clip_min.y < 0.0f) { clip_min.y = 0.0f; }
        if (clip_max.x > viewport.width) { clip_max.x = viewport.width; }
        if (clip_max.y > viewport.height) { clip_max.y = viewport.height; }
        if (clip_max.x <= clip_min.x || clip_max.y <= clip_min.y)
          continue;

        // Apply scissor/clipping rectangle.
        vk::Rect2D scissor = {
          .offset = {
            .x = static_cast<int32_t>(clip_min.x),
            .y = static_cast<int32_t>(clip_min.y) },
          .extent = {
            .width = static_cast<uint32_t>(clip_max.x - clip_min.x),
            .height = static_cast<uint32_t>(clip_max.y - clip_min.y)
          }
        };
        command_buffer_w->setScissor(0, 1, &scissor);

        // Bind DescriptorSet with font or user texture.
        std::array<vk::DescriptorSet, 1> desc_set = { static_cast<VkDescriptorSet>(pcmd->TextureId) };
        command_buffer_w->bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *m_pipeline_layout, 0, desc_set, {});

        // Draw
        command_buffer_w->drawIndexed(pcmd->ElemCount, 1, pcmd->IdxOffset + global_idx_offset, pcmd->VtxOffset + global_vtx_offset, 0);
      }
    }
    global_idx_offset += cmd_list->IdxBuffer.Size;
    global_vtx_offset += cmd_list->VtxBuffer.Size;
  }
}

ImGui::~ImGui()
{
  if (GetCurrentContext())
    DestroyContext();
}

} // namespace vulkan

namespace imgui {

void StatsWindow::draw(ImGuiIO& io, vk_utils::TimerData const& timer)
{
  ImGui::SetNextWindowSize(ImVec2(100.0f, 100.0));
  ImGui::Begin("Stats", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar);

  if (ImGui::RadioButton("FPS", m_show_fps))
  {
    m_show_fps = true;
  }
  ImGui::SameLine();
  if (ImGui::RadioButton("ms", !m_show_fps))
  {
    m_show_fps = false;
  }

  if (m_show_fps)
  {
    ImGui::SetCursorPosX(20.0f);
    ImGui::Text("%7.1f", timer.get_moving_average_FPS());

    auto histogram = timer.get_FPS_histogram();
    ImGui::PlotHistogram("", histogram.data(), static_cast<int>(histogram.size()), 0, nullptr, 0.0f, FLT_MAX, ImVec2(85.0f, 30.0f));
  }
  else
  {
    ImGui::SetCursorPosX(20.0f);
    ImGui::Text("%9.3f", timer.get_moving_average_ms());

    auto histogram = timer.get_delta_ms_histogram();
    ImGui::PlotHistogram("", histogram.data(), static_cast<int>(histogram.size()), 0, nullptr, 0.0f, FLT_MAX, ImVec2(85.0f, 30.0f));
  }

  ImGui::End();
}

} // namespace imgui
