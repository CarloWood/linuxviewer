#include "sys.h"
#include "ImGui.h"
#include "SynchronousWindow.h"
#include "LogicalDevice.h"
#include "Application.h"
#include "debug.h"
#include <imgui.h>
#include <backends/imgui_impl_vulkan.h>

static void check_version()
{
  // This won't work inside namespace vulkan.
  IMGUI_CHECKVERSION();
}

namespace vulkan {

void ImGui::init(task::SynchronousWindow const* owning_window)
{
  DoutEntering(dc::vulkan, "ImGui::init(" << owning_window << ")");
  check_version();

  using dt = vk::DescriptorType;

  // 1: Create descriptor pool for ImGui.
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
  using namespace ImGui;

  // This initializes the core structures of imgui.
  CreateContext();

  // Set up ImGui style to use.
  StyleColorsDark();
  ImGuiStyle& gui_style = GetStyle();
  gui_style.Colors[ImGuiCol_TitleBg] = ImVec4( 0.16f, 0.29f, 0.48f, 0.9f );
  gui_style.Colors[ImGuiCol_TitleBgActive] = ImVec4( 0.16f, 0.29f, 0.48f, 0.9f );
  gui_style.Colors[ImGuiCol_WindowBg] = ImVec4( 0.06f, 0.07f, 0.08f, 0.8f );
  gui_style.Colors[ImGuiCol_PlotHistogram] = ImVec4( 0.20f, 0.40f, 0.60f, 1.0f );
  gui_style.Colors[ImGuiCol_PlotHistogramHovered] = ImVec4( 0.20f, 0.45f, 0.90f, 1.0f );

  // Short cuts.
  Application const& application = owning_window->application();
  LogicalDevice const& device = owning_window->logical_device();
  Queue const& queue = owning_window->presentation_surface().graphics_queue();

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

  ImGui_ImplVulkan_Init(&init_info, owning_window->swapchain().vh_render_pass());
}

ImGui::~ImGui()
{
  ImGui_ImplVulkan_Shutdown();
  ::ImGui::DestroyContext();
}

} // namespace vulkan
