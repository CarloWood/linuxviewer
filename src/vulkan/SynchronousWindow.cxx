#include "sys.h"
#include "SynchronousWindow.h"
#include "OperatingSystem.h"
#include "LogicalDevice.h"
#include "Application.h"
#include "FrameResourcesData.h"
#include "Exceptions.h"
#include "SynchronousTask.h"
#include "pipeline/Handle.h"
#include "pipeline/PipelineCache.h"
#include "queues/CopyDataToImage.h"
#include "vk_utils/print_flags.h"
#include "vk_utils/UniformColorDataFeeder.h"
#include "xcb-task/ConnectionBrokerKey.h"
#include "utils/cpu_relax.h"
#include "utils/u8string_to_filename.h"
#include "utils/malloc_size.h"
#ifdef CWDEBUG
#include "debug/DebugSetName.h"
#include "debug/vulkan_print_on.h"
#include "utils/debug_ostream_operators.h"
#include "utils/at_scope_end.h"
#endif
#ifdef TRACY_ENABLE
#include "utils/at_scope_end.h"
#endif
#include "tracy/CwTracy.h"
#include <vulkan/utility/vk_format_utils.h>
#include <algorithm>
#include "debug.h"

#if defined(CWDEBUG) && !defined(DOXYGEN)
NAMESPACE_DEBUG_CHANNELS_START
channel_ct renderloop("RENDERLOOP");
NAMESPACE_DEBUG_CHANNELS_END
#endif

