#include "sys.h"
#include "rendergraph/AttachmentNode.h"
#include "debug_ostream_operators.h"
#include "vk_utils/print_version.h"
#include <iostream>

namespace vk {

using NAMESPACE_DEBUG::print_string;
using vk_utils::print_api_version;

std::ostream& operator<<(std::ostream& os, vk::AttachmentReference const& attachment_reference)
{
  os << '{';
  vulkan::rendergraph::AttachmentIndex attachment{attachment_reference.attachment};
  os << "attachment:" << attachment <<
      ", layout:" << to_string(attachment_reference.layout);
  os << '}';
  return os;
}

} // namespace vk
