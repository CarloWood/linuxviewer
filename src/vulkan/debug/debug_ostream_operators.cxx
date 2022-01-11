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
  vulkan::rendergraph::pAttachmentsIndex attachment{attachment_reference.attachment};
  os << "attachment:" << attachment <<
      ", layout:" << to_string(attachment_reference.layout);
  os << '}';
  return os;
}

#ifdef CWDEBUG
std::ostream& operator<<(std::ostream& os, vk::ClearColorValue const& value)
{
  // Try to heuristically determine the format.
  // It is not possible to distinguish all zeroes, so lets call that just "zeroed".
  bool is_zeroed = true;
  bool is_float = true;
  bool is_unsigned = true;
  bool is_signed = true;
  bool is_super_small_float = true;
  for (int i = 0; i < 4; ++i)
  {
    if (value.uint32[i] == 0)
      continue;
    is_zeroed = false;
    if (value.uint32[i] > 1000000)              // Probably just negative uint.
      is_unsigned = false;
    if (std::isnan(value.float32[i]) || value.float32[i] < -1.f || value.float32[i] > 100.f) // Should really be between 0 and 1, but well.
      is_float = false;
    else if (value.float32[i] > 10e-40)
      is_super_small_float = false;
    if (value.int32[i] < -100000 || value.int32[i] > 1000000)
      is_signed = false;
  }
  if (is_super_small_float && !is_zeroed)
    is_float = false;
  os << '{';
  if (is_zeroed)
    os << "<zeroed>";
  else
  {
    if (is_float)
    {
      os << "float32:" << value.float32;
      if (is_unsigned || is_signed)
        os << " / ";
    }
    if (is_unsigned)
    {
      if (!is_signed)
        os << "uint32:";
      os << value.uint32;
    }
    else if (is_signed)
      os << "int32:" << value.int32;
  }
  os << '}';
  return os;
}

std::ostream& operator<<(std::ostream& os, vk::ClearDepthStencilValue const& value)
{
  os << '{';
  os << "depth:" << value.depth <<
      ", stencil:0x" << std::hex << value.stencil << std::dec;
  os << '}';
  return os;
}
#endif

} // namespace vk
