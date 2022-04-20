#include "sys.h"
#include "ImmediateSubmit.h"

namespace task {

ImmediateSubmit::~ImmediateSubmit()
{
  DoutEntering(dc::statefultask(mSMDebug), "ImmediateSubmit::~ImmediateSubmit() [" << this << "]");
}

char const* ImmediateSubmit::state_str_impl(state_type run_state) const
{
  switch (run_state)
  {
    AI_CASE_RETURN(ImmediateSubmit_start);
    AI_CASE_RETURN(ImmediateSubmit_done);
  }
  AI_NEVER_REACHED
}

void ImmediateSubmit::multiplex_impl(state_type run_state)
{
  switch (run_state)
  {
    case ImmediateSubmit_start:
      break;
    case ImmediateSubmit_done:
      break;
  }
}

} // namespace task
