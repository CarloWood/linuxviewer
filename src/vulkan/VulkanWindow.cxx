#include "sys.h"
#include "VulkanWindow.h"
#include "OperatingSystem.h"
#include "LogicalDevice.h"
#include "Application.h"
#include "FrameResourcesData.h"
#include "VertexData.h"
#include "StagingBufferParameters.h"
#include "vk_utils/get_image_data.h"
#include "vk_utils/print_flags.h"
#include "xcb-task/ConnectionBrokerKey.h"
#ifdef CWDEBUG
#include "utils/debug_ostream_operators.h"
#endif

namespace task {

VulkanWindow::VulkanWindow(vulkan::Application* application, std::unique_ptr<linuxviewer::OS::Window>&& window COMMA_CWDEBUG_ONLY(bool debug)) :
  AIStatefulTask(CWDEBUG_ONLY(debug)), m_application(application), m_window(std::move(window)), m_frame_rate_limiter([this](){ signal(frame_timer); }) COMMA_CWDEBUG_ONLY(mVWDebug(mSMDebug))
{
  DoutEntering(dc::statefultask(mSMDebug), "task::VulkanWindow::VulkanWindow(" << application << ", " << (void*)m_window.get() << ") [" << (void*)this << "]");
  // Don't do that.
  ASSERT(m_window);
}

VulkanWindow::~VulkanWindow()
{
  DoutEntering(dc::statefultask(mSMDebug), "task::VulkanWindow::~VulkanWindow() [" << (void*)this << "]");
  m_frame_rate_limiter.stop();
  m_window->destroy();
}

vulkan::LogicalDevice* VulkanWindow::get_logical_device() const
{
  return m_application->get_logical_device(get_logical_device_index());
}

vk::Extent2D VulkanWindow::get_extent() const
{
  return m_window->get_extent();
}

char const* VulkanWindow::state_str_impl(state_type run_state) const
{
  switch (run_state)
  {
    AI_CASE_RETURN(VulkanWindow_xcb_connection);
    AI_CASE_RETURN(VulkanWindow_create);
    AI_CASE_RETURN(VulkanWindow_logical_device_index_available);
    AI_CASE_RETURN(VulkanWindow_acquire_queues);
    AI_CASE_RETURN(VulkanWindow_create_swapchain);
    AI_CASE_RETURN(VulkanWindow_render_loop);
    AI_CASE_RETURN(VulkanWindow_close);
  }
  ASSERT(false);
  return "UNKNOWN STATE";
}

void VulkanWindow::initialize_impl()
{
  DoutEntering(dc::vulkan, "VulkanWindow::initialize_impl() [" << this << "]");

  try
  {
    m_application->add(this);
  }
  catch (AIAlert::Error const& error)
  {
    Dout(dc::warning, error);
    abort();
    return;
  }
  set_state(VulkanWindow_xcb_connection);
}

void VulkanWindow::multiplex_impl(state_type run_state)
{
  switch (run_state)
  {
    case VulkanWindow_xcb_connection:
      // Get the- or create a task::XcbConnection object that is associated with m_broker_key (ie DISPLAY).
      m_xcb_connection_task = m_broker->run(*m_broker_key, [this](bool success){ Dout(dc::notice, "xcb_connection finished!"); signal(connection_set_up); });
      // Wait until the connection with the X server is established, then continue with VulkanWindow_create.
      set_state(VulkanWindow_create);
      wait(connection_set_up);
      break;
    case VulkanWindow_create:
      // Create a new xcb window using the established connection.
      m_window->set_xcb_connection(m_xcb_connection_task->connection());
      m_presentation_surface = m_window->create(m_application->vh_instance(), m_title, m_extent.width, m_extent.height, this);
      // Trigger the "window created" event.
      m_window_created_event.trigger();
      // If a logical device was passed then we need to copy its index as soon as that becomes available.
      if (m_logical_device_task)
      {
        // Wait until the logical device index becomes available on m_logical_device.
        m_logical_device_task->m_logical_device_index_available_event.register_task(this, logical_device_index_available);
        set_state(VulkanWindow_logical_device_index_available);
        wait(logical_device_index_available);
        break;
      }
      // Otherwise we can continue with acquiring the swapchain queues.
      set_state(VulkanWindow_acquire_queues);
      // But wait until the logical_device_index is set, if it wasn't already.
      if (m_logical_device_index.load(std::memory_order::relaxed) == -1)
        wait(logical_device_index_available);
      break;
    case VulkanWindow_logical_device_index_available:
      // The logical device index is available. Copy it.
      set_logical_device_index(m_logical_device_task->get_index());
      // Create the swapchain.
      set_state(VulkanWindow_acquire_queues);
      [[fallthrough]];
    case VulkanWindow_acquire_queues:
      acquire_queues();
      if (m_logical_device_task)
      {
        // We just linked m_logical_device_task and this window by passing it to Application::create_root_window, without ever
        // really verifying that presentation to this window is supported.
        // But the Vulkan spec states: [a swapchain] surface must be a surface that is supported by the device as determined
        // using vkGetPhysicalDeviceSurfaceSupportKHR. So, in order to keep the validation layer happy, we verify that here.
        if (AI_UNLIKELY(!m_application->get_logical_device(get_logical_device_index())->verify_presentation_support(m_presentation_surface)))
        {
          // This should never happen; not sure if I can recover from it when it does.
          abort();
          break;
        }
      }
      set_state(VulkanWindow_create_swapchain);
      [[fallthrough]];
    case VulkanWindow_create_swapchain:
      create_swapchain();
      set_state(VulkanWindow_render_loop);
      // Turn off debug output for this statefultask while processing the render loop.
      Debug(mSMDebug = false);
      break;
    case VulkanWindow_render_loop:
      if (AI_LIKELY(!m_must_close && m_swapchain.can_render()))
      {
        m_frame_rate_limiter.start(m_frame_rate_interval);
        m_window->draw_frame();
        yield(m_application->m_medium_priority_queue);
        wait(frame_timer);
        break;
      }
      if (!m_must_close)
      {
        // We can't render, drop frame rate to 7.8 FPS (because slow_down already uses 128 ms anyway).
        static threadpool::Timer::Interval s_no_render_frame_rate_interval{threadpool::Interval<128, std::chrono::milliseconds>()};
        m_frame_rate_limiter.start(s_no_render_frame_rate_interval);
        yield(m_application->m_low_priority_queue);
        wait(frame_timer);
        break;
      }
      set_state(VulkanWindow_close);
      [[fallthrough]];
    case VulkanWindow_close:
      // Turn on debug output again.
      Debug(mSMDebug = mVWDebug);
      finish();
      break;
  }
}

void VulkanWindow::acquire_queues()
{
  DoutEntering(dc::vulkan, "VulkanWindow::acquire_queues()");
  using vulkan::QueueFlagBits;

  vulkan::LogicalDevice const* logical_device = get_logical_device();

  vulkan::Queue vh_graphics_queue = logical_device->acquire_queue(QueueFlagBits::eGraphics|QueueFlagBits::ePresentation, m_window_cookie);
  vulkan::Queue vh_presentation_queue;

  if (!vh_graphics_queue)
  {
    // The combination of eGraphics|ePresentation failed. We have to try separately.
    vh_graphics_queue = logical_device->acquire_queue(QueueFlagBits::eGraphics, m_window_cookie);
    vh_presentation_queue = logical_device->acquire_queue(QueueFlagBits::ePresentation, m_window_cookie);
  }
  else
    vh_presentation_queue = vh_graphics_queue;

  m_presentation_surface.set_queues(vh_graphics_queue, vh_presentation_queue);
}

void VulkanWindow::create_swapchain()
{
  DoutEntering(dc::vulkan, "VulkanWindow::create_swapchain()");
  m_swapchain.prepare(this, vk::ImageUsageFlagBits::eColorAttachment, vk::PresentModeKHR::eFifo);
}

void VulkanWindow::OnWindowSizeChanged()
{
  DoutEntering(dc::vulkan, "VulkanWindow::OnWindowSizeChanged()");
  int logical_device_index = m_logical_device_index.load(std::memory_order::relaxed);
  if (logical_device_index == -1)
    return;
  vulkan::LogicalDevice* logical_device = m_application->get_logical_device(logical_device_index);
  logical_device->wait_idle();
  m_window->OnWindowSizeChanged_Pre();
  m_swapchain.recreate(this, m_window->get_extent());
  if (m_swapchain.can_render())
    m_window->OnWindowSizeChanged_Post();
}

void VulkanWindow::OnSampleWindowSizeChanged_Pre()
{
  DoutEntering(dc::vulkan, "VulkanWindow::OnSampleWindowSizeChanged_Pre()");
}

void VulkanWindow::OnSampleWindowSizeChanged_Post()
{
  DoutEntering(dc::vulkan, "VulkanWindow::OnSampleWindowSizeChanged_Post()");

  vk::ImageSubresourceRange image_subresource_range;
  image_subresource_range
    .setBaseMipLevel(0)
    .setLevelCount(1)
    .setBaseArrayLayer(0)
    .setLayerCount(1)
    ;

  // Create depth attachment and transition it away from an undefined layout.
  image_subresource_range.setAspectMask(vk::ImageAspectFlagBits::eDepth);
  for (std::unique_ptr<vulkan::FrameResourcesData> const& frame_resources_data : m_frame_resources)
  {
    frame_resources_data->m_depth_attachment = get_logical_device()->create_image(
        m_swapchain.extent().width,
        m_swapchain.extent().height,
        s_default_depth_format,
        vk::ImageUsageFlagBits::eDepthStencilAttachment,
        vk::MemoryPropertyFlagBits::eDeviceLocal,
        vk::ImageAspectFlagBits::eDepth);
    set_image_memory_barrier(
        *frame_resources_data->m_depth_attachment.m_image,
        image_subresource_range,
        vk::ImageLayout::eUndefined,
        vk::AccessFlags(0),
        vk::PipelineStageFlagBits::eTopOfPipe,
        vk::ImageLayout::eDepthStencilAttachmentOptimal,
        vk::AccessFlagBits::eDepthStencilAttachmentWrite,
        vk::PipelineStageFlagBits::eEarlyFragmentTests);
  }

  // Pre-transition all swapchain images away from an undefined layout.
  image_subresource_range.setAspectMask(vk::ImageAspectFlagBits::eColor);
  m_swapchain.set_image_memory_barriers(
      this,
      image_subresource_range,
      vk::ImageLayout::eUndefined,
      vk::AccessFlagBits::eNoneKHR,
      vk::PipelineStageFlagBits::eTopOfPipe,
      vk::ImageLayout::ePresentSrcKHR,
      vk::AccessFlagBits::eMemoryRead,
      vk::PipelineStageFlagBits::eBottomOfPipe);
}

void VulkanWindow::set_image_memory_barrier(
    vk::Image vh_image,
    vk::ImageSubresourceRange const& image_subresource_range,
    vk::ImageLayout current_image_layout,
    vk::AccessFlags current_image_access,
    vk::PipelineStageFlags generating_stages,
    vk::ImageLayout new_image_layout,
    vk::AccessFlags new_image_access,
    vk::PipelineStageFlags consuming_stages) const
{
  // Allocate temporary command buffer from a temporary command pool.
  vulkan::LogicalDevice* logical_device = get_logical_device();
  vk::UniqueCommandPool command_pool = logical_device->create_command_pool(m_presentation_surface.queue_family_indices()[0], vk::CommandPoolCreateFlagBits::eTransient);
  vk::UniqueCommandBuffer command_buffer = std::move(logical_device->allocate_command_buffers(*command_pool, vk::CommandBufferLevel::ePrimary, 1 )[0]);

  // Record command buffer which copies data from the staging buffer to the destination buffer.
  {
    command_buffer->begin({ .flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit });

    vk::ImageMemoryBarrier image_memory_barrier{
      .srcAccessMask = current_image_access,
      .dstAccessMask = new_image_access,
      .oldLayout = current_image_layout,
      .newLayout = new_image_layout,
      .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
      .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
      .image = vh_image,
      .subresourceRange = image_subresource_range
    };
    command_buffer->pipelineBarrier(generating_stages, consuming_stages, {}, {}, {}, { image_memory_barrier });

    command_buffer->end();
  }
  // Submit
  {
    auto fence = logical_device->create_fence(false);

    vk::SubmitInfo submit_info{
      .waitSemaphoreCount = 0,
      .pWaitSemaphores = nullptr,
      .pWaitDstStageMask = nullptr,
      .commandBufferCount = 1,
      .pCommandBuffers = &(*command_buffer),
      .signalSemaphoreCount = 0,
      .pSignalSemaphores = nullptr
    };
    m_presentation_surface.vh_graphics_queue().submit({ submit_info }, *fence);
    if (logical_device->wait_for_fences( { *fence }, VK_FALSE, 3000000000 ) != vk::Result::eSuccess)
      throw std::runtime_error("waitForFences");
  }
}

void VulkanWindow::create_frame_resources()
{
  DoutEntering(dc::vulkan, "VulkanWindow::create_frame_resources() [" << this << "]");

  // We only draw do things for the first window.
  vulkan::LogicalDevice const* logical_device = get_logical_device();

  for (size_t i = 0; i < m_frame_resources.size(); ++i)
  {
    m_frame_resources[i] = std::make_unique<vulkan::FrameResourcesData>();
    auto& frame_resources = m_frame_resources[i];

    frame_resources->m_image_available_semaphore = logical_device->create_semaphore();
    frame_resources->m_finished_rendering_semaphore = logical_device->create_semaphore();
    frame_resources->m_fence = logical_device->create_fence(true);
    // Too specialized (should this be part of a derived class?)
    frame_resources->m_command_pool =
      logical_device->create_command_pool(
          m_presentation_surface.queue_family_indices()[0],
          vk::CommandPoolCreateFlagBits::eResetCommandBuffer | vk::CommandPoolCreateFlagBits::eTransient);
    frame_resources->m_pre_command_buffer = std::move(logical_device->allocate_command_buffers(*frame_resources->m_command_pool, vk::CommandBufferLevel::ePrimary, 1)[0]);
    frame_resources->m_post_command_buffer = std::move(logical_device->allocate_command_buffers(*frame_resources->m_command_pool, vk::CommandBufferLevel::ePrimary, 1)[0]);
  }

  OnSampleWindowSizeChanged_Post();
}

void VulkanWindow::create_render_passes()
{
  DoutEntering(dc::vulkan, "VulkanWindow::create_render_passes() [" << this << "]");

  vulkan::LogicalDevice const* logical_device = get_logical_device();

  std::vector<vulkan::RenderPassSubpassData> subpass_descriptions = {
    {
      {},                                                         // std::vector<VkAttachmentReference> const& InputAttachments
      {                                                           // std::vector<VkAttachmentReference> const& ColorAttachments
        {
          .attachment = 0,
          .layout = vk::ImageLayout::eColorAttachmentOptimal
        }
      },
      {                                                           // VkAttachmentReference const& DepthStencilAttachment;
        .attachment = 1,
        .layout = vk::ImageLayout::eDepthStencilAttachmentOptimal
      }
    }
  };

  std::vector<vk::SubpassDependency> dependencies = {
    {
      .srcSubpass = VK_SUBPASS_EXTERNAL,
      .dstSubpass = 0,
      .srcStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput,
      .dstStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput,
      .srcAccessMask = vk::AccessFlagBits::eColorAttachmentWrite,
      .dstAccessMask = vk::AccessFlagBits::eColorAttachmentWrite,
      .dependencyFlags = vk::DependencyFlagBits::eByRegion
    },
    {
      .srcSubpass = 0,
      .dstSubpass = VK_SUBPASS_EXTERNAL,
      .srcStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput,
      .dstStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput,
      .srcAccessMask = vk::AccessFlagBits::eColorAttachmentWrite,
      .dstAccessMask = vk::AccessFlagBits::eColorAttachmentWrite,
      .dependencyFlags = vk::DependencyFlagBits::eByRegion
    }
  };

  // Render pass - from present_src to color_attachment.
  {
    std::vector<vulkan::RenderPassAttachmentData> attachment_descriptions = {
      {
        .m_format = m_swapchain.format(),
        .m_load_op = vk::AttachmentLoadOp::eClear,
        .m_store_op = vk::AttachmentStoreOp::eStore,
        .m_initial_layout = vk::ImageLayout::ePresentSrcKHR,
        .m_final_layout = vk::ImageLayout::eColorAttachmentOptimal
      },
      {
        .m_format = s_default_depth_format,
        .m_load_op = vk::AttachmentLoadOp::eClear,
        .m_store_op = vk::AttachmentStoreOp::eStore,
        .m_initial_layout = vk::ImageLayout::eDepthStencilAttachmentOptimal,
        .m_final_layout = vk::ImageLayout::eDepthStencilAttachmentOptimal
      }
    };
    m_render_pass = logical_device->create_render_pass(attachment_descriptions, subpass_descriptions, dependencies);
  }

  // Post-render pass - from color_attachment to present_src.
  {
    std::vector<vulkan::RenderPassAttachmentData> attachment_descriptions = {
      {
        .m_format = m_swapchain.format(),
        .m_load_op = vk::AttachmentLoadOp::eLoad,
        .m_store_op = vk::AttachmentStoreOp::eStore,
        .m_initial_layout = vk::ImageLayout::eColorAttachmentOptimal,
        .m_final_layout = vk::ImageLayout::ePresentSrcKHR
      },
      {
        .m_format = s_default_depth_format,
        .m_load_op = vk::AttachmentLoadOp::eDontCare,
        .m_store_op = vk::AttachmentStoreOp::eDontCare,
        .m_initial_layout = vk::ImageLayout::eDepthStencilAttachmentOptimal,
        .m_final_layout = vk::ImageLayout::eDepthStencilAttachmentOptimal
      }
    };
    m_post_render_pass = logical_device->create_render_pass(attachment_descriptions, subpass_descriptions, dependencies);
  }
}

void VulkanWindow::create_descriptor_set()
{
  DoutEntering(dc::vulkan, "VulkanWindow::create_descriptor_set() [" << this << "]");

  vulkan::LogicalDevice const* logical_device = get_logical_device();

  std::vector<vk::DescriptorSetLayoutBinding> layout_bindings = {
    {
      .binding = 0,
      .descriptorType = vk::DescriptorType::eCombinedImageSampler,
      .descriptorCount = 1,
      .stageFlags = vk::ShaderStageFlagBits::eFragment,
      .pImmutableSamplers = nullptr
    },
    {
      .binding = 1,
      .descriptorType = vk::DescriptorType::eCombinedImageSampler,
      .descriptorCount = 1,
      .stageFlags = vk::ShaderStageFlagBits::eFragment,
      .pImmutableSamplers = nullptr
    }
  };
  std::vector<vk::DescriptorPoolSize> pool_sizes = {
  {
    .type = vk::DescriptorType::eCombinedImageSampler,
    .descriptorCount = 2
  }};

  m_descriptor_set = logical_device->create_descriptor_resources(layout_bindings, pool_sizes);
}

void VulkanWindow::create_textures()
{
  DoutEntering(dc::vulkan, "VulkanWindow::create_textures() [" << this << "]");

  vulkan::LogicalDevice const* logical_device = get_logical_device();

  // Background texture.
  {
    int width = 0, height = 0, data_size = 0;
    std::vector<char> texture_data = vk_utils::get_image_data(m_application->resources_path() / "textures/background.png", 4, &width, &height, nullptr, &data_size);
    // Create descriptor resources.
    {
      m_background_texture =
        logical_device->create_image(
            width, height,
            vk::Format::eR8G8B8A8Unorm,
            vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled,
            vk::MemoryPropertyFlagBits::eDeviceLocal,
            vk::ImageAspectFlagBits::eColor);

      m_background_texture.m_sampler =
        logical_device->create_sampler(
            vk::SamplerMipmapMode::eNearest,
            vk::SamplerAddressMode::eClampToEdge,
            VK_FALSE);
    }
    // Copy data.
    {
      vk::ImageSubresourceRange image_subresource_range{
        .aspectMask = vk::ImageAspectFlagBits::eColor,
        .baseMipLevel = 0,
        .levelCount = 1,
        .baseArrayLayer = 0,
        .layerCount = 1
      };

      copy_data_to_image(data_size, texture_data.data(), *m_background_texture.m_image, width, height, image_subresource_range, vk::ImageLayout::eUndefined,
          vk::AccessFlags(0), vk::PipelineStageFlagBits::eTopOfPipe, vk::ImageLayout::eShaderReadOnlyOptimal, vk::AccessFlagBits::eShaderRead,
          vk::PipelineStageFlagBits::eFragmentShader);
    }
    // Update descriptor set.
    {
      std::vector<vk::DescriptorImageInfo> image_infos = {
        {
          .sampler = *m_background_texture.m_sampler,
          .imageView = *m_background_texture.m_image_view,
          .imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal
        }
      };
      logical_device->update_descriptor_set(*m_descriptor_set.m_handle, vk::DescriptorType::eCombinedImageSampler, 0, 0, image_infos);
    }
  }

  // Sample texture.
  {
    int width = 0, height = 0, data_size = 0;
    std::vector<char> texture_data = vk_utils::get_image_data(m_application->resources_path() / "textures/frame_resources.png", 4, &width, &height, nullptr, &data_size);
    // Create descriptor resources.
    {
      m_texture = logical_device->create_image(width, height, vk::Format::eR8G8B8A8Unorm, vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled,
          vk::MemoryPropertyFlagBits::eDeviceLocal, vk::ImageAspectFlagBits::eColor);
      m_texture.m_sampler = logical_device->create_sampler(vk::SamplerMipmapMode::eNearest, vk::SamplerAddressMode::eClampToEdge, VK_FALSE);
    }
    // Copy data.
    {
      vk::ImageSubresourceRange image_subresource_range{
        .aspectMask = vk::ImageAspectFlagBits::eColor,
        .baseMipLevel = 0,
        .levelCount = 1,
        .baseArrayLayer = 0,
        .layerCount = 1
      };
      copy_data_to_image(data_size, texture_data.data(), *m_texture.m_image, width, height, image_subresource_range, vk::ImageLayout::eUndefined,
          vk::AccessFlags(0), vk::PipelineStageFlagBits::eTopOfPipe, vk::ImageLayout::eShaderReadOnlyOptimal, vk::AccessFlagBits::eShaderRead,
          vk::PipelineStageFlagBits::eFragmentShader);
    }
    // Update descriptor set.
    {
      std::vector<vk::DescriptorImageInfo> image_infos = {
        {
          .sampler = *m_texture.m_sampler,
          .imageView = *m_texture.m_image_view,
          .imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal
        }
      };
      logical_device->update_descriptor_set(*m_descriptor_set.m_handle, vk::DescriptorType::eCombinedImageSampler, 1, 0, image_infos);
    }
  }
}

void VulkanWindow::create_pipeline_layout()
{
  DoutEntering(dc::vulkan, "VulkanWindow::create_pipeline_layout() [" << this << "]");

  vulkan::LogicalDevice const* logical_device = get_logical_device();

  vk::PushConstantRange push_constant_ranges{
    .stageFlags = vk::ShaderStageFlagBits::eVertex,
    .offset = 0,
    .size = 4
  };
  m_pipeline_layout = logical_device->create_pipeline_layout({ *m_descriptor_set.m_layout }, { push_constant_ranges });
}

void VulkanWindow::create_graphics_pipeline()
{
  DoutEntering(dc::vulkan, "VulkanWindow::create_graphics_pipeline() [" << this << "]");

  vulkan::LogicalDevice const* logical_device = get_logical_device();

  vk::UniqueShaderModule vertex_shader_module = logical_device->create_shader_module(m_application->resources_path() / "shaders/shader.vert.spv");
  vk::UniqueShaderModule fragment_shader_module = logical_device->create_shader_module(m_application->resources_path() / "shaders/shader.frag.spv");

  std::vector<vk::PipelineShaderStageCreateInfo> shader_stage_create_infos = {
    // Vertex shader.
    {
      .flags = vk::PipelineShaderStageCreateFlags(0),
      .stage = vk::ShaderStageFlagBits::eVertex,
      .module = *vertex_shader_module,
      .pName = "main"
    },
    // Fragment shader.
    {
      .flags = vk::PipelineShaderStageCreateFlags(0),
      .stage = vk::ShaderStageFlagBits::eFragment,
      .module = *fragment_shader_module,
      .pName = "main"
    }
  };

  std::vector<vk::VertexInputBindingDescription> vertex_binding_description = {
    {
      .binding = 0,
      .stride = sizeof(vulkan::VertexData),
      .inputRate = vk::VertexInputRate::eVertex
    },
    {
      .binding = 1,
      .stride = 4 * sizeof(float),
      .inputRate = vk::VertexInputRate::eInstance
    }
  };
  std::vector<vk::VertexInputAttributeDescription> vertex_attribute_descriptions = {
    {
      .location = 0,
      .binding = vertex_binding_description[0].binding,
      .format = vk::Format::eR32G32B32A32Sfloat,
      .offset = 0
    },
    {
      .location = 1,
      .binding = vertex_binding_description[0].binding,
      .format = vk::Format::eR32G32Sfloat,
      .offset = offsetof(vulkan::VertexData, Texcoords)
    },
    {
      .location = 2,
      .binding = vertex_binding_description[1].binding,
      .format = vk::Format::eR32G32B32A32Sfloat,
      .offset = 0
    }
  };

  vk::PipelineVertexInputStateCreateInfo vertex_input_state_create_info{
    .flags = vk::PipelineVertexInputStateCreateFlags(0),
    .vertexBindingDescriptionCount = static_cast<uint32_t>(vertex_binding_description.size()),
    .pVertexBindingDescriptions = vertex_binding_description.data(),
    .vertexAttributeDescriptionCount = static_cast<uint32_t>(vertex_attribute_descriptions.size()),
    .pVertexAttributeDescriptions = vertex_attribute_descriptions.data()
  };

  vk::PipelineInputAssemblyStateCreateInfo input_assembly_state_create_info{
    .flags = vk::PipelineInputAssemblyStateCreateFlags(0),
    .topology = vk::PrimitiveTopology::eTriangleList,
    .primitiveRestartEnable = VK_FALSE
  };

  vk::PipelineViewportStateCreateInfo viewport_state_create_info{
    .flags = vk::PipelineViewportStateCreateFlags(0),
    .viewportCount = 1,
    .pViewports = nullptr,
    .scissorCount = 1,
    .pScissors = nullptr
  };
  vk::PipelineRasterizationStateCreateInfo rasterization_state_create_info{
    .flags = vk::PipelineRasterizationStateCreateFlags(0),
    .depthClampEnable = VK_FALSE,
    .rasterizerDiscardEnable = VK_FALSE,
    .polygonMode = vk::PolygonMode::eFill,
    .cullMode = vk::CullModeFlagBits::eBack,
    .frontFace = vk::FrontFace::eCounterClockwise,
    .depthBiasEnable = VK_FALSE,
    .depthBiasConstantFactor = 0.0f,
    .depthBiasClamp = 0.0f,
    .depthBiasSlopeFactor = 0.0f,
    .lineWidth = 1.0f
  };

  vk::PipelineMultisampleStateCreateInfo multisample_state_create_info{
    .flags = vk::PipelineMultisampleStateCreateFlags(0),
    .rasterizationSamples = vk::SampleCountFlagBits::e1,
    .sampleShadingEnable = VK_FALSE,
    .minSampleShading = 1.0f,
    .pSampleMask = nullptr,
    .alphaToCoverageEnable = VK_FALSE,
    .alphaToOneEnable = VK_FALSE
  };

  vk::PipelineDepthStencilStateCreateInfo depth_stencil_state_create_info{
    .flags = vk::PipelineDepthStencilStateCreateFlags(0),
    .depthTestEnable = VK_TRUE,
    .depthWriteEnable = VK_TRUE,
    .depthCompareOp = vk::CompareOp::eLessOrEqual
  };

  vk::PipelineColorBlendAttachmentState color_blend_attachment_state{
    .blendEnable = VK_FALSE,
    .srcColorBlendFactor = vk::BlendFactor::eSrcAlpha,
    .dstColorBlendFactor = vk::BlendFactor::eOneMinusSrcAlpha,
    .colorBlendOp = vk::BlendOp::eAdd,
    .srcAlphaBlendFactor = vk::BlendFactor::eOne,
    .dstAlphaBlendFactor = vk::BlendFactor::eZero,
    .alphaBlendOp = vk::BlendOp::eAdd,
    .colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA
  };

  vk::PipelineColorBlendStateCreateInfo color_blend_state_create_info{
    .flags = vk::PipelineColorBlendStateCreateFlags(0),
    .logicOpEnable = VK_FALSE,
    .logicOp = vk::LogicOp::eCopy,
    .attachmentCount = 1,
    .pAttachments = &color_blend_attachment_state
  };

  std::vector<vk::DynamicState> dynamic_states = {
    vk::DynamicState::eViewport,
    vk::DynamicState::eScissor
  };

  vk::PipelineDynamicStateCreateInfo dynamic_state_create_info{
    .flags = vk::PipelineDynamicStateCreateFlags(0),
    .dynamicStateCount = static_cast<uint32_t>(dynamic_states.size()),
    .pDynamicStates = dynamic_states.data()
  };

  vk::GraphicsPipelineCreateInfo pipeline_create_info{
    .flags = vk::PipelineCreateFlags(0),
    .stageCount = static_cast<uint32_t>(shader_stage_create_infos.size()),
    .pStages = shader_stage_create_infos.data(),
    .pVertexInputState = &vertex_input_state_create_info,
    .pInputAssemblyState = &input_assembly_state_create_info,
    .pTessellationState = nullptr,
    .pViewportState = &viewport_state_create_info,
    .pRasterizationState = &rasterization_state_create_info,
    .pMultisampleState = &multisample_state_create_info,
    .pDepthStencilState = &depth_stencil_state_create_info,
    .pColorBlendState = &color_blend_state_create_info,
    .pDynamicState = &dynamic_state_create_info,
    .layout = *m_pipeline_layout,
    .renderPass = *m_render_pass,
    .subpass = 0,
    .basePipelineHandle = vk::Pipeline{},
    .basePipelineIndex = -1
  };

  m_graphics_pipeline = logical_device->handle().createGraphicsPipelineUnique(vk::PipelineCache{}, pipeline_create_info).value;
}

void VulkanWindow::copy_data_to_buffer(uint32_t data_size, void const* data, vk::Buffer target_buffer,
    vk::DeviceSize buffer_offset, vk::AccessFlags current_buffer_access, vk::PipelineStageFlags generating_stages,
    vk::AccessFlags new_buffer_access, vk::PipelineStageFlags consuming_stages) const
{
  DoutEntering(dc::vulkan, "VulkanWindow::copy_data_to_buffer(" << data_size << ", " << data << ", " << target_buffer << ", " <<
    buffer_offset << ", " << current_buffer_access << ", " << generating_stages << ", " <<
    new_buffer_access << ", " << consuming_stages << ") [" << this << "]");

  vulkan::LogicalDevice const* logical_device = get_logical_device();

  // Create staging buffer and map its memory to copy data from the CPU.
  vulkan::StagingBufferParameters staging_buffer;
  {
    staging_buffer.m_buffer = logical_device->create_buffer(data_size, vk::BufferUsageFlagBits::eTransferSrc, vk::MemoryPropertyFlagBits::eHostVisible);
    staging_buffer.m_pointer = logical_device->handle().mapMemory(*staging_buffer.m_buffer.m_memory, 0, data_size);

    std::memcpy(staging_buffer.m_pointer, data, data_size);

    vk::MappedMemoryRange memory_range{
      .memory = *staging_buffer.m_buffer.m_memory,
      .offset = 0,
      .size = VK_WHOLE_SIZE
    };
    logical_device->handle().flushMappedMemoryRanges({ memory_range });

    logical_device->handle().unmapMemory(*staging_buffer.m_buffer.m_memory);
  }
  // Allocate temporary command buffer from a temporary command pool.
  vk::UniqueCommandPool command_pool = logical_device->create_command_pool(m_presentation_surface.queue_family_indices()[0], vk::CommandPoolCreateFlagBits::eTransient);
  vk::UniqueCommandBuffer command_buffer = std::move(logical_device->allocate_command_buffers(*command_pool, vk::CommandBufferLevel::ePrimary, 1 )[0]);

  // Record command buffer which copies data from the staging buffer to the destination buffer.
  {
    command_buffer->begin({ .flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit });

    vk::BufferMemoryBarrier pre_transfer_buffer_memory_barrier{
      .srcAccessMask = current_buffer_access,
      .dstAccessMask = vk::AccessFlagBits::eTransferWrite,
      .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
      .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
      .buffer = target_buffer,
      .offset = buffer_offset,
      .size = data_size
    };
    command_buffer->pipelineBarrier(generating_stages, vk::PipelineStageFlagBits::eTransfer, vk::DependencyFlags( 0 ), {}, { pre_transfer_buffer_memory_barrier }, {});

    vk::BufferCopy buffer_copy_region{
      .srcOffset = 0,
      .dstOffset = buffer_offset,
      .size = data_size
    };
    command_buffer->copyBuffer(*staging_buffer.m_buffer.m_buffer, target_buffer, { buffer_copy_region });

    vk::BufferMemoryBarrier post_transfer_buffer_memory_barrier{
      .srcAccessMask = vk::AccessFlagBits::eTransferWrite,
      .dstAccessMask = new_buffer_access,
      .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
      .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
      .buffer = target_buffer,
      .offset = buffer_offset,
      .size = data_size
    };
    command_buffer->pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, consuming_stages, vk::DependencyFlags(0), {}, { post_transfer_buffer_memory_barrier }, {});
    command_buffer->end();
  }
  // Submit
  {
    vk::UniqueFence fence = logical_device->create_fence(false);

    vk::SubmitInfo submit_info{
      .waitSemaphoreCount = 0,
      .pWaitSemaphores = nullptr,
      .pWaitDstStageMask = nullptr,
      .commandBufferCount = 1,
      .pCommandBuffers = &(*command_buffer)
    };
    m_presentation_surface.vh_graphics_queue().submit( { submit_info }, *fence );
    if (logical_device->wait_for_fences({ *fence }, VK_FALSE, 3000000000) != vk::Result::eSuccess)
      throw std::runtime_error("waitForFences");
  }
}

