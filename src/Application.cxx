#include "sys.h"
#include "Application.h"

bool Application::on_gui_idle()
{
  m_gui_idle_engine.mainloop();

  // Returning true means we want to be called again (more work is to be done).
  return false;
}
