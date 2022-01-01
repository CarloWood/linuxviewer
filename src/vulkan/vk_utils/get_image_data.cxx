#include "sys.h"
#include "get_image_data.h"
#include "get_binary_file_contents.h"
#include "utils/AIAlert.h"
#include <cstring>
#include <stdexcept>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

namespace vk_utils {

// Function loading image (texture) data from a specified file.
std::vector<std::byte> get_image_data(std::filesystem::path const& filename, int requested_components, int* width, int* height, int* components, int* data_size)
{
  auto file_data = get_binary_file_contents(filename);
  int tmp_width = 0, tmp_height = 0, tmp_components = 0;

  unsigned char* image_data = stbi_load_from_memory(
      reinterpret_cast<unsigned char*>(file_data.data()), static_cast<int>(file_data.size()), &tmp_width, &tmp_height, &tmp_components, requested_components);

  if (image_data == nullptr || tmp_width <= 0 || tmp_height <= 0 || tmp_components <= 0)
    THROW_ALERT("Could not get image data for file \"[FILENAME]\"", AIArgs("[FILENAME]", filename));

  int size = tmp_width * tmp_height * (requested_components <= 0 ? tmp_components : requested_components);

  if (data_size)
    *data_size = size;
  if (width)
    *width = tmp_width;
  if (height)
    *height = tmp_height;
  if (components)
    *components = tmp_components;

  std::vector<std::byte> output(size);
  std::memcpy(output.data(), image_data, size);

  stbi_image_free(image_data);

  return output;
}

} // namespace vk_utils