void VulkanWindow::create_vertex_buffers()
{
  DoutEntering(dc::vulkan, "VulkanWindow::create_vertex_buffers() [" << this << "]");

  vulkan::LogicalDevice const* logical_device = get_logical_device();

  // 3D model
  std::vector<vulkan::VertexData> vertex_data;

  float const size = 0.12f;
  float step       = 2.0f * size / s_quad_tessellation;
  for (int x = 0; x < s_quad_tessellation; ++x)
  {
    for (int y = 0; y < s_quad_tessellation; ++y)
    {
      float pos_x = -size + x * step;
      float pos_y = -size + y * step;

      vertex_data.push_back({{pos_x, pos_y, 0.0f, 1.0f}, {static_cast<float>(x) / (s_quad_tessellation), static_cast<float>(y) / (s_quad_tessellation)}});
      vertex_data.push_back(
          {{pos_x, pos_y + step, 0.0f, 1.0f}, {static_cast<float>(x) / (s_quad_tessellation), static_cast<float>(y + 1) / (s_quad_tessellation)}});
      vertex_data.push_back(
          {{pos_x + step, pos_y, 0.0f, 1.0f}, {static_cast<float>(x + 1) / (s_quad_tessellation), static_cast<float>(y) / (s_quad_tessellation)}});
      vertex_data.push_back(
          {{pos_x + step, pos_y, 0.0f, 1.0f}, {static_cast<float>(x + 1) / (s_quad_tessellation), static_cast<float>(y) / (s_quad_tessellation)}});
      vertex_data.push_back(
          {{pos_x, pos_y + step, 0.0f, 1.0f}, {static_cast<float>(x) / (s_quad_tessellation), static_cast<float>(y + 1) / (s_quad_tessellation)}});
      vertex_data.push_back(
          {{pos_x + step, pos_y + step, 0.0f, 1.0f}, {static_cast<float>(x + 1) / (s_quad_tessellation), static_cast<float>(y + 1) / (s_quad_tessellation)}});
    }
  }

  m_vertex_buffer = logical_device->create_buffer(static_cast<uint32_t>(vertex_data.size()) * sizeof(vulkan::VertexData),
      vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eVertexBuffer, vk::MemoryPropertyFlagBits::eDeviceLocal);
  copy_data_to_buffer(static_cast<uint32_t>(vertex_data.size()) * sizeof(vulkan::VertexData), vertex_data.data(), *m_vertex_buffer.m_buffer, 0, vk::AccessFlags(0),
      vk::PipelineStageFlagBits::eTopOfPipe, vk::AccessFlagBits::eVertexAttributeRead, vk::PipelineStageFlagBits::eVertexInput);

  // Per instance data (position offsets and distance)
  std::vector<float> instance_data(s_max_objects_count * 4);
  for (size_t i = 0; i < instance_data.size(); i += 4)
  {
    instance_data[i]     = static_cast<float>(std::rand() % 513) / 256.0f - 1.0f;
    instance_data[i + 1] = static_cast<float>(std::rand() % 513) / 256.0f - 1.0f;
    instance_data[i + 2] = static_cast<float>(std::rand() % 513) / 512.0f;
    instance_data[i + 3] = 0.0f;
  }

  m_instance_buffer = logical_device->create_buffer(static_cast<uint32_t>(instance_data.size()) * sizeof(float),
      vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eVertexBuffer, vk::MemoryPropertyFlagBits::eDeviceLocal);
  copy_data_to_buffer(static_cast<uint32_t>(instance_data.size()) * sizeof(float), instance_data.data(), *m_instance_buffer.m_buffer, 0, vk::AccessFlags(0),
      vk::PipelineStageFlagBits::eTopOfPipe, vk::AccessFlagBits::eVertexAttributeRead, vk::PipelineStageFlagBits::eVertexInput);
}

