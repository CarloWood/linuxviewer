#include "sys.h"
#define STB_IMAGE_IMPLEMENTATION
#include "ImageData.h"
#include "get_binary_file_contents.h"
#include "utils/AIAlert.h"
#include <cstring>
#include <stdexcept>
#include "debug.h"

namespace vk_utils {
namespace stbi {

// Load image (texture) data from a specified file.
ImageData::ImageData(std::filesystem::path const& filename, int requested_components)
{
  DoutEntering(dc::vulkan, "ImageData::ImageData(" << filename << ", " << requested_components << ")");

  auto const file_data = get_binary_file_contents(filename);

  int width = 0, height = 0;
  m_image_data = reinterpret_cast<std::byte*>(stbi_load_from_memory(
      reinterpret_cast<stbi_uc const*>(file_data.data()), static_cast<int>(file_data.size()), &width, &height, &m_components, requested_components));
  // These casts are OK because of the test below.
  m_extent = vk::Extent2D{ static_cast<uint32_t>(width), static_cast<uint32_t>(height) };

  if (m_image_data == nullptr || width <= 0 || height <= 0 || m_components <= 0)
    THROW_ALERT("Could not get image data for file \"[FILENAME]\"", AIArgs("[FILENAME]", filename));

  m_size = width * height * (requested_components > 0 ? requested_components : m_components);
}

ImageData::~ImageData()
{
  if (m_image_data)
    stbi_image_free(m_image_data);
}

} // namespace stbi
} // namespace vk_utils
