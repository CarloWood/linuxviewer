#include "sys.h"

#include "ImageKind.h"
#include "vk_utils/print_flags.h"
#include "vk_utils/PrintList.h"

namespace vulkan {

void ImageKind::print_members(std::ostream& os) const
{
  os << "image_type:" << image_type <<
      ", format:" << format <<
      ", mip_levels:" << mip_levels <<
      ", array_layers:" << array_layers <<
      ", samples:" << samples <<
      ", tiling:" << tiling <<
      ", usage:" << usage <<
      ", sharing_mode:" << sharing_mode;
  if (sharing_mode == vk::SharingMode::eConcurrent)
    os << vk_utils::print_list(m_queue_family_indices, m_queue_family_index_count);
  os << ", initial_layout:" << initial_layout;
}

} // namespace vukan
