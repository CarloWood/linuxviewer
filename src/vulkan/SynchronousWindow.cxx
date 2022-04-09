#include "sys.h"
#include "SynchronousWindow.h"
#include "OperatingSystem.h"
#include "LogicalDevice.h"
#include "Application.h"
#include "FrameResourcesData.h"
#include "StagingBufferParameters.h"
#include "Exceptions.h"
#include "SynchronousTask.h"
#include "pipeline/Handle.h"
#include "vk_utils/print_flags.h"
#include "xcb-task/ConnectionBrokerKey.h"
#include "debug/DebugSetName.h"
#include "vulkan/vk_format_utils.h"
#include "utils/cpu_relax.h"
#include "utils/u8string_to_filename.h"
#ifdef CWDEBUG
#include "debug/vulkan_print_on.h"
#include "utils/debug_ostream_operators.h"
#include "utils/at_scope_end.h"
#endif
#include <algorithm>
#include "debug.h"

#if defined(CWDEBUG) && !defined(DOXYGEN)
NAMESPACE_DEBUG_CHANNELS_START
channel_ct renderloop("RENDERLOOP");
NAMESPACE_DEBUG_CHANNELS_END
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

//static
vulkan::ImageKind const SynchronousWindow::s_color_image_kind({});      // The default is exactly what we want, as default.

//static
vulkan::ImageViewKind const SynchronousWindow::s_color_image_view_kind(s_color_image_kind, {});

SynchronousWindow::SynchronousWindow(vulkan::Application* application COMMA_CWDEBUG_ONLY(bool debug)) :
  AIStatefulTask(CWDEBUG_ONLY(debug)), SynchronousEngine("SynchronousEngine", 10.0f),
  m_application(application), m_frame_rate_limiter([this](){ signal(frame_timer); }),
  attachment_index_context(vulkan::rendergraph::AttachmentIndex{0})
  COMMA_CWDEBUG_ONLY(mVWDebug(mSMDebug))
{
  DoutEntering(dc::statefultask(mSMDebug), "task::SynchronousWindow::SynchronousWindow(" << application << ") [" << (void*)this << "]");
}

