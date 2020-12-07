#include "sys.h"
#include "LinuxViewerApplication.h"
#include "helloworld-task/HelloWorld.h"
#include "statefultask/AIEngine.h"

//static
std::unique_ptr<LinuxViewerApplication> LinuxViewerApplication::create(AIEngine& gui_idle_engine)
{
  return std::make_unique<LinuxViewerApplication>(gui_idle_engine);
}

// Called when the main instance (as determined by the GUI) of the application is starting.
void LinuxViewerApplication::on_main_instance_startup()
{
  auto task = task::create<task::HelloWorld>();
  task->initialize(42);
  task->run(&m_gui_idle_engine, [=](bool CWDEBUG_ONLY(success)){
      Dout(dc::notice, "Inside the call-back (" <<
          (success ? "success" : "failure") << ").");
      task->run();      // Restart the task from the beginning.
  });
}

// This is called from the main loop of the GUI during "idle" cycles.
bool LinuxViewerApplication::on_gui_idle()
{
  m_gui_idle_engine.mainloop();

  // Returning true means we want to be called again (more work is to be done).
  return true;
}
