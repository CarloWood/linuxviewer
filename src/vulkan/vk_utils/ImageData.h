#include "stb_image.h"
#include "memory/DataFeeder.h"
#include "utils/Badge.h"
#include <vulkan/vulkan.hpp>
#include <vector>
#include <filesystem>
#include <cstddef>
#include "debug.h"

namespace vk_utils {
namespace stbi {

class ImageDataFeeder;

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

  // Transfer data ownership.
  std::byte* move_image_data(utils::Badge<ImageDataFeeder>) &&
  {
    std::byte* image_data = m_image_data;
    m_image_data = nullptr;
    return image_data;
  }
};

class ImageDataFeeder final : public vulkan::DataFeeder
{
 private:
  std::byte* m_image_data;
  uint32_t m_size;

 public:
  ImageDataFeeder(ImageData&& image_data) : m_image_data(std::move(image_data).move_image_data({})), m_size(image_data.size())
  {
    // Don't create an ImageDataFeeder from a ImageData that doesn't have data.
    ASSERT(m_image_data);
  }

  ~ImageDataFeeder() override
  {
    stbi_image_free(m_image_data);
  }

  uint32_t chunk_size() const override { return m_size; }
  int chunk_count() const override { return 1; }
  int next_batch() override { return 1; }
  void get_chunks(unsigned char* chunk_ptr) override { std::memcpy(chunk_ptr, m_image_data, m_size); }
};

} // namespace stbi
} // namespace vk_utils
