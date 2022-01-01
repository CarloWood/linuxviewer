#include <vector>
#include <filesystem>
#include <cstddef>

namespace vk_utils {

std::vector<std::byte> get_image_data(std::filesystem::path const& filename, int requested_components, int* width, int* height, int* components, int* data_size);

} // namespace vk_utils
