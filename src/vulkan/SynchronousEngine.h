#include "sys.h"

#include "SpecialCircumstances.h"
#include "statefultask/AIEngine.h"

namespace vulkan {

class SynchronousEngine : public AIEngine, protected SpecialCircumstances
{
 protected:
  SynchronousEngine(char const* name, float max_duration) : AIEngine(name, max_duration)
  {
    // SynchronousEngine must have a positive value for max_duration: the maximum
    // duration per loop (in milliseconds) during which new tasks are executed.
    ASSERT(max_duration > 0);
  }

  void handle_synchronous_tasks(CWDEBUG_ONLY(bool debug_output))
  {
    DoutEntering(dc::vulkan(debug_output), "SynchronousEngine::handle_synchronous_tasks()");
    // Reset the bit in advance.
    //
    // The bit is set after adding something to the engine.
    // Resetting the bit before running the engine we might
    // call mainloop() too often when there are no running
    // tasks in the engine, but an added task will never be
    // forgotten.
    //
    //    Other thread(s)   This thread
    //    [Add task]-.
    //    <Set flag>-+.
    //               | \
    //    [Add task]-\  \
    //                \  `-<Reset flag>
    //    <Set flag>-. `-->[Execute task(s)]
    //                \
    //                 \
    //                  `--<Reset flag>
    //                     [Nothing to execute]
    //
    reset_have_synchronous_task();
    Dout(dc::notice(debug_output)|continued_cf, "Calling mainloop() = ");
    if (mainloop().is_momentary_true())
    {
      Dout(dc::finish, "true");
      set_have_synchronous_task();                // Task were remaining-- call handle_synchronous_task() again next frame.
    }
    else
      Dout(dc::finish, "false");
  }
};

} // namespace vulkan
