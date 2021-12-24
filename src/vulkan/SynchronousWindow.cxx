#include "sys.h"
#include "SynchronousWindow.h"
#include "OperatingSystem.h"
#include "LogicalDevice.h"
#include "Application.h"
#include "FrameResourcesData.h"
#include "VertexData.h"
#include "StagingBufferParameters.h"
#include "Exceptions.h"
#include "vk_utils/get_image_data.h"
#include "vk_utils/print_flags.h"
#include "xcb-task/ConnectionBrokerKey.h"
#include "debug/DebugSetName.h"
#ifdef CWDEBUG
#include "utils/debug_ostream_operators.h"
#include "utils/at_scope_end.h"
#endif

namespace task {

//static
vulkan::ImageKind const SynchronousWindow::s_depth_image_kind({
  .format = s_default_depth_format,
  .usage = vk::ImageUsageFlagBits::eDepthStencilAttachment,
  .initial_layout = vk::ImageLayout::eUndefined
});

//static
vulkan::ImageViewKind const SynchronousWindow::s_depth_image_view_kind(s_depth_image_kind, {});

SynchronousWindow::SynchronousWindow(vulkan::Application* application COMMA_CWDEBUG_ONLY(bool debug)) :
  AIStatefulTask(CWDEBUG_ONLY(debug)), SynchronousEngine("SynchronousEngine", 10.0f),
  m_application(application), m_frame_rate_limiter([this](){ signal(frame_timer); })
  COMMA_CWDEBUG_ONLY(mVWDebug(mSMDebug))
{
  DoutEntering(dc::statefultask(mSMDebug), "task::SynchronousWindow::SynchronousWindow(" << application << ") [" << (void*)this << "]");
}

SynchronousWindow::~SynchronousWindow()
{
  DoutEntering(dc::statefultask(mSMDebug), "task::SynchronousWindow::~SynchronousWindow() [" << (void*)this << "]");
  m_frame_rate_limiter.stop();
}

vulkan::LogicalDevice* SynchronousWindow::get_logical_device() const
{
  DoutEntering(dc::vulkan, "SynchronousWindow::get_logical_device() [" << this << "]");
  // This is a reasonable expensive call: it locks a mutex to access a vector.
  // Therefore SynchronousWindow stores a copy that can be used from the render loop.
  return m_application->get_logical_device(get_logical_device_index());
}

char const* SynchronousWindow::state_str_impl(state_type run_state) const
{
  switch (run_state)
  {
    AI_CASE_RETURN(SynchronousWindow_xcb_connection);
    AI_CASE_RETURN(SynchronousWindow_create);
    AI_CASE_RETURN(SynchronousWindow_logical_device_index_available);
    AI_CASE_RETURN(SynchronousWindow_acquire_queues);
    AI_CASE_RETURN(SynchronousWindow_prepare_swapchain);
    AI_CASE_RETURN(SynchronousWindow_create_render_objects);
    AI_CASE_RETURN(SynchronousWindow_render_loop);
    AI_CASE_RETURN(SynchronousWindow_close);
  }
  ASSERT(false);
  return "UNKNOWN STATE";
}

void SynchronousWindow::initialize_impl()
{
  DoutEntering(dc::vulkan, "SynchronousWindow::initialize_impl() [" << this << "]");

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
  set_state(SynchronousWindow_xcb_connection);
}

#ifdef CWDEBUG
struct RenderLoopEntered
{
  debug::Mark m_mark;
  RenderLoopEntered() : m_mark("renderloop ")
  {
    Dout(dc::always, "ENTERING RENDERLOOP");
  }
  ~RenderLoopEntered()
  {
    Dout(dc::always, "LEAVING RENDERLOOP");
  }
};
#endif

void SynchronousWindow::multiplex_impl(state_type run_state)
{
  switch (run_state)
  {
    case SynchronousWindow_xcb_connection:
      // Get the- or create a task::XcbConnection object that is associated with m_broker_key (ie DISPLAY).
      m_xcb_connection_task = m_broker->run(*m_broker_key, [this](bool success){ Dout(dc::notice, "xcb_connection finished!"); signal(connection_set_up); });
      // Wait until the connection with the X server is established, then continue with SynchronousWindow_create.
      set_state(SynchronousWindow_create);
      wait(connection_set_up);
      break;
    case SynchronousWindow_create:
      // Create a new xcb window using the established connection.
      m_window_events->set_xcb_connection(m_xcb_connection_task->connection());
      // We can't set a debug name for the surface yet, because there might not be logical device yet.
      m_presentation_surface = m_window_events->create(m_application->vh_instance(), m_title, get_extent());
      // Trigger the "window created" event.
      m_window_created_event.trigger();
      // If a logical device was passed then we need to copy its index as soon as that becomes available.
      if (m_logical_device_task)
      {
        // Wait until the logical device index becomes available on m_logical_device.
        m_logical_device_task->m_logical_device_index_available_event.register_task(this, logical_device_index_available);
        set_state(SynchronousWindow_logical_device_index_available);
        wait(logical_device_index_available);
        break;
      }
      // Otherwise we can continue with acquiring the swapchain queues.
      set_state(SynchronousWindow_acquire_queues);
      // But wait until the logical_device_index is set, if it wasn't already.
      if (m_logical_device_index.load(std::memory_order::relaxed) == -1)
        wait(logical_device_index_available);
      break;
    case SynchronousWindow_logical_device_index_available:
      // The logical device index is available. Copy it.
      set_logical_device_index(m_logical_device_task->get_index());
      // Create the swapchain.
      set_state(SynchronousWindow_acquire_queues);
      [[fallthrough]];
    case SynchronousWindow_acquire_queues:
      // At this point the logical_device_index is available, which means we can call get_logical_device().
      // Cache the pointer to the vulkan::LogicalDevice.
      m_logical_device = get_logical_device();
      // From this moment on we can use the accessor logical_device().
      // Delayed from SynchronousWindow_create; set the debug name of the surface.
      DebugSetName(m_presentation_surface.vh_surface(), debug_name_prefix("m_presentation_surface.m_surface"));
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
      set_state(SynchronousWindow_prepare_swapchain);
      [[fallthrough]];
    case SynchronousWindow_prepare_swapchain:
      prepare_swapchain();
      set_state(SynchronousWindow_create_render_objects);
      [[fallthrough]];
    case SynchronousWindow_create_render_objects:
      create_frame_resources();
      create_render_passes();
      create_swapchain_framebuffer();
      create_descriptor_set();
      create_textures();
      create_pipeline_layout();
      create_graphics_pipeline();
      create_vertex_buffers();
      set_state(SynchronousWindow_render_loop);
      can_render_again();
      // Turn off debug output for this statefultask while processing the render loop.
      Debug(mSMDebug = false);
      [[fallthrough]];
    case SynchronousWindow_render_loop:
    {
#if CW_DEBUG
      RenderLoopEntered scoped;
#endif
      int special_circumstances = atomic_flags();
      for (;;)  // So that we can use continue.
      {
        if (AI_LIKELY(!special_circumstances))
        {
          try
          {
            // Render the next frame.
            m_frame_rate_limiter.start(m_frame_rate_interval);
            draw_frame();
            m_delay_by_completed_draw_frames.step({});
            yield(m_application->m_medium_priority_queue);
            wait(frame_timer);
            return;
          }
          catch (vulkan::OutOfDateKHR_Exception const& error)
          {
            Dout(dc::warning, "Rendering aborted due to: " << error.what());
            if (!m_frame_rate_limiter.stop())
            {
              // We could not stop the timer from firing. Perhaps because it already
              // fired, or because it is already calling expire(). Wait until it
              // returned from expire().
              m_frame_rate_limiter.wait_for_possible_expire_to_finish();
              // Now we are sure that signal(frame_timer) was already called.
              // To nullify that, call wait(frame_timer) here.
              wait(frame_timer);
            }
            special_circumstances = atomic_flags();
            // No other thread can be running this render loop. A flag must be set before throwing.
            ASSERT(special_circumstances);
          }
        }
        // Handle special circumstances.
        bool need_draw_frame = false;
        if (must_close(special_circumstances))
          break;
        if (have_synchronous_task(special_circumstances))
        {
          handle_synchronous_tasks(CWDEBUG_ONLY(mSMDebug));
          special_circumstances &= ~have_synchronous_task_bit;
          need_draw_frame = true;
        }
        if (!can_render(special_circumstances))
        {
          // We can't render, drop frame rate to 7.8 FPS (because slow_down already uses 128 ms anyway).
          static threadpool::Timer::Interval s_no_render_frame_rate_interval{threadpool::Interval<128, std::chrono::milliseconds>()};
          m_frame_rate_limiter.start(s_no_render_frame_rate_interval);
          yield(m_application->m_low_priority_queue);
          wait(frame_timer);
          need_draw_frame = false;
        }
        else if (extent_changed(special_circumstances))
        {
          handle_window_size_changed();
          special_circumstances = atomic_flags();
          if (need_draw_frame)                                          // Did we already call handle_synchronous_tasks()?
            special_circumstances &= ~have_synchronous_task_bit;        // Don't do that again this frame.
          // Don't call handle_window_size_changed() again either this frame.
          special_circumstances &= ~extent_changed_bit;
          need_draw_frame = true;
        }
        if (!need_draw_frame)
          return;
      }
      set_state(SynchronousWindow_close);
      [[fallthrough]];
    }
    case SynchronousWindow_close:
      // Turn on debug output again.
      Debug(mSMDebug = mVWDebug);
      wait_for_all_fences();
      finish();
      break;
  }
}

void SynchronousWindow::acquire_queues()
{
  DoutEntering(dc::vulkan, "SynchronousWindow::acquire_queues()");
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

  m_presentation_surface.set_queues(vh_graphics_queue, vh_presentation_queue
      COMMA_CWDEBUG_ONLY(debug_name_prefix("m_presentation_surface")));
}

void SynchronousWindow::prepare_swapchain()
{
  DoutEntering(dc::vulkan, "SynchronousWindow::prepare_swapchain()");
  m_swapchain.prepare(this, vk::ImageUsageFlagBits::eColorAttachment, vk::PresentModeKHR::eFifo
      COMMA_CWDEBUG_ONLY(debug_name_prefix("m_swapchain")));
}

void SynchronousWindow::create_swapchain_framebuffer()
{
  m_swapchain.recreate_swapchain_framebuffer(this
      COMMA_CWDEBUG_ONLY(debug_name_prefix("m_swapchain")));
}

void SynchronousWindow::wait_for_all_fences() const
{
  std::vector<vk::Fence> all_fences;
  for (auto&& resources : m_frame_resources_list)
    all_fences.push_back(*resources->m_command_buffers_completed);

  vk::Result res = m_logical_device->wait_for_fences(all_fences, VK_TRUE, 1000000000);
  if (res != vk::Result::eSuccess)
    THROW_FALERTC(res, "wait_for_fences");
}

void SynchronousWindow::handle_window_size_changed()
{
  DoutEntering(dc::vulkan, "SynchronousWindow::handle_window_size_changed()");

  // No reason to call wait_idle: handle_window_size_changed is called from the render loop.
  // Besides, we can't call wait_idle because another window can still be using queues on the logical device.
  on_window_size_changed_pre();
  // We must wait here until all fences are signalled.
  wait_for_all_fences();
  // Now it is safe to recreate the swapchain.
  m_swapchain.recreate(this, get_extent()
      COMMA_CWDEBUG_ONLY(debug_name_prefix("m_swapchain")));
  if (can_render(atomic_flags()))
    on_window_size_changed_post();
}

void SynchronousWindow::set_image_memory_barrier(
    vulkan::ResourceState const& source,
    vulkan::ResourceState const& destination,
    vk::Image vh_image,
    vk::ImageSubresourceRange const& image_subresource_range) const
{
  DoutEntering(dc::vulkan, "SynchronousWindow::set_image_memory_barrier(" << source << ", " << destination << ", " << vh_image << ", " << image_subresource_range << ")");

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

  vk::ImageMemoryBarrier const image_memory_barrier{
    .srcAccessMask = source.access_mask,
    .dstAccessMask = destination.access_mask,
    .oldLayout = source.layout,
    .newLayout = destination.layout,
    .srcQueueFamilyIndex = source.queue_family_index,
    .dstQueueFamilyIndex = destination.queue_family_index,
    .image = vh_image,
    .subresourceRange = image_subresource_range
  };

  // Record command buffer which copies data from the staging buffer to the destination buffer.
  {
    tmp_command_buffer_w->begin({ .flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit });
    tmp_command_buffer_w->pipelineBarrier(source.pipeline_stage_mask, destination.pipeline_stage_mask, {}, {}, {}, { image_memory_barrier });
    tmp_command_buffer_w->end();
  }

  // Submit
  {
    auto fence = logical_device().create_fence(false
        COMMA_CWDEBUG_ONLY(mSMDebug, debug_name_prefix("set_image_memory_barrier()::fence")));

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

    int count = 10;
    vk::Result res;
    do
    {
      res = logical_device().wait_for_fences({ *fence }, VK_TRUE, 300000000);
      if (res != vk::Result::eTimeout)
        break;
      Dout(dc::notice, "wait_for_fences timed out " << count);
    }
    while (--count > 0);
    if (res != vk::Result::eSuccess)
    {
      Dout(dc::warning, "wait_for_fences returned " << res);
      logical_device().reset_fences({ *fence });
      THROW_ALERTC(res, "waitForFences");
    }
  }
}

void SynchronousWindow::create_descriptor_set()
{
  DoutEntering(dc::vulkan, "SynchronousWindow::create_descriptor_set() [" << this << "]");

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

void SynchronousWindow::create_textures()
{
  DoutEntering(dc::vulkan, "SynchronousWindow::create_textures() [" << this << "]");

  // Background texture.
  {
    int width = 0, height = 0, data_size = 0;
    std::vector<char> texture_data = vk_utils::get_image_data(m_application->resources_path() / "textures/background.png", 4, &width, &height, nullptr, &data_size);
    // Create descriptor resources.
    {
      static vulkan::ImageKind const background_image_kind({
        .format = vk::Format::eR8G8B8A8Unorm,
        .usage = vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled
      });

      static vulkan::ImageViewKind const background_image_view_kind(background_image_kind, {});

      m_background_texture =
        m_logical_device->create_image(width, height, background_image_kind, background_image_view_kind, vk::MemoryPropertyFlagBits::eDeviceLocal
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
      vk_defaults::ImageSubresourceRange const image_subresource_range;
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
      static vulkan::ImageKind const sample_image_kind({
        .format = vk::Format::eR8G8B8A8Unorm,
        .usage = vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled
      });

      static vulkan::ImageViewKind const sample_image_view_kind(sample_image_kind, {});

      m_texture = m_logical_device->create_image(width, height, sample_image_kind, sample_image_view_kind, vk::MemoryPropertyFlagBits::eDeviceLocal
          COMMA_CWDEBUG_ONLY(debug_name_prefix("m_texture")));
      m_texture.m_sampler = m_logical_device->create_sampler(vk::SamplerMipmapMode::eNearest, vk::SamplerAddressMode::eClampToEdge, VK_FALSE
          COMMA_CWDEBUG_ONLY(debug_name_prefix("m_texture.m_sampler")));
    }
    // Copy data.
    {
      vk_defaults::ImageSubresourceRange const image_subresource_range;
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

void SynchronousWindow::create_pipeline_layout()
{
  DoutEntering(dc::vulkan, "SynchronousWindow::create_pipeline_layout() [" << this << "]");

  vk::PushConstantRange push_constant_ranges{
    .stageFlags = vk::ShaderStageFlagBits::eVertex,
    .offset = 0,
    .size = 4
  };
  m_pipeline_layout = m_logical_device->create_pipeline_layout({ *m_descriptor_set.m_layout }, { push_constant_ranges }
      COMMA_CWDEBUG_ONLY(debug_name_prefix("m_pipeline_layout")));
}

void SynchronousWindow::copy_data_to_buffer(uint32_t data_size, void const* data, vk::Buffer target_buffer,
    vk::DeviceSize buffer_offset, vk::AccessFlags current_buffer_access, vk::PipelineStageFlags generating_stages,
    vk::AccessFlags new_buffer_access, vk::PipelineStageFlags consuming_stages) const
{
  DoutEntering(dc::vulkan, "SynchronousWindow::copy_data_to_buffer(" << data_size << ", " << data << ", " << target_buffer << ", " <<
    buffer_offset << ", " << current_buffer_access << ", " << generating_stages << ", " <<
    new_buffer_access << ", " << consuming_stages << ") [" << this << "]");

  // Create staging buffer and map its memory to copy data from the CPU.
  vulkan::StagingBufferParameters staging_buffer;
  {
    staging_buffer.m_buffer = m_logical_device->create_buffer(data_size, vk::BufferUsageFlagBits::eTransferSrc, vk::MemoryPropertyFlagBits::eHostVisible
        COMMA_CWDEBUG_ONLY(debug_name_prefix("copy_data_to_buffer()::staging_buffer.m_buffer")));
    staging_buffer.m_pointer = m_logical_device->map_memory(*staging_buffer.m_buffer.m_memory, 0, data_size);

    std::memcpy(staging_buffer.m_pointer, data, data_size);

    vk::MappedMemoryRange memory_range{
      .memory = *staging_buffer.m_buffer.m_memory,
      .offset = 0,
      .size = VK_WHOLE_SIZE
    };
    m_logical_device->flush_mapped_memory_ranges({ memory_range });

    m_logical_device->unmap_memory(*staging_buffer.m_buffer.m_memory);
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
    vk::UniqueFence fence = m_logical_device->create_fence(false
        COMMA_CWDEBUG_ONLY(mSMDebug, debug_name_prefix("copy_data_to_buffer()::fence")));

    vk::SubmitInfo submit_info{
      .waitSemaphoreCount = 0,
      .pWaitSemaphores = nullptr,
      .pWaitDstStageMask = nullptr,
      .commandBufferCount = 1,
      .pCommandBuffers = tmp_command_buffer_w
    };
    m_presentation_surface.vh_graphics_queue().submit( { submit_info }, *fence );

    vk::Result res = logical_device().wait_for_fences({ *fence }, VK_FALSE, 1000000000);
    if (res != vk::Result::eSuccess)
      THROW_ALERTC(res, "waitForFences");
  }
}

void SynchronousWindow::copy_data_to_image(uint32_t data_size, void const* data, vk::Image target_image,
    uint32_t width, uint32_t height, vk::ImageSubresourceRange const& image_subresource_range,
    vk::ImageLayout current_image_layout, vk::AccessFlags current_image_access, vk::PipelineStageFlags generating_stages,
    vk::ImageLayout new_image_layout, vk::AccessFlags new_image_access, vk::PipelineStageFlags consuming_stages) const
{
  DoutEntering(dc::vulkan, "SynchronousWindow::copy_data_to_image(" << data_size << ", " << data << ", " << target_image << ", " << width << ", " << height << ", " <<
      image_subresource_range << ", " << current_image_layout << ", " << current_image_access << ", " << generating_stages << ", " <<
      new_image_layout << ", " << new_image_access << ", " << consuming_stages << ")");

  // Create staging buffer and map it's memory to copy data from the CPU.
  vulkan::StagingBufferParameters staging_buffer;
  {
    staging_buffer.m_buffer = m_logical_device->create_buffer(data_size, vk::BufferUsageFlagBits::eTransferSrc, vk::MemoryPropertyFlagBits::eHostVisible
        COMMA_CWDEBUG_ONLY(debug_name_prefix("copy_data_to_image()::staging_buffer.m_buffer")));
    staging_buffer.m_pointer = m_logical_device->map_memory(*staging_buffer.m_buffer.m_memory, 0, data_size);

    std::memcpy(staging_buffer.m_pointer, data, data_size);

    vk::MappedMemoryRange memory_range{
        .memory = *staging_buffer.m_buffer.m_memory,
        .offset = 0,
        .size = VK_WHOLE_SIZE
    };
    m_logical_device->flush_mapped_memory_ranges({ memory_range });

    m_logical_device->unmap_memory(*staging_buffer.m_buffer.m_memory);
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
    vk::UniqueFence fence = m_logical_device->create_fence(false
        COMMA_CWDEBUG_ONLY(mSMDebug, debug_name_prefix("copy_data_to_image()::fence")));

    vk::SubmitInfo submit_info{
      .waitSemaphoreCount = 0,
      .pWaitSemaphores = nullptr,
      .pWaitDstStageMask = nullptr,
      .commandBufferCount = 1,
      .pCommandBuffers = tmp_command_buffer_w
    };
    m_presentation_surface.vh_graphics_queue().submit({ submit_info }, *fence);

    vk::Result res = logical_device().wait_for_fences({ *fence }, VK_FALSE, 1000000000);
    if (res != vk::Result::eSuccess)
      THROW_ALERTC(res, "waitForFences");
  }
}

void SynchronousWindow::start_frame()
{
  DoutEntering(dc::vkframe, "SynchronousWindow::start_frame(...)");
#if 0
  Timer.Update();
  Gui.StartFrame( Timer, MouseState );
  PrepareGUIFrame();
#endif
  m_current_frame.m_resource_index = (m_current_frame.m_resource_index + 1) % m_current_frame.m_resource_count;
  m_current_frame.m_frame_resources = m_frame_resources_list[m_current_frame.m_resource_index].get();

  if (m_logical_device->wait_for_fences({ *m_current_frame.m_frame_resources->m_command_buffers_completed }, VK_FALSE, 1000000000) != vk::Result::eSuccess)
    throw std::runtime_error("Waiting for a fence takes too long!");
}

void SynchronousWindow::finish_frame(/* vulkan::handle::CommandBuffer command_buffer,*/ vk::RenderPass render_pass)
{
  DoutEntering(dc::vkframe, "SynchronousWindow::finish_frame(...)");

#if 0
  // Draw GUI
  {
    Gui.Draw(m_current_frame.ResourceIndex, command_buffer, render_pass, *m_current_frame.FrameResources->Framebuffer);

    vk::PipelineStageFlags wait_dst_stage_mask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
    vk::SubmitInfo submit_info{
      .waitSemaphoreCount = 0,
      .pWaitSemaphores = nullptr,
      .pWaitDstStageMask = &wait_dst_stage_mask,
      .commandBufferCount = 1,
      .pCommandBuffers = &command_buffer,
      .signalSemaphoreCount = 1,
      .pSignalSemaphores = &(*m_current_frame.m_frame_resources->m_finished_rendering_semaphore)
    };
    m_presentation_surface.vh_presentation_queue().submit({ submit_info }, VK_NULL_HANDLE);
  }
#endif

  // Present frame
  {
    vk::Result result = vk::Result::eSuccess;
    vk::SwapchainKHR vh_swapchain = *m_swapchain;
    uint32_t swapchain_image_index = m_swapchain.current_index().get_value();
    vk::PresentInfoKHR present_info{
      .waitSemaphoreCount = 1,
      .pWaitSemaphores = m_swapchain.vhp_current_rendering_finished_semaphore(),
      .swapchainCount = 1,
      .pSwapchains = &vh_swapchain,
      .pImageIndices = &swapchain_image_index
    };

    vk::Result res = m_presentation_surface.vh_presentation_queue().presentKHR(&present_info);
    switch (res)
    {
      case vk::Result::eSuccess:
        break;
      case vk::Result::eSuboptimalKHR:
        Dout(dc::warning, "presentKHR() returned eSuboptimalKHR!");
        break;
      case vk::Result::eErrorOutOfDateKHR:
        // Force regeneration of the swapchain.
        set_extent_changed();
        throw vulkan::OutOfDateKHR_Exception();
      default:
        THROW_ALERTC(res, "Could not acquire swapchain image!");
    }
  }
}

vk::UniqueFramebuffer SynchronousWindow::create_imageless_swapchain_framebuffer(CWDEBUG_ONLY(vulkan::AmbifixOwner const& debug_name)) const
{
  DoutEntering(dc::vulkan, "SynchronousWindow::create_imageless_swapchain_framebuffer()" << " [" << (is_slow() ? "SlowWindow" : "Window") << "]");

  vk::Extent2D const& extent = m_swapchain.extent();
  vulkan::rendergraph::RenderPass const* swapchain_render_pass_output_sink = m_swapchain.render_pass_output_sink();
  utils::Vector<vk::FramebufferAttachmentImageInfo, vulkan::rendergraph::AttachmentIndex> framebuffer_attachment_image_infos =
    swapchain_render_pass_output_sink->get_framebuffer_attachment_image_infos(extent);

  vk::StructureChain<vk::FramebufferCreateInfo, vk::FramebufferAttachmentsCreateInfo> framebuffer_create_info_chain(
    {
      .flags = vk::FramebufferCreateFlagBits::eImageless,
      .renderPass = m_swapchain.vh_render_pass(),
      .attachmentCount = static_cast<uint32_t>(framebuffer_attachment_image_infos.size()),
      .width = extent.width,
      .height = extent.height,
      .layers = m_swapchain.image_kind()->array_layers
    },
    {
      .attachmentImageInfoCount = static_cast<uint32_t>(framebuffer_attachment_image_infos.size()),
      .pAttachmentImageInfos = framebuffer_attachment_image_infos.data()
    }
  );

  vk::UniqueFramebuffer framebuffer = m_logical_device->create_framebuffer(framebuffer_create_info_chain.get<vk::FramebufferCreateInfo>()
      COMMA_CWDEBUG_ONLY(debug_name));

  return framebuffer;
}

void SynchronousWindow::acquire_image()
{
  DoutEntering(dc::vkframe, "SynchronousWindow::acquire_image() [" << this << "]");

  // Acquire swapchain image.
  vulkan::SwapchainIndex new_swapchain_index;
  vk::Result res = m_logical_device->acquire_next_image(
      *m_swapchain,
      1000000000,
      m_swapchain.vh_acquire_semaphore(),
      vk::Fence(),
      new_swapchain_index);
  switch (res)
  {
    case vk::Result::eSuccess:
      break;
    case vk::Result::eSuboptimalKHR:
      Dout(dc::warning, "acquire_next_image() returned eSuboptimalKHR!");
      break;
    case vk::Result::eErrorOutOfDateKHR:
      // Force regeneration of the swapchain.
      set_extent_changed();
      throw vulkan::OutOfDateKHR_Exception();
    default:
      THROW_ALERTC(res, "Could not acquire swapchain image!");
  }

  m_swapchain.update_current_index(new_swapchain_index);
}

void SynchronousWindow::finish_impl()
{
  DoutEntering(dc::vulkan, "SynchronousWindow::finish_impl() [" << this << "]");
  // Not doing anything here, finishing the task causes us to leave the run()
  // function and then leave the scope of the boost::intrusive_ptr that keeps
  // this task alive. When it gets destructed, also Window will be destructed
  // and it's destructor does the work required to close the window.
  //
  // However, remove us from Application::window_list_t, or else this SynchronousWindow
  // won't be destructed as the list stores boost::intrusive_ptr<task::SynchronousWindow>'s.
  m_application->remove(this);
}

//virtual
threadpool::Timer::Interval SynchronousWindow::get_frame_rate_interval() const
{
  return threadpool::Interval<10, std::chrono::milliseconds>{};
}

void SynchronousWindow::on_window_size_changed_pre()
{
  DoutEntering(dc::vulkan, "SynchronousWindow::on_window_size_changed_pre()");
}

void SynchronousWindow::on_window_size_changed_post()
{
  DoutEntering(dc::vulkan, "SynchronousWindow::on_window_size_changed_post()");

  // Should already be set.
  ASSERT(swapchain().extent().width > 0);

//FIXME, add m_gui    m_gui.on_window_size_changed();

  // For all depth attachments.

  vk::ImageSubresourceRange const depth_subresource_range =
    vk::ImageSubresourceRange{vulkan::Swapchain::s_default_subresource_range}
      .setAspectMask(vk::ImageAspectFlagBits::eDepth);

  vulkan::ResourceState const new_image_resource_state;
  vulkan::ResourceState const initial_depth_resource_state = {
    .pipeline_stage_mask        = vk::PipelineStageFlagBits::eEarlyFragmentTests,
    .access_mask                = vk::AccessFlagBits::eDepthStencilAttachmentWrite,
    .layout                     = vk::ImageLayout::eDepthStencilAttachmentOptimal
  };

#ifdef CWDEBUG
  int frame_resource_index = 0;
#endif
  for (std::unique_ptr<vulkan::FrameResourcesData> const& frame_resources_data : m_frame_resources_list)
  {
    // Create depth attachment.
    frame_resources_data->m_depth_attachment = m_logical_device->create_image(
        swapchain().extent().width,
        swapchain().extent().height,
        s_depth_image_kind,
        s_depth_image_view_kind,
        vk::MemoryPropertyFlagBits::eDeviceLocal
        COMMA_CWDEBUG_ONLY(debug_name_prefix("m_frame_resources_list[" + std::to_string(frame_resource_index++) + "]->m_depth_attachment")));

    // Transition it away from an undefined layout.
    set_image_memory_barrier(
        new_image_resource_state,
        initial_depth_resource_state,
        *frame_resources_data->m_depth_attachment.m_image,
        depth_subresource_range);
  }

  vulkan::ResourceState const initial_present_resource_state = {
    .pipeline_stage_mask        = vk::PipelineStageFlagBits::eBottomOfPipe,
    .access_mask                = vk::AccessFlagBits::eMemoryRead,
    .layout                     = vk::ImageLayout::ePresentSrcKHR
  };

  // Pre-transition all swapchain images away from an undefined layout.
  swapchain().set_image_memory_barriers(
      this,
      new_image_resource_state,
      initial_present_resource_state);
}

//virtual
// Override this function to change this value.
size_t SynchronousWindow::number_of_frame_resources() const
{
  return s_default_number_of_frame_resources;
}

void SynchronousWindow::create_frame_resources()
{
  DoutEntering(dc::vulkan, "SynchronousWindow::create_frame_resources() [" << this << "]");

  m_frame_resources_list.resize(number_of_frame_resources());
  Dout(dc::vulkan, "Creating " << m_frame_resources_list.size() << " frame resources.");
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

    frame_resources->m_command_buffers_completed = m_logical_device->create_fence(true COMMA_CWDEBUG_ONLY(true, ambifix("->m_command_buffers_completed")));

    {
      // Lock command pool.
      vulkan::FrameResourcesData::command_pool_type::wat command_pool_w(frame_resources->m_command_pool);

      frame_resources->m_pre_command_buffer = command_pool_w->allocate_buffer(
          CWDEBUG_ONLY(ambifix("->m_pre_command_buffer"))
          );
#if 0
      frame_resources->m_post_command_buffer = command_pool_w->allocate_buffer(
          CWDEBUG_ONLY(ambifix("->m_post_command_buffer"))
          );
#endif
    } // Unlock command pool.
  }

  // Initialize m_current_frame to point to frame resources index 0.
  m_current_frame = vulkan::CurrentFrameData{
    .m_frame_resources = m_frame_resources_list[0].get(),
    .m_resource_count = static_cast<uint32_t>(m_frame_resources_list.size()),
    .m_resource_index = 0
  };

  on_window_size_changed_post();
}

#ifdef CWDEBUG
vulkan::AmbifixOwner SynchronousWindow::debug_name_prefix(std::string prefix) const
{
  return { this, std::move(prefix) };
}
#endif

} // namespace task
