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
#include "debug/DebugSetName.h"
#ifdef CWDEBUG
#include "utils/debug_ostream_operators.h"
#endif

namespace task {

VulkanWindow::VulkanWindow(vulkan::Application* application COMMA_CWDEBUG_ONLY(bool debug)) :
  AIStatefulTask(CWDEBUG_ONLY(debug)), m_application(application), m_frame_rate_limiter([this](){ signal(frame_timer); }) COMMA_CWDEBUG_ONLY(mVWDebug(mSMDebug))
{
  DoutEntering(dc::statefultask(mSMDebug), "task::VulkanWindow::VulkanWindow(" << application << ") [" << (void*)this << "]");
}

VulkanWindow::~VulkanWindow()
{
  DoutEntering(dc::statefultask(mSMDebug), "task::VulkanWindow::~VulkanWindow() [" << (void*)this << "]");
  m_frame_rate_limiter.stop();
}

vulkan::LogicalDevice* VulkanWindow::get_logical_device() const
{
  DoutEntering(dc::vulkan, "VulkanWindow::get_logical_device() [" << this << "]");
  // This is a reasonable expensive call: it locks a mutex to access a vector.
  // Therefore VulkanWindow stores a copy that can be used from the render loop.
  return m_application->get_logical_device(get_logical_device_index());
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
    AI_CASE_RETURN(VulkanWindow_create_remaining_objects);
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

  m_frame_rate_interval = get_frame_rate_interval();
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
      linuxviewer::OS::Window::set_xcb_connection(m_xcb_connection_task->connection());
      m_presentation_surface = create(m_application->vh_instance(), m_title, m_extent.width, m_extent.height);
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
      // At this point the logical_device_index is available, which means we can call get_logical_device().
      // Cache the pointer to the vulkan::LogicalDevice.
      m_logical_device = get_logical_device();
      // From this moment on we can use the accessor logical_device().
      // Next get on with the real work.
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
      set_state(VulkanWindow_create_remaining_objects);
      [[fallthrough]];
    case VulkanWindow_create_remaining_objects:
      m_frame_resources_list.resize(5);   // FIXME: belongs in derived class.
      create_frame_resources();
      create_render_passes();
      create_descriptor_set();
      create_textures();
      create_pipeline_layout();
      create_graphics_pipeline();
      create_vertex_buffers();
      set_state(VulkanWindow_render_loop);
      // Turn off debug output for this statefultask while processing the render loop.
      Debug(mSMDebug = false);
      [[fallthrough]];
    case VulkanWindow_render_loop:
      if (AI_LIKELY(!m_must_close && m_swapchain.can_render()))
      {
        m_frame_rate_limiter.start(m_frame_rate_interval);
        draw_frame();
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

  vulkan::Queue vh_graphics_queue = logical_device().acquire_queue(QueueFlagBits::eGraphics|QueueFlagBits::ePresentation, m_window_cookie);
  vulkan::Queue vh_presentation_queue;

  if (!vh_graphics_queue)
  {
    // The combination of eGraphics|ePresentation failed. We have to try separately.
    vh_graphics_queue = logical_device().acquire_queue(QueueFlagBits::eGraphics, m_window_cookie);
    vh_presentation_queue = logical_device().acquire_queue(QueueFlagBits::ePresentation, m_window_cookie);
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

void VulkanWindow::OnWindowSizeChanged(uint32_t width, uint32_t height)
{
  DoutEntering(dc::vulkan, "VulkanWindow::OnWindowSizeChanged(" << width << ", " << height << ")");

  // Update m_extent so that extent() will return the new value.
  m_extent.setWidth(width).setHeight(height);

  int logical_device_index = m_logical_device_index.load(std::memory_order::relaxed);
  if (logical_device_index == -1)
    return;
  vulkan::LogicalDevice* logical_device = m_application->get_logical_device(logical_device_index);
  logical_device->wait_idle();
  OnWindowSizeChanged_Pre();
  m_swapchain.recreate(this, extent());
  if (m_swapchain.can_render())
    OnWindowSizeChanged_Post();
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
  DoutEntering(dc::vulkan, "VulkanWindow::set_image_memory_barrier(...)");

  // We use a temporary command pool here.
  using command_pool_type = vulkan::CommandPool<VK_COMMAND_POOL_CREATE_TRANSIENT_BIT>;

  // Construct the temporary command pool.
  command_pool_type tmp_command_pool(m_logical_device, m_presentation_surface.graphics_queue().queue_family()
        COMMA_CWDEBUG_ONLY(debug_name_prefix("set_image_memory_barrier()::tmp_command_pool")));

  // Take lock on command pool.
  command_pool_type::wat tmp_command_pool_w(tmp_command_pool);

  // Allocate a temporary command buffer from the temporary command pool.
  vulkan::handle::CommandBuffer tmp_command_buffer = tmp_command_pool_w->allocate_buffer(
      CWDEBUG_ONLY(debug_name_prefix("set_image_memory_barrier()::tmp_command_buffer")));

  // Accessor for the command buffer (this is a noop in Release mode, but checks that the right pool is currently locked in debug mode).
  auto tmp_command_buffer_w = tmp_command_buffer(tmp_command_pool_w);

  // Record command buffer which copies data from the staging buffer to the destination buffer.
  {
    tmp_command_buffer_w->begin({ .flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit });

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
    tmp_command_buffer_w->pipelineBarrier(generating_stages, consuming_stages, {}, {}, {}, { image_memory_barrier });

    tmp_command_buffer_w->end();
  }

  // Submit
  {
    auto fence = logical_device().create_fence(false
        COMMA_CWDEBUG_ONLY(debug_name_prefix("set_image_memory_barrier()::fence")));

    vk::SubmitInfo submit_info{
      .waitSemaphoreCount = 0,
      .pWaitSemaphores = nullptr,
      .pWaitDstStageMask = nullptr,
      .commandBufferCount = 1,
      .pCommandBuffers = tmp_command_buffer_w,
      .signalSemaphoreCount = 0,
      .pSignalSemaphores = nullptr
    };
    m_presentation_surface.vh_graphics_queue().submit({ submit_info }, *fence);
    if (logical_device().wait_for_fences( { *fence }, VK_FALSE, 3000000000 ) != vk::Result::eSuccess)
      throw std::runtime_error("waitForFences");
  }
}

void VulkanWindow::create_descriptor_set()
{
  DoutEntering(dc::vulkan, "VulkanWindow::create_descriptor_set() [" << this << "]");

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

  m_descriptor_set = logical_device().create_descriptor_resources(layout_bindings, pool_sizes
      COMMA_CWDEBUG_ONLY(debug_name_prefix("m_descriptor_set")));
}

void VulkanWindow::create_textures()
{
  DoutEntering(dc::vulkan, "VulkanWindow::create_textures() [" << this << "]");

  // Background texture.
  {
    int width = 0, height = 0, data_size = 0;
    std::vector<char> texture_data = vk_utils::get_image_data(m_application->resources_path() / "textures/background.png", 4, &width, &height, nullptr, &data_size);
    // Create descriptor resources.
    {
      m_background_texture =
        m_logical_device->create_image(
            width, height,
            vk::Format::eR8G8B8A8Unorm,
            vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled,
            vk::MemoryPropertyFlagBits::eDeviceLocal,
            vk::ImageAspectFlagBits::eColor
            COMMA_CWDEBUG_ONLY(debug_name_prefix("m_background_texture")));

      m_background_texture.m_sampler =
        m_logical_device->create_sampler(
            vk::SamplerMipmapMode::eNearest,
            vk::SamplerAddressMode::eClampToEdge,
            VK_FALSE
            COMMA_CWDEBUG_ONLY(debug_name_prefix("m_background_texture.m_sampler")));
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
      m_logical_device->update_descriptor_set(*m_descriptor_set.m_handle, vk::DescriptorType::eCombinedImageSampler, 0, 0, image_infos);
    }
  }

  // Sample texture.
  {
    int width = 0, height = 0, data_size = 0;
    std::vector<char> texture_data = vk_utils::get_image_data(m_application->resources_path() / "textures/frame_resources.png", 4, &width, &height, nullptr, &data_size);
    // Create descriptor resources.
    {
      m_texture = m_logical_device->create_image(width, height, vk::Format::eR8G8B8A8Unorm, vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled,
          vk::MemoryPropertyFlagBits::eDeviceLocal, vk::ImageAspectFlagBits::eColor
          COMMA_CWDEBUG_ONLY(debug_name_prefix("m_texture")));
      m_texture.m_sampler = m_logical_device->create_sampler(vk::SamplerMipmapMode::eNearest, vk::SamplerAddressMode::eClampToEdge, VK_FALSE
          COMMA_CWDEBUG_ONLY(debug_name_prefix("m_texture.m_sampler")));
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
      m_logical_device->update_descriptor_set(*m_descriptor_set.m_handle, vk::DescriptorType::eCombinedImageSampler, 1, 0, image_infos);
    }
  }
}

void VulkanWindow::create_pipeline_layout()
{
  DoutEntering(dc::vulkan, "VulkanWindow::create_pipeline_layout() [" << this << "]");

  vk::PushConstantRange push_constant_ranges{
    .stageFlags = vk::ShaderStageFlagBits::eVertex,
    .offset = 0,
    .size = 4
  };
  m_pipeline_layout = m_logical_device->create_pipeline_layout({ *m_descriptor_set.m_layout }, { push_constant_ranges }
      COMMA_CWDEBUG_ONLY(debug_name_prefix("m_pipeline_layout")));
}

void VulkanWindow::copy_data_to_buffer(uint32_t data_size, void const* data, vk::Buffer target_buffer,
    vk::DeviceSize buffer_offset, vk::AccessFlags current_buffer_access, vk::PipelineStageFlags generating_stages,
    vk::AccessFlags new_buffer_access, vk::PipelineStageFlags consuming_stages) const
{
  DoutEntering(dc::vulkan, "VulkanWindow::copy_data_to_buffer(" << data_size << ", " << data << ", " << target_buffer << ", " <<
    buffer_offset << ", " << current_buffer_access << ", " << generating_stages << ", " <<
    new_buffer_access << ", " << consuming_stages << ") [" << this << "]");

  // Create staging buffer and map its memory to copy data from the CPU.
  vulkan::StagingBufferParameters staging_buffer;
  {
    staging_buffer.m_buffer = m_logical_device->create_buffer(data_size, vk::BufferUsageFlagBits::eTransferSrc, vk::MemoryPropertyFlagBits::eHostVisible
        COMMA_CWDEBUG_ONLY(debug_name_prefix("copy_data_to_buffer()::staging_buffer.m_buffer")));
    staging_buffer.m_pointer = m_logical_device->handle().mapMemory(*staging_buffer.m_buffer.m_memory, 0, data_size);

    std::memcpy(staging_buffer.m_pointer, data, data_size);

    vk::MappedMemoryRange memory_range{
      .memory = *staging_buffer.m_buffer.m_memory,
      .offset = 0,
      .size = VK_WHOLE_SIZE
    };
    m_logical_device->handle().flushMappedMemoryRanges({ memory_range });

    m_logical_device->handle().unmapMemory(*staging_buffer.m_buffer.m_memory);
  }

  // We use a temporary command pool here.
  using command_pool_type = vulkan::CommandPool<VK_COMMAND_POOL_CREATE_TRANSIENT_BIT>;

  // Construct the temporary command pool.
  command_pool_type tmp_command_pool(m_logical_device, m_presentation_surface.graphics_queue().queue_family()
        COMMA_CWDEBUG_ONLY(debug_name_prefix("copy_data_to_buffer()::tmp_command_pool")));

  // Take lock on command pool.
  command_pool_type::wat tmp_command_pool_w(tmp_command_pool);

  // Allocate a temporary command buffer from the temporary command pool.
  vulkan::handle::CommandBuffer tmp_command_buffer = tmp_command_pool_w->allocate_buffer(
      CWDEBUG_ONLY(debug_name_prefix("copy_data_to_buffer()::tmp_command_buffer")));

  // Accessor for the command buffer (this is a noop in Release mode, but checks that the right pool is currently locked in debug mode).
  auto tmp_command_buffer_w = tmp_command_buffer(tmp_command_pool_w);

  // Record command buffer which copies data from the staging buffer to the destination buffer.
  {
    tmp_command_buffer_w->begin({ .flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit });

    vk::BufferMemoryBarrier pre_transfer_buffer_memory_barrier{
      .srcAccessMask = current_buffer_access,
      .dstAccessMask = vk::AccessFlagBits::eTransferWrite,
      .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
      .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
      .buffer = target_buffer,
      .offset = buffer_offset,
      .size = data_size
    };
    tmp_command_buffer_w->pipelineBarrier(generating_stages, vk::PipelineStageFlagBits::eTransfer, vk::DependencyFlags( 0 ), {}, { pre_transfer_buffer_memory_barrier }, {});

    vk::BufferCopy buffer_copy_region{
      .srcOffset = 0,
      .dstOffset = buffer_offset,
      .size = data_size
    };
    tmp_command_buffer_w->copyBuffer(*staging_buffer.m_buffer.m_buffer, target_buffer, { buffer_copy_region });

    vk::BufferMemoryBarrier post_transfer_buffer_memory_barrier{
      .srcAccessMask = vk::AccessFlagBits::eTransferWrite,
      .dstAccessMask = new_buffer_access,
      .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
      .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
      .buffer = target_buffer,
      .offset = buffer_offset,
      .size = data_size
    };
    tmp_command_buffer_w->pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, consuming_stages, vk::DependencyFlags(0), {}, { post_transfer_buffer_memory_barrier }, {});
    tmp_command_buffer_w->end();
  }
  // Submit
  {
    vk::UniqueFence fence = m_logical_device->create_fence(false COMMA_CWDEBUG_ONLY(debug_name_prefix("copy_data_to_buffer()::fence")));

    vk::SubmitInfo submit_info{
      .waitSemaphoreCount = 0,
      .pWaitSemaphores = nullptr,
      .pWaitDstStageMask = nullptr,
      .commandBufferCount = 1,
      .pCommandBuffers = tmp_command_buffer_w
    };
    m_presentation_surface.vh_graphics_queue().submit( { submit_info }, *fence );
    if (m_logical_device->wait_for_fences({ *fence }, VK_FALSE, 3000000000) != vk::Result::eSuccess)
      throw std::runtime_error("waitForFences");
  }
}

void VulkanWindow::copy_data_to_image(uint32_t data_size, void const* data, vk::Image target_image,
    uint32_t width, uint32_t height, vk::ImageSubresourceRange const& image_subresource_range,
    vk::ImageLayout current_image_layout, vk::AccessFlags current_image_access, vk::PipelineStageFlags generating_stages,
    vk::ImageLayout new_image_layout, vk::AccessFlags new_image_access, vk::PipelineStageFlags consuming_stages) const
{
  DoutEntering(dc::vulkan, "VulkanWindow::copy_data_to_image(" << data_size << ", " << data << ", " << target_image << ", " << width << ", " << height << ", " <<
      image_subresource_range << ", " << current_image_layout << ", " << current_image_access << ", " << generating_stages << ", " <<
      new_image_layout << ", " << new_image_access << ", " << consuming_stages << ")");

  // Create staging buffer and map it's memory to copy data from the CPU.
  vulkan::StagingBufferParameters staging_buffer;
  {
    staging_buffer.m_buffer = m_logical_device->create_buffer(data_size, vk::BufferUsageFlagBits::eTransferSrc, vk::MemoryPropertyFlagBits::eHostVisible
        COMMA_CWDEBUG_ONLY(debug_name_prefix("copy_data_to_image()::staging_buffer.m_buffer")));
    staging_buffer.m_pointer = m_logical_device->handle().mapMemory(*staging_buffer.m_buffer.m_memory, 0, data_size);

    std::memcpy(staging_buffer.m_pointer, data, data_size);

    vk::MappedMemoryRange memory_range{
        .memory = *staging_buffer.m_buffer.m_memory,
        .offset = 0,
        .size = VK_WHOLE_SIZE
    };
    m_logical_device->handle().flushMappedMemoryRanges({ memory_range });

    m_logical_device->handle().unmapMemory(*staging_buffer.m_buffer.m_memory);
  }

  // We use a temporary command pool here.
  using command_pool_type = vulkan::CommandPool<VK_COMMAND_POOL_CREATE_TRANSIENT_BIT>;

  // Construct the temporary command pool.
  command_pool_type tmp_command_pool(m_logical_device, m_presentation_surface.graphics_queue().queue_family()
        COMMA_CWDEBUG_ONLY(debug_name_prefix("copy_data_to_image()::tmp_command_pool")));

  // Take lock on command pool.
  command_pool_type::wat tmp_command_pool_w(tmp_command_pool);

  // Allocate a temporary command buffer from the temporary command pool.
  vulkan::handle::CommandBuffer tmp_command_buffer = tmp_command_pool_w->allocate_buffer(
      CWDEBUG_ONLY(debug_name_prefix("copy_data_to_image()::tmp_command_buffer")));

  // Accessor for the command buffer (this is a noop in Release mode, but checks that the right pool is currently locked in debug mode).
  auto tmp_command_buffer_w = tmp_command_buffer(tmp_command_pool_w);

  // Record command buffer which copies data from the staging buffer to the destination buffer.
  {
    tmp_command_buffer_w->begin({ .flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit });

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
    tmp_command_buffer_w->pipelineBarrier(generating_stages, vk::PipelineStageFlagBits::eTransfer, vk::DependencyFlags(0), {}, {}, { pre_transfer_image_memory_barrier });

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
    tmp_command_buffer_w->copyBufferToImage(*staging_buffer.m_buffer.m_buffer, target_image, vk::ImageLayout::eTransferDstOptimal, buffer_image_copy);

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
    tmp_command_buffer_w->pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, consuming_stages, vk::DependencyFlags(0), {}, {}, { post_transfer_image_memory_barrier });

    tmp_command_buffer_w->end();
  }

  // Submit
  {
    vk::UniqueFence fence = m_logical_device->create_fence(false COMMA_CWDEBUG_ONLY(debug_name_prefix("copy_data_to_image()::fence")));

    vk::SubmitInfo submit_info{
      .waitSemaphoreCount = 0,
      .pWaitSemaphores = nullptr,
      .pWaitDstStageMask = nullptr,
      .commandBufferCount = 1,
      .pCommandBuffers = tmp_command_buffer_w
    };
    m_presentation_surface.vh_graphics_queue().submit({ submit_info }, *fence);
    if (m_logical_device->wait_for_fences({ *fence }, VK_FALSE, 3000000000 ) != vk::Result::eSuccess)
      throw std::runtime_error("waitForFences");
  }
}

void VulkanWindow::start_frame()
{
  DoutEntering(dc::vulkan, "VulkanWindow::start_frame(...)");
#if 0
  Timer.Update();
  Gui.StartFrame( Timer, MouseState );
  PrepareGUIFrame();
#endif
  m_current_frame.m_resource_index = (m_current_frame.m_resource_index + 1) % m_current_frame.m_resource_count;
  m_current_frame.m_frame_resources = m_frame_resources_list[m_current_frame.m_resource_index].get();

  if (m_logical_device->wait_for_fences({ *m_current_frame.m_frame_resources->m_fence }, VK_FALSE, 1000000000) != vk::Result::eSuccess)
    throw std::runtime_error( "Waiting for a fence takes too long!" );

  m_logical_device->reset_fences({ *m_current_frame.m_frame_resources->m_fence });
}

void VulkanWindow::finish_frame(/* vulkan::handle::CommandBuffer command_buffer,*/ vk::RenderPass render_pass)
{
  DoutEntering(dc::vulkan, "VulkanWindow::finish_frame(...)");
  // Draw GUI
  {
//    Gui.Draw(m_current_frame.ResourceIndex, command_buffer, render_pass, *m_current_frame.FrameResources->Framebuffer);

    vk::PipelineStageFlags wait_dst_stage_mask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
    vk::SubmitInfo submit_info{
      .waitSemaphoreCount = 0,
      .pWaitSemaphores = nullptr,
      .pWaitDstStageMask = &wait_dst_stage_mask,
//      .commandBufferCount = 1,
//      .pCommandBuffers = &command_buffer,
      .signalSemaphoreCount = 1,
      .pSignalSemaphores = &(*m_current_frame.m_frame_resources->m_finished_rendering_semaphore)
    };
    m_presentation_surface.vh_presentation_queue().submit({ submit_info }, *m_current_frame.m_frame_resources->m_fence);
  }
  // Present frame
  {
    vk::Result result = vk::Result::eSuccess;
    vk::SwapchainKHR vh_swapchain = *m_swapchain;
    uint32_t swapchain_image_index = m_current_frame.m_swapchain_image_index.get_value();
    vk::PresentInfoKHR present_info{
      .waitSemaphoreCount = 1,
      .pWaitSemaphores = &(*m_current_frame.m_frame_resources->m_finished_rendering_semaphore),
      .swapchainCount = 1,
      .pSwapchains = &vh_swapchain,
      .pImageIndices = &swapchain_image_index
    };

    switch (m_presentation_surface.vh_presentation_queue().presentKHR(&present_info))
    {
      case vk::Result::eSuccess:
        break;
      case vk::Result::eSuboptimalKHR:
      case vk::Result::eErrorOutOfDateKHR:
      {
        // Same as the tail of OnWindowSizeChanged.
        m_logical_device->wait_idle();
        OnWindowSizeChanged_Pre();
        m_swapchain.recreate(this, extent());
        if (m_swapchain.can_render())
          OnWindowSizeChanged_Post();
        break;
      }
      default:
        throw std::runtime_error("Could not present swapchain image!");
    }
  }
}

vk::UniqueFramebuffer VulkanWindow::create_framebuffer(std::vector<vk::ImageView> const& image_views, vk::Extent2D const& extent, vk::RenderPass render_pass
    COMMA_CWDEBUG_ONLY(vulkan::AmbifixOwner const& debug_name)) const
{
  DoutEntering(dc::vulkan, "VulkanWindow::create_framebuffer(...)");
  vk::FramebufferCreateInfo framebuffer_create_info{
    .flags = vk::FramebufferCreateFlags(0),
    .renderPass = render_pass,
    .attachmentCount = static_cast<uint32_t>(image_views.size()),
    .pAttachments = image_views.data(),
    .width = extent.width,
    .height = extent.height,
    .layers = 1
  };

  vk::UniqueFramebuffer framebuffer = m_logical_device->handle().createFramebufferUnique(framebuffer_create_info);
  DebugSetName(framebuffer, debug_name);
  return framebuffer;
}

void VulkanWindow::acquire_image(vk::RenderPass render_pass)
{
  DoutEntering(dc::vulkan, "VulkanWindow::acquire_image(...) [" << this << "]");

  // Acquire swapchain image.
  switch (m_logical_device->acquire_next_image(
        *m_swapchain,
        3000000000,
        *m_current_frame.m_frame_resources->m_image_available_semaphore,
        vk::Fence(),
        m_current_frame.m_swapchain_image_index))
  {
    case vk::Result::eSuccess:
    case vk::Result::eSuboptimalKHR:
      break;
    case vk::Result::eErrorOutOfDateKHR:
      // Same as the tail of OnWindowSizeChanged.
      m_logical_device->wait_idle();
      OnWindowSizeChanged_Pre();
      m_swapchain.recreate(this, extent());
      if (m_swapchain.can_render())
        OnWindowSizeChanged_Post();
      break;
    default:
      throw std::runtime_error("Could not acquire swapchain image!");
  }
  // Create a framebuffer for current frame.
  m_current_frame.m_frame_resources->m_framebuffer = create_framebuffer(
      { *m_swapchain.image_views()[m_current_frame.m_swapchain_image_index],
        *m_current_frame.m_frame_resources->m_depth_attachment.m_image_view },
      m_swapchain.extent(), render_pass
      COMMA_CWDEBUG_ONLY(debug_name_prefix("m_current_frame.m_frame_resources->m_framebuffer")));
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

//virtual
threadpool::Timer::Interval VulkanWindow::get_frame_rate_interval() const
{
  return threadpool::Interval<10, std::chrono::milliseconds>{};
}

void VulkanWindow::OnWindowSizeChanged_Pre()
{
  DoutEntering(dc::vulkan, "VulkanWindow::OnWindowSizeChanged_Pre()");
}

void VulkanWindow::OnWindowSizeChanged_Post()
{
  DoutEntering(dc::vulkan, "VulkanWindow::OnWindowSizeChanged_Post()");

  // Should already be set.
  ASSERT(swapchain().extent().width > 0);

//FIXME, add m_gui    m_gui.OnWindowSizeChanged();

  vk::ImageSubresourceRange image_subresource_range;
  image_subresource_range
    .setBaseMipLevel(0)
    .setLevelCount(1)
    .setBaseArrayLayer(0)
    .setLayerCount(1)
    ;

  // Create depth attachment and transition it away from an undefined layout.
  image_subresource_range.setAspectMask(vk::ImageAspectFlagBits::eDepth);
#ifdef CWDEBUG
  int i = 0;
#endif
  for (std::unique_ptr<vulkan::FrameResourcesData> const& frame_resources_data : m_frame_resources_list)
  {
    frame_resources_data->m_depth_attachment = m_logical_device->create_image(
        swapchain().extent().width,
        swapchain().extent().height,
        s_default_depth_format,
        vk::ImageUsageFlagBits::eDepthStencilAttachment,
        vk::MemoryPropertyFlagBits::eDeviceLocal,
        vk::ImageAspectFlagBits::eDepth
        COMMA_CWDEBUG_ONLY(debug_name_prefix("m_frame_resources_list[" + std::to_string(i++) + "]->m_depth_attachment")));
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
  swapchain().set_image_memory_barriers(
      this,
      image_subresource_range,
      vk::ImageLayout::eUndefined,
      vk::AccessFlagBits::eNoneKHR,
      vk::PipelineStageFlagBits::eTopOfPipe,
      vk::ImageLayout::ePresentSrcKHR,
      vk::AccessFlagBits::eMemoryRead,
      vk::PipelineStageFlagBits::eBottomOfPipe);
}

void VulkanWindow::create_frame_resources()
{
  DoutEntering(dc::vulkan, "VulkanWindow::create_frame_resources() [" << this << "]");

  for (size_t i = 0; i < m_frame_resources_list.size(); ++i)
  {
#ifdef CWDEBUG
    vulkan::AmbifixOwner const ambifix = debug_name_prefix("m_frame_resources_list[" + std::to_string(i) + "]");
#endif
    m_frame_resources_list[i] = std::make_unique<vulkan::FrameResourcesData>(
        // Constructor arguments for m_command_pool. Too specialized? (should this be part of a derived class?)
        m_logical_device, m_presentation_surface.graphics_queue().queue_family()
        COMMA_CWDEBUG_ONLY(ambifix("->m_command_pool")));

    auto& frame_resources = m_frame_resources_list[i];

    frame_resources->m_image_available_semaphore = m_logical_device->create_semaphore(CWDEBUG_ONLY(ambifix("->m_image_available_semaphore")));
    frame_resources->m_finished_rendering_semaphore = m_logical_device->create_semaphore(CWDEBUG_ONLY(ambifix("->m_finished_rendering_semaphore")));
    frame_resources->m_fence = m_logical_device->create_fence(true COMMA_CWDEBUG_ONLY(ambifix("->m_fence")));

    {
      // Lock command pool.
      vulkan::FrameResourcesData::command_pool_type::wat command_pool_w(frame_resources->m_command_pool);

      frame_resources->m_pre_command_buffer = command_pool_w->allocate_buffer(
          CWDEBUG_ONLY(ambifix("->m_pre_command_buffer"))
          );

      frame_resources->m_post_command_buffer = command_pool_w->allocate_buffer(
          CWDEBUG_ONLY(ambifix("->m_post_command_buffer"))
          );
    } // Unlock command pool.
  }

  m_current_frame = vulkan::CurrentFrameData{
    .m_frame_resources = m_frame_resources_list[0].get(),
    .m_resource_count = static_cast<uint32_t>(m_frame_resources_list.size()),
    .m_resource_index = 0,
    .m_swapchain_image_index = {}
  };

  OnWindowSizeChanged_Post();
}

#ifdef CWDEBUG
vulkan::AmbifixOwner VulkanWindow::debug_name_prefix(std::string prefix) const
{
  return { this, std::move(prefix) };
}
#endif

} // namespace task
