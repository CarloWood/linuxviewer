#include "sys.h"
#include "VulkanWindow.h"
#include "OperatingSystem.h"
#include "LogicalDevice.h"
#include "Application.h"
#include "xcb-task/ConnectionBrokerKey.h"
#ifdef CWDEBUG
#include "utils/debug_ostream_operators.h"
#endif

namespace task {

VulkanWindow::VulkanWindow(vulkan::Application* application, std::unique_ptr<linuxviewer::OS::Window>&& window COMMA_CWDEBUG_ONLY(bool debug)) :
  AIStatefulTask(CWDEBUG_ONLY(debug)), m_application(application), m_window(std::move(window)), m_frame_rate_limiter([this](){ signal(frame_timer); }), mVWDebug(mSMDebug)
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

vulkan::LogicalDevice const* VulkanWindow::logical_device() const
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
      if (AI_LIKELY(!m_must_close))
      {
        wait(frame_timer);
        m_frame_rate_limiter.start(m_frame_rate_interval);
        yield(m_application->m_medium_priority_queue);
        break;
      }
      set_state(VulkanWindow_close);
      [[fallthrough]];
    case VulkanWindow_close:
      // Turn on debug output again.
      Debug(mSMDebug = mVWDebug);
      finish();
      Dout(dc::notice, "this = " << this);
      tracked::intrusive_ptr<VulkanWindow>::for_each_instance([](tracked::intrusive_ptr<VulkanWindow> const* ptr){
          Dout(dc::notice, "Object at " << (void*)ptr->get() << " kept alive by " << print_using(ptr, &tracked::intrusive_ptr<VulkanWindow>::print_tracker_info_on)); });
      break;
  }
}

void VulkanWindow::acquire_queues()
{
  DoutEntering(dc::vulkan, "VulkanWindow::acquire_queues()");
  using vulkan::QueueFlagBits;

  vulkan::LogicalDevice* logical_device = m_application->get_logical_device(m_logical_device_index);

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
