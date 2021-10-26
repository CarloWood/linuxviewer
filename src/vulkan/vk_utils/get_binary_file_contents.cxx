#include "sys.h"
#include "get_binary_file_contents.h"
#include "utils/AIAlert.h"
#include <fstream>
#include <stdexcept>
#include <filesystem>

namespace vk_utils {

// Function reading binary contents of a file.
std::vector<char> get_binary_file_contents(std::filesystem::path const& filename)
{
  std::ifstream file(filename, std::ios::binary);

  if (file.fail())
    THROW_ALERT("Could not open [FILENAME] file!", AIArgs("[FILENAME]", filename));

  std::streampos begin, end;
  begin = file.tellg();
  file.seekg(0, std::ios::end);
  end = file.tellg();

  std::vector<char> result(static_cast<size_t>(end - begin));
  file.seekg(0, std::ios::beg);
  file.read(&result[0], end - begin);
  file.close();

  return result;
}

} // namespace vk_utils
