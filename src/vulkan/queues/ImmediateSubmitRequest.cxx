#include "sys.h"
#include "ImmediateSubmitRequest.h"
#include "ImmediateSubmit.h"

namespace vulkan {

void ImmediateSubmitRequest::finished() const
{
  m_immediate_submit->signal(task::ImmediateSubmit::submit_finished);
}

void ImmediateSubmitRequest::issued(uint64_t signal_value) const
{
  m_immediate_submit->set_signal_value(signal_value);
  m_immediate_submit->signal(task::ImmediateSubmit::submit_issued);
}

#ifdef CWDEBUG
void ImmediateSubmitRequest::print_on(std::ostream& os) const
{
  os << "{m_logical_device:" << m_logical_device <<
    ", m_queue_request_key:" << m_queue_request_key <<
    ", m_record_function:" << (m_record_function ? "<set>" : "nullptr") << '}';
}
#endif

} // namespace vulkan