SynchronousWindow::~SynchronousWindow()
{
  DoutEntering(dc::statefultask(mSMDebug), "task::SynchronousWindow::~SynchronousWindow() [" << (void*)this << "]");
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
#if CW_DEBUG
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
    AI_CASE_RETURN(SynchronousWindow_initialize_vukan);
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
      set_state(SynchronousWindow_initialize_vukan);
      [[fallthrough]];
    case SynchronousWindow_initialize_vukan:
      copy_graphics_settings();
      set_default_clear_values(m_render_graph.m_default_color_clear_value, m_render_graph.m_default_depth_stencil_clear_value);
      prepare_swapchain();
      create_render_graph();
      create_swapchain_images();
      create_frame_resources();
      create_imageless_framebuffers();  // Must be called after create_swapchain_images()!
      create_descriptor_set();
      create_textures();
      create_pipeline_layout();
      create_graphics_pipelines();
      if (m_use_imgui)                  // Set in create_descriptor_set by the user.
        create_imgui();

      set_state(SynchronousWindow_render_loop);
      // We already have a swapchain up there - but only now we can really render anything, so set it here.
      vulkan::SynchronousEngine::have_swapchain();
      // Turn off debug output for this statefultask while processing the render loop.
      Debug(mSMDebug = false);
      [[fallthrough]];
    case SynchronousWindow_render_loop:
    {
      if (m_use_imgui)
      {
        // Set a thread-local global variable that imgui uses to access its context.
        m_imgui.set_current_context();
      }
#if CW_DEBUG
      RenderLoopEntered scoped(m_title.c_str());
#endif
      int special_circumstances = atomic_flags();
      for (;;)  // So that we can use continue to try and render again, break to close the window and return when we're done until the next frame.
      {
        if (AI_LIKELY(!special_circumstances))
        {
          try
          {
            // Render the next frame.
            m_frame_rate_limiter.start(m_frame_rate_interval);
            m_timer.update();   // Keep track of FPS and stuff.
            consume_input_events();
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

  vulkan::Queue vh_graphics_queue = logical_device().acquire_queue(QueueFlagBits::eGraphics|QueueFlagBits::ePresentation, m_request_cookie);
  vulkan::Queue vh_presentation_queue;

  if (!vh_graphics_queue)
  {
    // The combination of eGraphics|ePresentation failed. We have to try separately.
    vh_graphics_queue = logical_device().acquire_queue(QueueFlagBits::eGraphics, m_request_cookie);
    vh_presentation_queue = logical_device().acquire_queue(QueueFlagBits::ePresentation, m_request_cookie);
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

void SynchronousWindow::create_swapchain_images()
{
  m_swapchain.recreate(this, get_extent()
      COMMA_CWDEBUG_ONLY(debug_name_prefix("m_swapchain")));
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

  m_imgui.init(this, attachment_description.samples
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
        //FIXME: pass enter/leave event to application here.
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

vulkan::Texture SynchronousWindow::upload_texture(void const* texture_data, uint32_t width, uint32_t height,
    int binding, vulkan::ImageViewKind const& image_view_kind, vulkan::SamplerKind const& sampler_kind, vk::DescriptorSet descriptor_set
    COMMA_CWDEBUG_ONLY(vulkan::AmbifixOwner const& ambifix)) const
{
  // Create texture parameters.
  vulkan::Texture texture = m_logical_device->create_texture(width, height,
      image_view_kind, vk::MemoryPropertyFlagBits::eDeviceLocal, sampler_kind, m_graphics_settings
      COMMA_CWDEBUG_ONLY(ambifix));

  size_t const data_size = width * height * vk_utils::format_component_count(image_view_kind.image_kind()->format);

  // Copy data.
  {
    vk_defaults::ImageSubresourceRange const image_subresource_range;
    copy_data_to_image(data_size, texture_data, *texture.m_image, width, height, image_subresource_range, vk::ImageLayout::eUndefined,
        vk::AccessFlags(0), vk::PipelineStageFlagBits::eTopOfPipe, vk::ImageLayout::eShaderReadOnlyOptimal, vk::AccessFlagBits::eShaderRead,
        vk::PipelineStageFlagBits::eFragmentShader);
  }

  // Update descriptor set.
  {
    std::vector<vk::DescriptorImageInfo> image_infos = {
      {
        .sampler = *texture.m_sampler,
        .imageView = *texture.m_image_view,
        .imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal
      }
    };
    m_logical_device->update_descriptor_set(descriptor_set, vk::DescriptorType::eCombinedImageSampler, binding, 0, image_infos);
  }

  return texture;
}

void SynchronousWindow::detect_if_imgui_is_used()
{
  m_use_imgui = imgui_pass.vh_render_pass();
  if (!m_use_imgui)
    unregister_render_pass(&imgui_pass);
}

void SynchronousWindow::start_frame()
{
  DoutEntering(dc::vkframe, "SynchronousWindow::start_frame()");
  m_current_frame.m_resource_index = (m_current_frame.m_resource_index + 1) % m_current_frame.m_resource_count;
  m_current_frame.m_frame_resources = m_frame_resources_list[m_current_frame.m_resource_index].get();

  if (m_use_imgui)
  {
    m_imgui.start_frame(m_timer.get_delta_ms() * 0.001f);
    draw_imgui();
  }

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
      vulkan::Attachment& attachment_ref = frame_resources_data->m_attachments[*attachment];
      attachment_ref = m_logical_device->create_attachment(
          swapchain().extent().width,
          swapchain().extent().height,
          attachment->image_view_kind(),
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
        COMMA_CWDEBUG_ONLY(ambifix("->m_command_pool")));

    // A handle alias for the newly created frame resources object.
    auto& frame_resources = m_frame_resources_list[i];

    // Create the 'command_buffers_completed' fence (even we only have a single command buffer right now).
    frame_resources->m_command_buffers_completed = m_logical_device->create_fence(true COMMA_CWDEBUG_ONLY(true, ambifix("->m_command_buffers_completed")));

    // Create the command buffer.
    {
      // Lock command pool.
      vulkan::FrameResourcesData::command_pool_type::wat command_pool_w(frame_resources->m_command_pool);

      frame_resources->m_command_buffer = command_pool_w->allocate_buffer(
          CWDEBUG_ONLY(ambifix("->m_command_buffer")));

    } // Unlock command pool.
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

void SynchronousWindow::submit(vulkan::CommandBufferWriteAccessType<vulkan::FrameResourcesData::pool_type>& command_buffer_w)
{
  vk::PipelineStageFlags wait_dst_stage_mask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
  vk::SubmitInfo submit_info{
    .waitSemaphoreCount = 1,
    .pWaitSemaphores = swapchain().vhp_current_image_available_semaphore(),
    .pWaitDstStageMask = &wait_dst_stage_mask,
    .commandBufferCount = 1,
    .pCommandBuffers = command_buffer_w,
    .signalSemaphoreCount = 1,
    .pSignalSemaphores = swapchain().vhp_current_rendering_finished_semaphore()
  };

  Dout(dc::vkframe, "Submitting command buffer: submit({" << submit_info << "}, " << *m_current_frame.m_frame_resources->m_command_buffers_completed << ")");
  presentation_surface().vh_graphics_queue().submit( { submit_info }, *m_current_frame.m_frame_resources->m_command_buffers_completed );
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

vulkan::pipeline::FactoryHandle SynchronousWindow::create_pipeline_factory(vk::PipelineLayout vh_pipeline_layout, vk::RenderPass vh_render_pass COMMA_CWDEBUG_ONLY(bool debug))
{
  auto factory = statefultask::create<PipelineFactory>(this, vh_pipeline_layout, vh_render_pass COMMA_CWDEBUG_ONLY(debug));
  auto const index = m_pipeline_factories.iend();
  m_pipeline_factories.push_back(std::move(factory));           // Now m_pipeline_factories[index] == factory.
  m_pipelines.emplace_back();
  m_application->run_pipeline_factory(m_pipeline_factories[index], this, index);
  m_pipeline_factories[index]->set_index(index);
  return index;
}

void SynchronousWindow::have_new_pipeline(vulkan::pipeline::Handle pipeline_handle, vk::UniquePipeline&& pipeline)
{
  auto& factory_pipelines = m_pipelines[pipeline_handle.m_pipeline_factory_index];
  if (factory_pipelines.iend() <= pipeline_handle.m_pipeline_index)
    factory_pipelines.resize(pipeline_handle.m_pipeline_index.get_value() + 1);
  factory_pipelines[pipeline_handle.m_pipeline_index] = std::move(pipeline);
  new_pipeline(pipeline_handle);
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
