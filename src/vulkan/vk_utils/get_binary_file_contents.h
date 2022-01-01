#include <vector>
#include <cstddef>
#include <filesystem>

namespace vk_utils {

std::vector<std::byte> get_binary_file_contents(std::filesystem::path const& filename);

} // namespace vk_utils
