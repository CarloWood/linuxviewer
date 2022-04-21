#include "sys.h"
#include "ImmediateSubmitData.h"

namespace vulkan {

#ifdef CWDEBUG
void ImmediateSubmitData::print_on(std::ostream& os) const
{
  os << "{m_logical_device:" << m_logical_device <<
    ", m_queue_request_key:" << m_queue_request_key <<
    ", m_record_function:" << (m_record_function ? "<set>" : "nullptr") << '}';
}
#endif

} // namespace vulkan
