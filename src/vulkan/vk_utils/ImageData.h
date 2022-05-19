#include <vulkan/vulkan.hpp>
#include <vector>
#include <filesystem>
#include <cstddef>

namespace vk_utils {
namespace stbi {

class ImageData
{
 private:
  std::byte* m_image_data;
  vk::Extent2D m_extent;
  int m_components{};
  uint32_t m_size;              // Size in bytes (int is large enough to store an image with 4 components and extent 32768x32768).

 public:
  ImageData(std::filesystem::path const& filename, int requested_components);
  ~ImageData();

  // Accessors.
  vk::Extent2D extent() const { return m_extent; }
  int components() const { return m_components; }
  uint32_t size() const { return m_size; }
  std::byte* image_data() const { return m_image_data; }
};

} // namespace stbi
} // namespace vk_utils
