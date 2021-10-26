#include <vector>
#include <string>
#include <filesystem>

namespace vk_utils {

std::vector<char> get_binary_file_contents(std::filesystem::path const& filename);

} // namespace vk_utils
