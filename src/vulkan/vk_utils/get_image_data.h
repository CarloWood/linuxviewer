#include <vector>
#include <string>

namespace vk_utils {

std::vector<char> get_image_data(std::string const& filename, int requested_components, int* width, int* height, int* components, int* data_size);

} // namespace vk_utils
