#include "sys.h"
#include "ImmediateSubmitRequest.h"
#include "ImmediateSubmit.h"

namespace vulkan {

void ImmediateSubmitRequest::submit_finished() const
{
  m_immediate_submit->signal(task::ImmediateSubmit::submit_finished);
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
