#include "sys.h"
#include "VulkanWindow.h"
#include "OperatingSystem.h"
#include "Application.h"
#include "xcb-task/ConnectionBrokerKey.h"

namespace task {

VulkanWindow::VulkanWindow(vulkan::Application* application, std::unique_ptr<linuxviewer::OS::Window>&& window COMMA_CWDEBUG_ONLY(bool debug)) :
  AIStatefulTask(CWDEBUG_ONLY(debug)), m_application(application), m_window(std::move(window)), m_frame_rate_limiter([this](){ signal(frame_timer); }), mVWDebug(mSMDebug)
{
  DoutEntering(dc::statefultask(mSMDebug), "task::VulkanWindow::VulkanWindow(" << application << ", " << (void*)m_window.get() << ") [" << (void*)this << "]");
}

VulkanWindow::~VulkanWindow()
{
  DoutEntering(dc::statefultask(mSMDebug), "task::VulkanWindow::~VulkanWindow() [" << (void*)this << "]");
  m_frame_rate_limiter.stop();
  if (m_window)
    m_window->destroy();
}

char const* VulkanWindow::state_str_impl(state_type run_state) const
{
  switch (run_state)
  {
    AI_CASE_RETURN(VulkanWindow_xcb_connection);
    AI_CASE_RETURN(VulkanWindow_create);
    AI_CASE_RETURN(VulkanWindow_render_loop);
    AI_CASE_RETURN(VulkanWindow_close);
  }
  ASSERT(false);
  return "UNKNOWN STATE";
}

void VulkanWindow::initialize_impl()
{
  DoutEntering(dc::vulkan, "VulkanWindow::initialize_impl() [" << this << "]");

  m_application->add(this);
  set_state(VulkanWindow_xcb_connection);
}

void VulkanWindow::multiplex_impl(state_type run_state)
{
  switch (run_state)
  {
    case VulkanWindow_xcb_connection:
      m_xcb_connection_task = m_broker->run(*m_broker_key, [this](bool success){ Dout(dc::notice, "xcb_connection finished!"); signal(connection_set_up); });
      set_state(VulkanWindow_create);
      wait(connection_set_up);
      break;
    case VulkanWindow_create:
      m_window->set_xcb_connection(m_xcb_connection_task->connection());
      m_window->create(m_title, m_extent.width, m_extent.height, this);
      m_window_created_event.trigger();
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
      break;
  }
}

void VulkanWindow::finish_impl()
{
  DoutEntering(dc::vulkan, "VulkanWindow::finish_impl() [" << this << "]");
  // Not doing anything here, finishing the task causes us to leave the run()
  // function and then leave the scope of the boost::intrusive_ptr that keeps
  // this tasks alive. When it gets destructed, also Window will be destructed
  // and it's destructor does the work required to close the window.
  m_application->remove(this);
}

} // namespace task