namespace vulkan::task {

//static
vulkan::ImageKind const SynchronousWindow::s_depth_image_kind({
  .format = s_default_depth_format,
  .usage = vk::ImageUsageFlagBits::eDepthStencilAttachment,
  .initial_layout = vk::ImageLayout::eUndefined
});

//static
vulkan::ImageViewKind const SynchronousWindow::s_depth_image_view_kind(s_depth_image_kind, {});

//static
vulkan::ImageKind const SynchronousWindow::s_color_image_kind({});      // The default is exactly what we want, as default.

//static
vulkan::ImageViewKind const SynchronousWindow::s_color_image_view_kind(s_color_image_kind, {});

SynchronousWindow::SynchronousWindow(vulkan::Application* application COMMA_CWDEBUG_ONLY(bool debug)) :
  AIStatefulTask(CWDEBUG_ONLY(debug)), SynchronousEngine("SynchronousEngine", 10.0f),
  m_application(application), m_frame_rate_limiter([this](){ signal(frame_timer); }),
  m_semaphore_watcher(statefultask::create<SemaphoreWatcher<SynchronousTask>>(this
        COMMA_CWDEBUG_ONLY(mSMDebug && Application::instance().debug_SemaphoreWatcher())))
  COMMA_TRACY_ONLY(tracy_acquired_image_tracy_context(8), tracy_acquired_image_busy(8)),
  attachment_index_context(vulkan::rendergraph::AttachmentIndex{0}), m_dependent_tasks(utils::max_malloc_size(4096))
  COMMA_CWDEBUG_ONLY(mVWDebug(mSMDebug))
{
  DoutEntering(dc::statefultask(mSMDebug), "SynchronousWindow(" << application << ") [" << (void*)this << "]");
  m_semaphore_watcher->run();
}

SynchronousWindow::~SynchronousWindow()
{
  DoutEntering(dc::statefultask(mSMDebug), "~SynchronousWindow() [" << (void*)this << "]");
  m_frame_rate_limiter.stop();
  if (m_parent_window_task)
    m_parent_window_task->remove_child_window_task(this);
}

vulkan::LogicalDevice* SynchronousWindow::get_logical_device() const
{
  DoutEntering(dc::vulkan, "SynchronousWindow::get_logical_device() [" << this << "]");
  // This is a reasonable expensive call: it locks a mutex to access a vector.
  // Therefore SynchronousWindow stores a copy that can be used from the render loop.
  return m_application->get_logical_device(get_logical_device_index());
}

void SynchronousWindow::register_render_pass(SynchronousWindow::RenderPass* render_pass)
{
#ifdef CWDEBUG
  std::string const& name = render_pass->name();
  auto res = std::find_if(m_render_passes.begin(), m_render_passes.end(), [&name](RenderPass* render_pass){ return render_pass->name() == name; });
  // Please use a unique name for each render pass.
  ASSERT(res == m_render_passes.end());
#endif
  m_render_passes.emplace_back(render_pass);
}

void SynchronousWindow::unregister_render_pass(SynchronousWindow::RenderPass* render_pass)
{
  m_render_passes.erase(std::remove(m_render_passes.begin(), m_render_passes.end(), render_pass), m_render_passes.end());
}

void SynchronousWindow::register_attachment(SynchronousWindow::Attachment const* attachment)
{
#if CW_DEBUG
  std::string const& name = attachment->name();
  auto res = std::find_if(m_attachments.begin(), m_attachments.end(), [&name](Attachment const* attachment){ return attachment->name() == name; });
  // Please use a unique name for each attachment.
  ASSERT(res == m_attachments.end());
#endif
  m_attachments.emplace_back(attachment);
  // Paranoia check: if the attachment isn't swapchain, then the attachment index will be used in an array of size m_attachments.size().
  // Since we don't get here for the swapchain attachment.
  ASSERT(!attachment->index().undefined());     // not swapchain
  // This should always hold (in fact, since this function is called immediately after assigning the index,
  // it will be the case that attachment->index().get_value() == m_attachments.size() - 1).
  ASSERT(0 <= attachment->index().get_value() && attachment->index().get_value() < m_attachments.size());
}

char const* SynchronousWindow::condition_str_impl(condition_type condition) const
{
  switch (condition)
  {
    AI_CASE_RETURN(connection_set_up);
    AI_CASE_RETURN(frame_timer);
    AI_CASE_RETURN(logical_device_index_available);
    AI_CASE_RETURN(loading_texture_ready);
    AI_CASE_RETURN(imgui_font_texture_ready);
    AI_CASE_RETURN(parent_window_created);
    AI_CASE_RETURN(condition_pipeline_available);
    AI_CASE_RETURN(free_condition);
  }
  return direct_base_type::condition_str_impl(condition);
}

char const* SynchronousWindow::state_str_impl(state_type run_state) const
{
  switch (run_state)
  {
    AI_CASE_RETURN(SynchronousWindow_xcb_connection);
    AI_CASE_RETURN(SynchronousWindow_create_child);
    AI_CASE_RETURN(SynchronousWindow_create);
    AI_CASE_RETURN(SynchronousWindow_parents_logical_device_index_available);
    AI_CASE_RETURN(SynchronousWindow_logical_device_index_available);
    AI_CASE_RETURN(SynchronousWindow_acquire_queues);
    AI_CASE_RETURN(SynchronousWindow_initialize_vulkan);
    AI_CASE_RETURN(SynchronousWindow_loading_texture_ready);
    AI_CASE_RETURN(SynchronousWindow_imgui_font_texture_ready);
    AI_CASE_RETURN(SynchronousWindow_render_loop);
    AI_CASE_RETURN(SynchronousWindow_close);
  }
  AI_NEVER_REACHED;
}

char const* SynchronousWindow::task_name_impl() const
{
  return "SynchronousWindow";
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

  m_frame_rate_interval = frame_rate_interval();
  set_state(SynchronousWindow_xcb_connection);
}

#ifdef CWDEBUG
struct RenderLoopEntered
{
  NAMESPACE_DEBUG::Mark m_mark;
  RenderLoopEntered(char8_t const* utf8_m) : m_mark(utf8_m)
  {
    Dout(dc::renderloop, "ENTERING RENDERLOOP");
  }
  ~RenderLoopEntered()
  {
    Dout(dc::renderloop, "LEAVING RENDERLOOP");
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
      // Wait until the connection with the X server is established, then continue with SynchronousWindow_create or SynchronousWindow_create_child.
      set_state(!m_parent_window_task ? SynchronousWindow_create : SynchronousWindow_create_child);
      wait(connection_set_up);
      break;
    case SynchronousWindow_create_child:
      // This is a child window. Wait until the parent window was created so that we can obtain its window handle.
      m_parent_window_task->m_window_created_event.register_task(this, parent_window_created);
      set_state(SynchronousWindow_create);
      wait(parent_window_created);
      break;
    case SynchronousWindow_create:
      // Allocate the ring buffer for input events.
      m_input_event_buffer.reallocate_buffer(s_input_event_buffer_size);
      // Register ourselves for input events.
      m_window_events->register_input_event_buffer(&m_input_event_buffer);
      // Create a new xcb window using the established connection.
      m_window_events->set_xcb_connection(m_xcb_connection_task->connection());
      // We can't set a debug name for the surface yet, because there might not be a logical device yet.
      m_presentation_surface = m_window_events->create(m_application->vh_instance(), m_title, { m_offset, get_extent() },
          m_parent_window_task ? m_parent_window_task->window_events() : nullptr);
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
      else if (m_parent_window_task)
      {
        // Apparently we're a child window that was created before the parent window had its logical device index.
        // Wait until the parent has its logical device, so we can just copy that.
        m_parent_window_task->m_logical_device_index_available_event.register_task(this, logical_device_index_available);
        set_state(SynchronousWindow_parents_logical_device_index_available);
        wait(logical_device_index_available);
        break;
      }
      // Otherwise we can continue with acquiring the swapchain queues.
      set_state(SynchronousWindow_acquire_queues);
      // But wait until the logical_device_index is set, if it wasn't already.
      if (m_logical_device_index.load(std::memory_order::relaxed) == -1)
      {
        // Wait until set_logical_device_index is called, which is called from Application::create_device called from
        // the logical device task state LogicalDevice_create for the root window that was passed to Application::create_logical_device.
        wait(logical_device_index_available);
      }
      break;
    case SynchronousWindow_parents_logical_device_index_available:
      // The logical device index is available on the parent. Copy it.
      set_logical_device_index(m_parent_window_task->get_logical_device_index());
      // Create the swapchain.
      set_state(SynchronousWindow_acquire_queues);
      break;
    case SynchronousWindow_logical_device_index_available:
      // The logical device index is available. Copy it.
      set_logical_device_index(m_logical_device_task->get_index());
      // Create the swapchain.
      set_state(SynchronousWindow_acquire_queues);
      Dout(dc::statefultask(mSMDebug), "Falling through to SynchronousWindow_acquire_queues [" << this << "]");
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
      set_state(SynchronousWindow_initialize_vulkan);
      Dout(dc::statefultask(mSMDebug), "Falling through to SynchronousWindow_initialize_vulkan [" << this << "]");
      [[fallthrough]];
    case SynchronousWindow_initialize_vulkan:
      copy_graphics_settings();
      set_default_clear_values(m_render_graph.m_default_color_clear_value, m_render_graph.m_default_depth_stencil_clear_value);
      prepare_swapchain();
      create_render_graph();
      create_swapchain_images();
      create_frame_resources();
      create_imageless_framebuffers();  // Must be called after create_swapchain_images()!
      create_vertex_buffers();
      register_shader_templates();
      // Upload the "loading texture".
      {
        vk::Extent2D const extent{1, 1};
        m_loading_texture = vulkan::Texture(
            m_logical_device, extent, {}, { .maxAnisotropy = 1.f }, { .properties = vk::MemoryPropertyFlagBits::eDeviceLocal }
            COMMA_CWDEBUG_ONLY(debug_name_prefix("m_loading_texture")));
        m_loading_texture.upload(extent, this, std::make_unique<vk_utils::UniformColorDataFeeder>(32, 32, 32), this, loading_texture_ready);
      }
      set_state(SynchronousWindow_loading_texture_ready);
      wait(loading_texture_ready);
      break;
    case SynchronousWindow_loading_texture_ready:
      create_graphics_pipelines();
      create_textures();
      if (m_use_imgui)                  //FIXME: this was set in create_descriptor_set by the user.
      {
        create_imgui();
        set_state(SynchronousWindow_imgui_font_texture_ready);
        wait(imgui_font_texture_ready);
        break;
      }
      Dout(dc::statefultask(mSMDebug), "Falling through to SynchronousWindow_imgui_font_texture_ready [" << this << "]");
      [[fallthrough]];
    case SynchronousWindow_imgui_font_texture_ready:
      set_state(SynchronousWindow_render_loop);
      // We already have a swapchain up there - but only now we can really render anything, so set it here.
      vulkan::SynchronousEngine::have_swapchain();
      // Turn off debug output for this statefultask while processing the render loop.
      Debug(mSMDebug = false);
      Dout(dc::statefultask(mSMDebug), "Falling through to SynchronousWindow_render_loop [" << this << "]");
      [[fallthrough]];
    case SynchronousWindow_render_loop:
    {
      if (m_use_imgui)
      {
        // Set a thread-local global variable that imgui uses to access its context.
        m_imgui.set_current_context();
      }
#ifdef CWDEBUG
      RenderLoopEntered scoped(m_title.c_str());
#endif
      int special_circumstances = atomic_flags();
      for (;;)  // So that we can use continue to try and render again, break to close the window and return when we're done until the next frame.
      {
        if (AI_LIKELY(!special_circumstances))
        {
          try
          {
            ZoneScopedNC("SynchronousWindow_render_loop / no special circumstances", 0xf5d193) // Tracy
            // Render the next frame.
            m_frame_rate_limiter.start(m_frame_rate_interval);
            m_imgui_timer.update();   // Keep track of FPS and stuff.
            consume_input_events();
            render_frame();
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
        if (map_changed(special_circumstances))
        {
          int map_flags = atomic_map_flags();
          Dout(dc::vulkan, "Handling map_changed " << print_map_flags(map_flags));
          bool minimized = !is_mapped(map_flags);
          bool success = handle_map_changed(map_flags);
          if (success)
          {
            special_circumstances &= ~(map_changed_bit|minimized_bit);  // That event was handled (default to unminimized).
            if (minimized)
              special_circumstances |= minimized_bit;                   // Set the minimized flag.
            else
              need_draw_frame = true;
          }
          else
          {
            Dout(dc::notice, "Failed to " << (minimized ? "minimize" : "unminimize") << ".");
            special_circumstances = atomic_flags();
            continue;
          }
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
      Dout(dc::statefultask(mSMDebug), "Falling through to SynchronousWindow_close [" << this << "]");
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

  vulkan::Queue vh_graphics_queue = logical_device()->acquire_queue({QueueFlagBits::eGraphics|QueueFlagBits::ePresentation, m_request_cookie});
  vulkan::Queue vh_presentation_queue;

  if (!vh_graphics_queue)
  {
    // The combination of eGraphics|ePresentation failed. We have to try separately.
    vh_graphics_queue = logical_device()->acquire_queue({QueueFlagBits::eGraphics, m_request_cookie});
    vh_presentation_queue = logical_device()->acquire_queue({QueueFlagBits::ePresentation, m_request_cookie});
  }
  else
    vh_presentation_queue = vh_graphics_queue;

  m_presentation_surface.set_queues(vh_graphics_queue, vh_presentation_queue
#ifdef TRACY_ENABLE
      , this
#endif
      COMMA_CWDEBUG_ONLY(debug_name_prefix("m_presentation_surface")));
}

void SynchronousWindow::prepare_swapchain()
{
  DoutEntering(dc::vulkan, "SynchronousWindow::prepare_swapchain()");
  m_swapchain.prepare(this, vk::ImageUsageFlagBits::eColorAttachment, vk::PresentModeKHR::eFifo
      COMMA_CWDEBUG_ONLY(debug_name_prefix("m_swapchain")));
}

void SynchronousWindow::create_swapchain_images()
{
  m_swapchain.recreate(this, get_extent()
      COMMA_CWDEBUG_ONLY(debug_name_prefix("m_swapchain")));
}

void SynchronousWindow::change_number_of_swapchain_images(uint32_t image_count)
{
  if (m_swapchain.change_image_count({}, this, image_count))
  {
    // Trigger recreation of the swap chain.
    m_window_events->recreate_swapchain({});
  }
}

void SynchronousWindow::create_imageless_framebuffers()
{
  prepare_begin_info_chains();
  vk::Extent2D extent = m_swapchain.extent();
  uint32_t layers = m_swapchain.image_kind()->array_layers;
  recreate_framebuffers(extent, layers);                        // Really just create for the first time.
}

void SynchronousWindow::create_imgui()
{
  DoutEntering(dc::vulkan, "SynchronousWindow::create_imgui()");
  // ImGui uses `layout(location = 0) out vec4 fColor;` in its imgui_frag_glsl.
  //
  // And it does draw calls while inside a beginRenderPass with vk::SubpassContents::eInline.
  // Therefore location = 0 corresponds to the image that corresponds to attachment #0 in the .pColorAttachments array of
  // subpass description #0 in the .pSubpasses array of the vk::RenderPassCreateInfo used to create the render pass of imgui_pass.
#if CW_DEBUG
  auto const& subpass_descriptions = imgui_pass.subpass_descriptions();
  ASSERT(subpass_descriptions.size() == 1);
  auto const only_subpass = subpass_descriptions.ibegin();              // subpass description #0
  auto const& subpass_description = subpass_descriptions[only_subpass];
#endif
  //
  // Where .pColorAttachments, is set to be a list of all "known" color attachments used in the render graph (see vulkan::rendergraph::RenderPass::create).
  // If `colorAttachmentCount` isn't 1 then there were more than one "known" color attachments;
  // which is nonsense and causes `location = 0` to refer to a "random" attachment (the order that known color attachments are added is not defined).
  //
  // For example, when you'd do:  imgui_pass[~color1].store(color2).
  //
  // Don't do that. imgui_pass is only allowed to see one color attachment (an no depth attachments).
  ASSERT(subpass_description.colorAttachmentCount == 1);
  // Make sure no depth attachment was passed because that isn't used.
  ASSERT(subpass_description.pDepthStencilAttachment == nullptr ||
         subpass_description.pDepthStencilAttachment->attachment == VK_ATTACHMENT_UNUSED);

  auto const& attachment_descriptions = imgui_pass.attachment_descriptions();
  // If the above two asserts hold, then this one should also hold (because all we have are color and depth attachments?)
  ASSERT(attachment_descriptions.size() == 1);  // There is only one known attachment.
  auto const only_attachment = attachment_descriptions.ibegin();        // attachment description #0
  auto const& attachment_description = attachment_descriptions[only_attachment];

  m_imgui.init(this, attachment_description.samples, imgui_font_texture_ready, graphics_settings()
      COMMA_CWDEBUG_ONLY(debug_name_prefix("m_imgui")));
}

void SynchronousWindow::consume_input_events()
{
  DoutEntering(dc::vkframe, "SynchronousWindow::consume_input_events() [" << this << "]");
  // We are the consumer thread.
  Dout(dc::vkframe|continued_cf, "Calling m_input_event_buffer.pop() = ");
  while (vulkan::InputEvent const* input_event = m_input_event_buffer.pop())
  {
    Dout(dc::finish, '{' << *input_event << '}');
    int16_t x = input_event->mouse_position.x();
    int16_t y = input_event->mouse_position.y();
    bool active = static_cast<uint16_t>(input_event->flags.event_type()) & 1;
    using vulkan::EventType;
    switch (input_event->flags.event_type())
    {
      case EventType::key_release:
      case EventType::key_press:
        if (m_use_imgui)
        {
          m_imgui.on_mouse_move(x, y);
          m_imgui.update_modifiers(input_event->modifier_mask.imgui());
          m_imgui.on_key_event(input_event->keysym, active);
          if (m_imgui.want_capture_keyboard())  // Inaccurate; this value corresponds to *previous* frame.
            break;                              // So it is possible that events are lost or duplicated. FIXME
        }
        //FIXME: pass key press/release to application here.
        break;
      case EventType::button_release:
      case EventType::button_press:
        if (m_use_imgui)
        {
          m_imgui.on_mouse_move(x, y);
          if (input_event->button <= 2)
          {
            m_imgui.update_modifiers(input_event->modifier_mask.imgui());
            m_imgui.on_mouse_click(input_event->button, active);
          }
          if (m_imgui.want_capture_mouse())     // Inaccurate; this value corresponds to the mouse position during the *previous* frame.
            break;                              // So it is possible that events are lost or duplicated. FIXME
        }
        //FIXME: pass button press/release to application here.
        break;
      case EventType::window_leave:
      case EventType::window_enter:
        if (m_use_imgui)
        {
          m_imgui.on_mouse_enter(x, y, active);
        }
        vulkan::Application::instance().on_mouse_enter(this, x, y, active);
        break;
      case EventType::window_out_focus:
      case EventType::window_in_focus:
        m_in_focus = active;
        if (m_use_imgui)
        {
          m_imgui.on_focus_changed(active);
        }
        //FIXME: pass focus/unfocus event to application here.
        break;
    }
    Dout(dc::vkframe|continued_cf, "Calling m_input_event_buffer.pop() = ");
  }
  Dout(dc::finish, "nullptr");
  // Consume mouse wheel offset.
  float delta_x, delta_y;
  {
    vulkan::WheelOffset::wat wheel_offset_w(m_window_events->m_accumulated_wheel_offset);
    wheel_offset_w->consume(delta_x, delta_y);
  }
  if (!m_in_focus)
    return;
  if (m_use_imgui)
  {
    // Pass most recent mouse position to imgui (for hovering effects).
    int x, y;
    {
      vulkan::MovedMousePosition::wat moved_mouse_position_w(m_window_events->m_mouse_position);
      x = moved_mouse_position_w->x();
      y = moved_mouse_position_w->y();
    }
    m_imgui.on_mouse_move(x, y);
    if (delta_x != 0.f || delta_y != 0.f)
      m_imgui.on_mouse_wheel_event(delta_x, delta_y);
    if (m_imgui.want_capture_mouse())
      return;
  }
  //FIXME: handle delta_x, delta_y for the application here.
}

void SynchronousWindow::wait_for_all_fences() const
{
  std::vector<vk::Fence> all_fences;
  for (auto&& resources : m_frame_resources_list)
    all_fences.push_back(*resources->m_command_buffers_completed);

  vk::Result res;
  {
    CwZoneScopedN("wait for all_fences", number_of_frame_resources(), m_current_frame.m_resource_index);
    res = m_logical_device->wait_for_fences(all_fences, VK_TRUE, 1000000000);
  }
  if (res != vk::Result::eSuccess)
    THROW_FALERTC(res, "wait_for_fences");
}

vk::Extent2D SynchronousWindow::get_extent() const
{
  // Take the lock on m_extent.
  linuxviewer::OS::WindowExtent::crat extent_r(m_window_events->locked_extent());
  // Read the value that was last written.
  vk::Extent2D extent = extent_r->m_extent;
  // Reset the extent_changed_bit in m_flags.
  reset_extent_changed();
  // Return the new extent and unlock m_extent.
  return extent;
  // Because atomic_flags() is called without taking the lock on m_extent
  // it is theorectically possible (this will NEVER happen in practise)
  // that even after returning here, atomic_flags() will again return
  // extent_changed_bit (even though we just reset it); that then would
  // cause another call to this function reading the same value of the extent.
}

void SynchronousWindow::handle_window_size_changed()
{
  DoutEntering(dc::vulkan, "SynchronousWindow::handle_window_size_changed()");

  // No reason to call wait_idle: handle_window_size_changed is called from the render loop.
  // Besides, we can't call wait_idle because another window can still be using queues on the logical device.
  on_window_size_changed_pre();
  // We must wait here until all fences are signaled.
  wait_for_all_fences();
  // Now it is safe to recreate the swapchain.
  vk::Extent2D extent = get_extent();
  m_swapchain.recreate(this, extent
      COMMA_CWDEBUG_ONLY(debug_name_prefix("m_swapchain")));
  uint32_t const layers = m_swapchain.image_kind()->array_layers;
  recreate_framebuffers(extent, layers);
  if (m_use_imgui)
    m_imgui.on_window_size_changed(extent);
  if (can_render(atomic_flags()))
    on_window_size_changed_post();
}

void SynchronousWindow::prepare_begin_info_chains()
{
  // Run over all render passes.
  for (auto render_pass : m_render_passes)
    render_pass->prepare_begin_info_chain();
}

void SynchronousWindow::recreate_framebuffers(vk::Extent2D extent, uint32_t layers)
{
  // Run over all render passes.
  for (auto render_pass : m_render_passes)
    render_pass->create_imageless_framebuffer(extent, layers);
}

bool SynchronousWindow::handle_map_changed(int map_flags)
{
  DoutEntering(dc::vulkan, "SynchronousWindow::handle_map_changed(" << print_map_flags(map_flags) << ")");
  reset_map_changed();

  bool is_minimized = !is_mapped(map_flags);

  if (is_minimized)
    set_minimized();
  else
    set_unminimized();

  // Returns true on success, false if there is another map change required.
  return handled_map_changed(map_flags);
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

  // Allocate a temporary command buffer from the temporary command pool.
  vulkan::handle::CommandBuffer tmp_command_buffer = tmp_command_pool.allocate_buffer(
      CWDEBUG_ONLY(debug_name_prefix("set_image_memory_barrier()::tmp_command_buffer")));

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
    tmp_command_buffer.begin({ .flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit });
    tmp_command_buffer.pipelineBarrier(source.pipeline_stage_mask, destination.pipeline_stage_mask, {}, {}, {}, { image_memory_barrier });
    tmp_command_buffer.end();
  }

  // Submit
  {
    auto fence = logical_device()->create_fence(false
        COMMA_CWDEBUG_ONLY(mSMDebug, debug_name_prefix("set_image_memory_barrier()::fence")));

    vk::SubmitInfo submit_info{
      .waitSemaphoreCount = 0,
      .pWaitSemaphores = nullptr,
      .pWaitDstStageMask = nullptr,
      .commandBufferCount = 1,
      .pCommandBuffers = &tmp_command_buffer,
      .signalSemaphoreCount = 0,
      .pSignalSemaphores = nullptr
    };
    m_presentation_surface.vh_graphics_queue().submit({ submit_info }, *fence);

    int count = 10;
    vk::Result res;
    do
    {
      {
        ZoneScopedNC("wait for set_image_memory_barrier()::fence", 0x9C2022);     // color: "Old Brick".
        res = logical_device()->wait_for_fences({ *fence }, VK_TRUE, 300000000);
      }
      if (res != vk::Result::eTimeout)
        break;
      Dout(dc::notice, "wait_for_fences timed out " << count);
    }
    while (--count > 0);
    if (res != vk::Result::eSuccess)
    {
      Dout(dc::warning, "wait_for_fences returned " << res);
      logical_device()->reset_fences({ *fence });
      THROW_ALERTC(res, "waitForFences");
    }
  }
}

void SynchronousWindow::detect_if_imgui_is_used()
{
  m_use_imgui = imgui_pass.vh_render_pass();
  if (!m_use_imgui)
    unregister_render_pass(&imgui_pass);
}

void SynchronousWindow::start_frame()
{
  ZoneNamed(start_frame_scoped_zone, true);
  DoutEntering(dc::vkframe, "SynchronousWindow::start_frame()");

  m_current_frame.m_resource_index = (m_current_frame.m_resource_index + 1) % m_current_frame.m_resource_count;
  m_current_frame.m_frame_resources = m_frame_resources_list[m_current_frame.m_resource_index].get();

  if (m_use_imgui)
  {
    m_imgui.start_frame(m_imgui_timer.get_delta_ms() * 0.001f);
    draw_imgui();
  }
}

void SynchronousWindow::wait_command_buffer_completed()
{
  CwZoneScopedN("m_command_buffers_completed", number_of_frame_resources(), m_current_frame.m_resource_index);
#if defined(CWDEBUG) && defined(NON_FATAL_LONG_FENCE_DELAY)
  // You might want to use this if a time out happens while debugging (for example stepping through code with a debugger).
  while (m_logical_device->wait_for_fences({ *m_current_frame.m_frame_resources->m_command_buffers_completed }, VK_FALSE, 1000000000) != vk::Result::eSuccess)
    Dout(dc::warning, "WAITING FOR A FENCE TOOK TOO LONG!");
#else
  // Normally, this is an error.
  if (m_logical_device->wait_for_fences({ *m_current_frame.m_frame_resources->m_command_buffers_completed }, VK_FALSE, 1000000000) != vk::Result::eSuccess)
    throw std::runtime_error("Waiting for a fence takes too long!");
#endif
}

void SynchronousWindow::finish_frame()
{
  DoutEntering(dc::vkframe, "SynchronousWindow::finish_frame(...)");

  // Present frame

  vk::Result result = vk::Result::eSuccess;
  vk::SwapchainKHR vh_swapchain = *m_swapchain;
  uint32_t const swapchain_image_index = m_swapchain.current_index().get_value();
  vk::PresentInfoKHR present_info{
    .waitSemaphoreCount = 1,
    .pWaitSemaphores = m_swapchain.vhp_current_rendering_finished_semaphore(),
    .swapchainCount = 1,
    .pSwapchains = &vh_swapchain,
    .pImageIndices = &swapchain_image_index
  };

  vk::Result res;
  {
    Dout(dc::vkframe, "Calling presentKHR with .pWaitSemaphores = " << present_info.pWaitSemaphores[0]);
    CwZoneScopedN("presentKHR", number_of_swapchain_images(), m_swapchain.current_index());
    res = m_presentation_surface.vh_presentation_queue().presentKHR(&present_info);
  }
#ifdef TRACY_ENABLE
  if (res == vk::Result::eSuccess || res == vk::Result::eSuboptimalKHR)
  {
    // Tracy can't deal with multiple windows; only call FrameMark for the window that has focus.
    if (m_is_tracy_window)
      FrameMark   // Tracy
  }
  {
    std::string msg = "presented image " + to_string(m_swapchain.current_index());
    TracyMessage(msg.data(), msg.size());

    vulkan::SwapchainIndex swapchain_index = swapchain().current_index();
    //Dout(dc::vulkan, "Calling TracyCZoneEnd while in fiber \"" << s_tl_tracy_fiber_name << "\" with ctx with id " <<
    //    tracy_acquired_image_tracy_context[swapchain_index].id << " from " << this << "->tracy_acquired_image_tracy_context[" << swapchain_index << "]");
    TracyCZoneEnd(tracy_acquired_image_tracy_context[swapchain_index]);
    tracy_acquired_image_busy[swapchain_index] = false;
  }
#endif
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

void SynchronousWindow::acquire_image()
{
  DoutEntering(dc::vkframe, "SynchronousWindow::acquire_image() [" << this << "]");
  {
    ZoneScopedN("acquire_image");

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

#ifdef TRACY_ENABLE
  vulkan::SwapchainIndex swapchain_index = m_swapchain.current_index();
  ASSERT(!tracy_acquired_image_busy[swapchain_index]);
  //Dout(dc::vulkan, "Calling TracyCZone(ctx, \"acquire_next_image<---presentKHR<\") while in fiber \"" << s_tl_tracy_fiber_name << "\".");
  CwTracyCZoneN(ctx, "acquire_next_image<---presentKHR<", 1, number_of_swapchain_images(), m_swapchain.current_index());
  //Dout(dc::vulkan, "Storing ctx with id " << ctx.id << " in " << this << "->tracy_acquired_image_tracy_context[" << swapchain_index << "]");
  tracy_acquired_image_tracy_context[swapchain_index] = ctx;
  tracy_acquired_image_busy[swapchain_index] = true;
#endif
}

void SynchronousWindow::finish_impl()
{
  DoutEntering(dc::vulkan, "SynchronousWindow::finish_impl() [" << this << "]");
  // Finishing the task causes us to leave the run() function and then leave the scope
  // of the boost::intrusive_ptr that keeps this task alive. When it gets destructed,
  // also Window will be destructed and its destructor does the work required to close
  // the window.
  //
  // However, we need to destroy all child windows because each has a
  // boost::intrusive_ptr<task::SynchronousWindow const> that points to us.
  {
    child_window_list_t::rat child_window_list_r(m_child_window_list);
    for (auto child_window : *child_window_list_r)
      child_window->close();
  }

  // Finally, remove us from Application::window_list_t or else this SynchronousWindow
  // won't be destructed as the list stores boost::intrusive_ptr<task::SynchronousWindow>'s.
  m_application->remove(this);

  // Abort all dependent tasks before destructing (this could even be done from the destructor,
  // if it wasn't that we also guard members of derived classes).
  m_dependent_tasks.abort_all();

  // Run the synchronous tasks, to give them a chance to abort.
  if (have_synchronous_task(atomic_flags()))
    handle_synchronous_tasks(CWDEBUG_ONLY(mSMDebug));

  // Wait for (certain) tasks to be finished, while giving CPU to possibly still running synchronous tasks.
  // Currently this waits for CopyDataToGPU and PipelineCache tasks; the most common reason for the latter
  // not finishing is because not all PipelineFactory tasks finished.
  while (!m_task_counter_gate.wait_for(100))
    while (mainloop().is_true())
      ;

  // Wait for semaphores to be finished (before destructing them).
  if (m_logical_device)
  {
    m_logical_device->wait_idle();
  }
}

//virtual
threadpool::Timer::Interval SynchronousWindow::frame_rate_interval() const
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

  // For all depth attachments.

#if 0
  vk::ImageSubresourceRange const depth_subresource_range =
    vk::ImageSubresourceRange{vulkan::Swapchain::s_default_subresource_range}
      .setAspectMask(vk::ImageAspectFlagBits::eDepth);

  vulkan::ResourceState const initial_depth_resource_state = {
    .pipeline_stage_mask        = vk::PipelineStageFlagBits::eEarlyFragmentTests,
    .access_mask                = vk::AccessFlagBits::eDepthStencilAttachmentWrite,
    .layout                     = vk::ImageLayout::eDepthStencilAttachmentOptimal
  };
#endif

#ifdef CWDEBUG
  vulkan::FrameResourceIndex frame_resource_index{0};
#endif
  // Run over all frame resources.
  for (std::unique_ptr<vulkan::FrameResourcesData> const& frame_resources_data : m_frame_resources_list)
  {
    // Run over all attachments.
    for (Attachment const* attachment : m_attachments)
    {
      if (attachment->index().undefined())      // Skip swapchain attachment.
        continue;
      Dout(dc::vulkan, "Creating attachment \"" << attachment->name() << "\".");
      frame_resources_data->m_attachments[*attachment] = vulkan::Attachment(
          m_logical_device,
          swapchain().extent(),
          attachment->image_view_kind(),
          0,
          vk::MemoryPropertyFlagBits::eDeviceLocal
          COMMA_CWDEBUG_ONLY(debug_name_prefix("m_frame_resources_list[" + to_string(frame_resource_index) +
              "]->m_attachments[" + to_string(attachment->index()) + "]")));
    }
#ifdef CWDEBUG
    ++frame_resource_index;
#endif
  }
}

//virtual
// Override this function to change this value.
vulkan::FrameResourceIndex SynchronousWindow::number_of_frame_resources() const
{
  return s_default_number_of_frame_resources;
}

//virtual
// Override this function to change this value.
vulkan::SwapchainIndex SynchronousWindow::number_of_swapchain_images() const
{
  return s_default_number_of_swapchain_images;
}

//virtual
// Override this function to change this value.
std::u8string SynchronousWindow::pipeline_cache_name() const
{
  // set_title must already be called here (and may not be empty).
  ASSERT(!m_title.empty());
  // The default filename is the title of the window.
  return m_title;
}

//virtual
// Override this function to change these values.
void SynchronousWindow::set_default_clear_values(vulkan::rendergraph::ClearValue& color, vulkan::rendergraph::ClearValue& depth_stencil)
{
  Dout(dc::vulkan, "Using default clear values; color: " << color << ", depth_stencil: " << depth_stencil);
  // These are already the default.
  //color.set(1.f, 0.f, 0.f, 1.f);      or  color = { 1.f, 0.f, 0.f, 1.f };
  //depth_stencil.set(1.f);             or  color = 1.f;
  //depth_stencil.set(1.f, 0xffffffff);
}

void SynchronousWindow::create_frame_resources()
{
  DoutEntering(dc::vulkan, "SynchronousWindow::create_frame_resources() [" << this << "]");

  vulkan::FrameResourceIndex const number_of_frame_resources = this->number_of_frame_resources();

  Dout(dc::vulkan, "Creating " << number_of_frame_resources.get_value() << " frame resources.");
  m_frame_resources_list.resize(number_of_frame_resources.get_value());
  for (vulkan::FrameResourceIndex i = m_frame_resources_list.ibegin(); i != m_frame_resources_list.iend(); ++i)
  {
#ifdef CWDEBUG
    vulkan::AmbifixOwner const ambifix = debug_name_prefix("m_frame_resources_list[" + to_string(i) + "]");
#endif
    // Create a new vulkan::FrameResourcesData object.
    m_frame_resources_list[i] = std::make_unique<vulkan::FrameResourcesData>(
        // The total number of attachments used in the rendergraph, excluding the swapchain attachment.
        number_of_registered_attachments(),
        // Constructor arguments for m_command_pool.
        m_logical_device, m_presentation_surface.graphics_queue().queue_family()
        COMMA_CWDEBUG_ONLY("->m_command_pool" + ambifix));

    // A handle alias for the newly created frame resources object.
    auto& frame_resources = m_frame_resources_list[i];

    // Create the 'command_buffers_completed' fence (using bufferS, even though we only have a single command buffer right now).
    frame_resources->m_command_buffers_completed = m_logical_device->create_fence(true COMMA_CWDEBUG_ONLY(true, "->m_command_buffers_completed" + ambifix));

    // Create the command buffer.
    frame_resources->m_command_buffer = frame_resources->m_command_pool.allocate_buffer(
        CWDEBUG_ONLY("->m_command_buffer" + ambifix));

#if 0 // FIXME: See FIXME above.
    // Move the overlapping descriptor set into m_frame_resources_list.
    frame_resources->m_overlapping_descriptor_set = std::move(overlapping_descriptor_sets[i]);
#endif
  }

  if (m_use_imgui)
    m_imgui.create_frame_resources(number_of_frame_resources
        COMMA_CWDEBUG_ONLY(debug_name_prefix("m_imgui")));

  // Initialize m_current_frame to point to frame resources index 0.
  m_current_frame = vulkan::CurrentFrameData{
    .m_frame_resources = m_frame_resources_list.begin()->get(),
    .m_resource_count = static_cast<vulkan::FrameResourceIndex>(m_frame_resources_list.size()),
    .m_resource_index = static_cast<vulkan::FrameResourceIndex>(0)
  };

  // Initialize all attachments (images, image views, memory).
  on_window_size_changed_post();
}

void SynchronousWindow::submit(vulkan::handle::CommandBuffer command_buffer)
{
#ifdef TRACY_ENABLE
  tracy::IndexPair const current_index_pair(m_current_frame.m_resource_index, m_swapchain.current_index(), number_of_swapchain_images());
#if 0
  CwZoneScopedN("submit",
          tracy::IndexPair(number_of_frame_resources(), vulkan::SwapchainIndex{0}, number_of_swapchain_images()),
          current_index_pair);
#endif
  CwZoneNamedN(__submit1, "submit", true, number_of_frame_resources(), m_current_frame.m_resource_index);
  CwZoneNamedN(__submit2, "submit", true, number_of_swapchain_images(), m_swapchain.current_index());
#endif

  vk::PipelineStageFlags wait_dst_stage_mask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
  vk::SubmitInfo submit_info{
    .waitSemaphoreCount = 1,
    .pWaitSemaphores = swapchain().vhp_current_image_available_semaphore(),
    .pWaitDstStageMask = &wait_dst_stage_mask,
    .commandBufferCount = 1,
    .pCommandBuffers = &command_buffer,
    .signalSemaphoreCount = 1,
    .pSignalSemaphores = swapchain().vhp_current_rendering_finished_semaphore()
  };

  Dout(dc::vkframe, "Submitting command buffer: submit({" << submit_info << "}, " << *m_current_frame.m_frame_resources->m_command_buffers_completed << ")");
  presentation_surface().vh_graphics_queue().submit({ submit_info }, *m_current_frame.m_frame_resources->m_command_buffers_completed);

#ifdef TRACY_ENABLE
  std::string message("Submitted CB ");
  message += to_string(current_index_pair);
  TracyMessage(message.data(), message.size());
#endif
}

void SynchronousWindow::copy_graphics_settings()
{
  DoutEntering(dc::vulkan, "SynchronousWindow::copy_graphics_settings() [" << this << "]");
  m_application->copy_graphics_settings_to(&m_graphics_settings, m_logical_device);
}

#if 0
void SynchronousWindow::load_pipeline_cache()
{
  DoutEntering(dc::vulkan, "SynchronousWindow::load_pipeline_cache()");
  // If this is a root window - then we might be the first to have access to the logical device.
  // Try to register it with the pipeline cache.
  if (!m_parent_window_task)
  {
    vulkan::PipelineCache* pipeline_cache = m_application->pipeline_cache_broker(m_logical_device);
    if (pipeline_cache)
    {
      // The first thread that locks Application::m_pipeline_cache_list for a given m_logical_device
      // will have pipeline_cache != nullptr here. It might not be the first thread to return
      // from pipeline_cache_broker - but other threads will wait in the while loop below until this
      // thread stored the pipeline_cache pointer.
      //
      // Any thread that reaches that while loop, while this thread didn't finish writing to the
      // LogicalDevice::m_pipeline_cache atomic yet, must belong to a different root window with its
      // own SynchronousWindow task. But waiting until m_logical_device->pipeline_cache() returns
      // a non-null value is enough to assure that that task will see the correct pointer in
      // subsequent runs because this function is called in the critical area of that task.
      //
      // For all the gory details,
      // see https://stackoverflow.com/questions/71340714/lock-free-write-once-read-often-value-in-conjunction-with-thread-pools
      m_logical_device->store_pipeline_cache(pipeline_cache);
    }
    else
    {
      // Wait until the thread that called pipeline_cache_broker with success finished storing
      // the returned pointer.
      while (m_logical_device->pipeline_cache() == nullptr)
        cpu_relax();
    }
  }
}
#endif

void SynchronousWindow::add_synchronous_task(std::function<void(SynchronousWindow*)> lambda)
{
  DoutEntering(dc::vulkan, "SynchronousWindow::add_synchronous_task(...)");
  auto synchronize_task = new SynchronousTask(this COMMA_CWDEBUG_ONLY(true));
  synchronize_task->run([lambda, this](bool){ lambda(this); });
}

vulkan::pipeline::FactoryHandle SynchronousWindow::create_pipeline_factory(vulkan::Pipeline& pipeline_out, vk::RenderPass vh_render_pass COMMA_CWDEBUG_ONLY(bool debug))
{
  auto factory = statefultask::create<PipelineFactory>(this, pipeline_out, vh_render_pass COMMA_CWDEBUG_ONLY(debug));
  PipelineFactoryIndex const index = m_pipeline_factories.iend();
  m_pipeline_factories.push_back(std::move(factory));           // Now m_pipeline_factories[index] == factory.
  m_pipelines.emplace_back();
  m_application->run_pipeline_factory(m_pipeline_factories[index], this, index);
  m_pipeline_factories[index]->set_index(index);
  return index;
}

void SynchronousWindow::have_new_pipeline(vulkan::Pipeline&& pipeline_handle_and_layout, vk::UniquePipeline&& pipeline)
{
  DoutEntering(dc::vulkan, "SynchronousWindow::have_new_pipeline(" << pipeline_handle_and_layout << ", " << *pipeline << ")");
  vulkan::pipeline::Handle const& pipeline_handle = pipeline_handle_and_layout.handle();
  auto& factory_pipelines = m_pipelines[pipeline_handle.m_pipeline_factory_index];
  if (factory_pipelines.iend() <= pipeline_handle.m_pipeline_index)
    factory_pipelines.resize(pipeline_handle.m_pipeline_index.get_value() + 1);
  factory_pipelines[pipeline_handle.m_pipeline_index] = std::move(pipeline);
  m_pipeline_factories[pipeline_handle.m_pipeline_factory_index]->set_pipeline(std::move(pipeline_handle_and_layout));
}

vk::Pipeline SynchronousWindow::vh_graphics_pipeline(vulkan::pipeline::Handle pipeline_handle) const
{
  return *m_pipelines[pipeline_handle.m_pipeline_factory_index][pipeline_handle.m_pipeline_index];
}

void SynchronousWindow::pipeline_factory_done(utils::Badge<synchronous::MoveNewPipelines>, PipelineFactoryIndex index)
{
  DoutEntering(dc::notice, "SynchronousWindow::pipeline_factory_done(" << index << ")");
  boost::intrusive_ptr<PipelineCache> pipeline_cache(m_pipeline_factories[index]->detach_pipeline_cache_task());
  m_pipeline_factories[index].reset();          // Delete the pipeline factory task.
  m_application->pipeline_factory_done(this, std::move(pipeline_cache));
}

#ifdef CWDEBUG
vulkan::AmbifixOwner SynchronousWindow::debug_name_prefix(std::string prefix) const
{
  return { this, std::move(prefix) };
}
#endif

} // namespace task