void VulkanWindow::copy_data_to_image(uint32_t data_size, void const* data, vk::Image target_image,
    uint32_t width, uint32_t height, vk::ImageSubresourceRange const& image_subresource_range,
    vk::ImageLayout current_image_layout, vk::AccessFlags current_image_access, vk::PipelineStageFlags generating_stages,
    vk::ImageLayout new_image_layout, vk::AccessFlags new_image_access, vk::PipelineStageFlags consuming_stages) const
{
  DoutEntering(dc::vulkan, "VulkanWindow::copy_data_to_image(" << data_size << ", " << data << ", " << target_image << ", " << width << ", " << height << ", " <<
      image_subresource_range << ", " << current_image_layout << ", " << current_image_access << ", " << generating_stages << ", " <<
      new_image_layout << ", " << new_image_access << ", " << consuming_stages << ")");

  vulkan::LogicalDevice const* logical_device = get_logical_device();

  // Create staging buffer and map it's memory to copy data from the CPU.
  vulkan::StagingBufferParameters staging_buffer;
  {
    staging_buffer.m_buffer = logical_device->create_buffer(data_size, vk::BufferUsageFlagBits::eTransferSrc, vk::MemoryPropertyFlagBits::eHostVisible);
    staging_buffer.m_pointer = logical_device->handle().mapMemory(*staging_buffer.m_buffer.m_memory, 0, data_size);

    std::memcpy(staging_buffer.m_pointer, data, data_size);

    vk::MappedMemoryRange memory_range{
        .memory = *staging_buffer.m_buffer.m_memory,
        .offset = 0,
        .size = VK_WHOLE_SIZE
    };
    logical_device->handle().flushMappedMemoryRanges({ memory_range });

    logical_device->handle().unmapMemory(*staging_buffer.m_buffer.m_memory);
  }

  // Allocate temporary command buffer from a temporary command pool
  vk::UniqueCommandPool command_pool = logical_device->create_command_pool(m_presentation_surface.queue_family_indices()[0], vk::CommandPoolCreateFlagBits::eTransient);
  vk::UniqueCommandBuffer command_buffer = std::move(logical_device->allocate_command_buffers(*command_pool, vk::CommandBufferLevel::ePrimary, 1 )[0]);

  // Record command buffer which copies data from the staging buffer to the destination buffer.
  {
    command_buffer->begin({ .flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit });

    vk::ImageMemoryBarrier pre_transfer_image_memory_barrier{
      .srcAccessMask = current_image_access,
      .dstAccessMask = vk::AccessFlagBits::eTransferWrite,
      .oldLayout = current_image_layout,
      .newLayout = vk::ImageLayout::eTransferDstOptimal,
      .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
      .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
      .image = target_image,
      .subresourceRange = image_subresource_range
    };
    command_buffer->pipelineBarrier(generating_stages, vk::PipelineStageFlagBits::eTransfer, vk::DependencyFlags(0), {}, {}, { pre_transfer_image_memory_barrier });

    std::vector<vk::BufferImageCopy> buffer_image_copy;
    buffer_image_copy.reserve(image_subresource_range.levelCount);
    for (uint32_t i = image_subresource_range.baseMipLevel; i < image_subresource_range.baseMipLevel + image_subresource_range.levelCount; ++i)
    {
      buffer_image_copy.emplace_back(vk::BufferImageCopy{
        .bufferOffset = 0,
        .bufferRowLength = 0,
        .bufferImageHeight = 0,
        .imageSubresource = vk::ImageSubresourceLayers{
          .aspectMask = image_subresource_range.aspectMask,
          .mipLevel = i,
          .baseArrayLayer = image_subresource_range.baseArrayLayer,
          .layerCount = image_subresource_range.layerCount
        },
        .imageOffset = vk::Offset3D{},
        .imageExtent = vk::Extent3D{
          .width = width,
          .height = height,
          .depth = 1
        }
      });
    }
    command_buffer->copyBufferToImage(*staging_buffer.m_buffer.m_buffer, target_image, vk::ImageLayout::eTransferDstOptimal, buffer_image_copy);

    vk::ImageMemoryBarrier post_transfer_image_memory_barrier{
      .srcAccessMask = vk::AccessFlagBits::eTransferWrite,
      .dstAccessMask = new_image_access,
      .oldLayout = vk::ImageLayout::eTransferDstOptimal,
      .newLayout = new_image_layout,
      .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
      .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
      .image = target_image,
      .subresourceRange = image_subresource_range
    };
    command_buffer->pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, consuming_stages, vk::DependencyFlags(0), {}, {}, { post_transfer_image_memory_barrier });

    command_buffer->end();
  }

  // Submit
  {
    vk::UniqueFence fence = logical_device->create_fence(false);

    vk::SubmitInfo submit_info{
      .waitSemaphoreCount = 0,
      .pWaitSemaphores = nullptr,
      .pWaitDstStageMask = nullptr,
      .commandBufferCount = 1,
      .pCommandBuffers = &(*command_buffer)
    };
    m_presentation_surface.vh_graphics_queue().submit({ submit_info }, *fence);
    if (logical_device->wait_for_fences({ *fence }, VK_FALSE, 3000000000 ) != vk::Result::eSuccess)
      throw std::runtime_error("waitForFences");
  }
}

void VulkanWindow::finish_impl()
{
  DoutEntering(dc::vulkan, "VulkanWindow::finish_impl() [" << this << "]");
  // Not doing anything here, finishing the task causes us to leave the run()
  // function and then leave the scope of the boost::intrusive_ptr that keeps
  // this task alive. When it gets destructed, also Window will be destructed
  // and it's destructor does the work required to close the window.
  //
  // However, remove us from Application::window_list_t, or else this VulkanWindow
  // won't be destructed as the list stores boost::intrusive_ptr<task::VulkanWindow>'s.
  m_application->remove(this);
}

} // namespace task
