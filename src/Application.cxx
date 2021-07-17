#include "sys.h"
#include "Application.h"
#include "WindowCreateInfo.h"

void Application::run(int argc, char* argv[], WindowCreateInfo const& main_window_create_info)
{
  DoutEntering(dc::notice|flush_cf|continued_cf, "Application::run(" << argc << ", ");
  // Print the commandline arguments used.
  char const* prefix = "{";
  for (char** arg = argv; *arg; ++arg)
  {
    Dout(dc::continued|flush_cf, prefix << '"' << *arg << '"');
    prefix = ", ";
  }
  Dout(dc::finish|flush_cf, '}');

  gui::Application::main(main_window_create_info);
}

bool Application::on_gui_idle()
{
  m_gui_idle_engine.mainloop();

  // Returning true means we want to be called again (more work is to be done).
  return false;
}
