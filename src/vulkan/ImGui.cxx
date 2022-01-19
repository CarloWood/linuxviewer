#include "sys.h"
#include "ImGui.h"
#include "SynchronousWindow.h"
#include "LogicalDevice.h"
#include "Application.h"
#include "ImageKind.h"
#include "debug.h"
#include <imgui.h>
#include <backends/imgui_impl_vulkan.h>

static void check_version()
{
  // This won't work inside namespace vulkan.
  IMGUI_CHECKVERSION();
}

// Use lower case for the imgui namespace because my class is already called ImGui.
namespace imgui = ImGui;

namespace vulkan {

// Just this compilation unit.
using namespace imgui;

void ImGui::init(task::SynchronousWindow const* owning_window, vk::Extent2D extent)
{
  DoutEntering(dc::vulkan, "ImGui::init(" << owning_window << ")");
  check_version();

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

  auto imgui_pool = owning_window->logical_device().create_descriptor_pool(pool_sizes, 1000
      COMMA_CWDEBUG_ONLY({owning_window, "ImGui::init()::imguiPool"}));

  // 2: Initialize imgui library.

  // This initializes the core structures of imgui.
  CreateContext();

  // Set initial framebuffer size.
  on_window_size_changed(extent);

  // Setting configuration flags.
  ImGuiIO& io = GetIO();
  // For all flags see https://github.com/ocornut/imgui/blob/master/imgui.h#L1530 (enum ImGuiConfigFlags_).
//  io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Master keyboard navigation enable flag. NewFrame() will automatically fill io.NavInputs[] based on io.AddKeyEvent() calls.

  // Set up ImGui style to use.
  StyleColorsDark();
  ImGuiStyle& gui_style = GetStyle();
  gui_style.Colors[ImGuiCol_TitleBg] = ImVec4( 0.16f, 0.29f, 0.48f, 0.9f );
  gui_style.Colors[ImGuiCol_TitleBgActive] = ImVec4( 0.16f, 0.29f, 0.48f, 0.9f );
  gui_style.Colors[ImGuiCol_WindowBg] = ImVec4( 0.06f, 0.07f, 0.08f, 0.8f );
  gui_style.Colors[ImGuiCol_PlotHistogram] = ImVec4( 0.20f, 0.40f, 0.60f, 1.0f );
  gui_style.Colors[ImGuiCol_PlotHistogramHovered] = ImVec4( 0.20f, 0.45f, 0.90f, 1.0f );

  // Short cut.
  LogicalDevice const* logical_device = &owning_window->logical_device();

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
  SamplerKind const imgui_font_sampler_kind(logical_device, {});
  m_font_texture = owning_window->upload_texture(texture_data, width, height, 0, imgui_font_image_view_kind, imgui_font_sampler_kind
      COMMA_CWDEBUG_ONLY({owning_window, "m_imgui.m_font_texture"}));
  io.Fonts->SetTexID(reinterpret_cast<ImTextureID>(static_cast<VkImageView>(*m_font_texture.m_image_view)));
  // Free memory allocated by GetTexDataAsRGBA32.
  io.Fonts->ClearTexData();

  // Short cuts.
  Application const& application = owning_window->application();
  LogicalDevice const& device = *logical_device;
  Queue const& queue = owning_window->presentation_surface().graphics_queue();

#if 0
  // This initializes imgui for Vulkan.
  ImGui_ImplVulkan_InitInfo init_info{
    .Instance = application.vh_instance(),
    .PhysicalDevice = device.vh_physical_device(),
    .Device = device.vh_logical_device({}),
    .QueueFamily = static_cast<uint32_t>(queue.queue_family().get_value()),
    .Queue = static_cast<vk::Queue const&>(queue),
    //.PipelineCache = ...
    .DescriptorPool = *imgui_pool,
    .MinImageCount = 2,
    .ImageCount = 2,
    .MSAASamples = VK_SAMPLE_COUNT_1_BIT
    //.Allocator = ...
    //.CheckVkResultFn = ...
  };

  ImGui_ImplVulkan_Init(&init_info, owning_window->swapchain().vh_render_pass());       // FIXME: use imgui_pass (move that to base class).
#endif
}

void ImGui::on_window_size_changed(vk::Extent2D extent)
{
  ImGuiIO& io = GetIO();
  io.DisplaySize.x = extent.width;
  io.DisplaySize.y = extent.height;
}

void ImGui::start_frame(float delta_s)
{
  ImGuiIO& io = GetIO();
  io.DeltaTime = delta_s;               // Time elapsed since the previous frame (in seconds).
  NewFrame();

  Text("Hello world!");
}

void ImGui::render_frame(CommandBufferWriteAccessType<pool_type>& command_buffer_w)
{
  EndFrame();
  Render();
  ImDrawData* draw_data = GetDrawData();
}

ImGui::~ImGui()
{
  if (imgui::GetCurrentContext())
    imgui::DestroyContext();
}

} // namespace vulkan
